#include "OOTDEConfig.h"
#include "DEStorage.h"

#include <system_error>

namespace fs = std::filesystem;
using nlohmann::json;

namespace DE {

void Config::RegisterSetting(const std::string& key, const json& defaultValue, const std::string& description) {
    std::lock_guard<std::recursive_mutex> lock(mMutex);
    mRegistry[key] = SettingInfo{ defaultValue, description };
}

bool Config::TypeMatches(const json& a, const json& b) const {
    if (a.is_number() && b.is_number()) {
        // An int setting must stay an int; a float one accepts ints.
        return !(a.is_number_integer() && b.is_number_float());
    }
    return a.type() == b.type();
}

// Layout history:
//   (no file) -> 1: first layout { schemaVersion, settings{key:value}, profiles{name:{prefix, settings}} }.
// To add version 2: bump kSchemaVersion, add `case 1:` converting the document to the 2 layout, then fall through.
bool Config::Migrate(json& doc, int fromVersion) {
    if (!doc.is_object() || fromVersion < 0 || fromVersion > kSchemaVersion) {
        return false;
    }
    switch (fromVersion) {
        case 0:
            // Version 0 = a bare object of settings (never released, accepted for hand-made files).
            doc = json{ { "schemaVersion", 1 }, { "settings", doc }, { "profiles", json::object() } };
            [[fallthrough]];
        case kSchemaVersion:
            break;
    }
    doc["schemaVersion"] = kSchemaVersion;
    return true;
}

Config::LoadResult Config::Load(const fs::path& path) {
    std::lock_guard<std::recursive_mutex> lock(mMutex);
    mPath = path;
    mValues.clear();
    mProfiles.clear();
    mUnknown = json::object();
    mLoadedSchemaVersion = kSchemaVersion;

    std::error_code ec;
    if (!fs::exists(path, ec)) {
        return LoadResult::CreatedNew;
    }

    std::string text;
    json doc;
    bool parsed = ReadWholeFile(path, text);
    if (parsed) {
        doc = json::parse(text, nullptr, false);
        parsed = !doc.is_discarded() && doc.is_object();
    }
    if (!parsed) {
        const fs::path broken = path.string() + ".corrompido-" + TimestampForFileName(NowUnixSeconds());
        fs::rename(path, broken, ec);
        return LoadResult::RecoveredBroken;
    }

    LoadResult result = LoadResult::Loaded;
    int version = 0;
    if (doc.contains("schemaVersion") && doc["schemaVersion"].is_number_integer()) {
        version = doc["schemaVersion"].get<int>();
    }
    mLoadedSchemaVersion = version;
    if (version > kSchemaVersion) {
        // Written by a newer app (the player went back to an older APK). Keep that file, use what is understood.
        AtomicCopyFile(path, path.string() + ".v" + std::to_string(version) + ".bak");
        result = LoadResult::NewerVersion;
    } else if (version < kSchemaVersion) {
        AtomicCopyFile(path, path.string() + ".v" + std::to_string(version) + ".bak");
        if (!Migrate(doc, version)) {
            return LoadResult::RecoveredBroken;
        }
        result = LoadResult::Migrated;
    }

    if (doc.contains("settings") && doc["settings"].is_object()) {
        for (auto& [key, value] : doc["settings"].items()) {
            auto info = mRegistry.find(key);
            if (info == mRegistry.end()) {
                mUnknown[key] = value; // kept and written back, so a newer feature's setting is not lost
            } else if (TypeMatches(info->second.defaultValue, value)) {
                mValues[key] = value;
            }
        }
    }
    if (doc.contains("profiles") && doc["profiles"].is_object()) {
        for (auto& [name, profile] : doc["profiles"].items()) {
            if (profile.is_object() && profile.contains("settings") && profile["settings"].is_object()) {
                mProfiles[name] = profile;
            }
        }
    }
    return result;
}

json Config::ToJsonLocked() const {
    json settings = mUnknown;
    for (const auto& [key, value] : mValues) {
        settings[key] = value;
    }
    json profiles = json::object();
    for (const auto& [name, profile] : mProfiles) {
        profiles[name] = profile;
    }
    return json{ { "schemaVersion", kSchemaVersion },
                 { "app", "Ocarina of Time Definitive Edition" },
                 { "settings", settings },
                 { "profiles", profiles } };
}

bool Config::Save(std::string* error) {
    std::lock_guard<std::recursive_mutex> lock(mMutex);
    if (mPath.empty()) {
        if (error != nullptr) {
            *error = "configuração sem caminho";
        }
        return false;
    }
    return AtomicWriteFile(mPath, ToJsonLocked().dump(2), error);
}

json Config::Get(const std::string& key) const {
    std::lock_guard<std::recursive_mutex> lock(mMutex);
    auto value = mValues.find(key);
    if (value != mValues.end()) {
        return value->second;
    }
    auto info = mRegistry.find(key);
    return info != mRegistry.end() ? info->second.defaultValue : json();
}

bool Config::GetBool(const std::string& key) const {
    json value = Get(key);
    return value.is_boolean() ? value.get<bool>() : false;
}

int Config::GetInt(const std::string& key) const {
    json value = Get(key);
    return value.is_number() ? value.get<int>() : 0;
}

float Config::GetFloat(const std::string& key) const {
    json value = Get(key);
    return value.is_number() ? value.get<float>() : 0.0f;
}

std::string Config::GetString(const std::string& key) const {
    json value = Get(key);
    return value.is_string() ? value.get<std::string>() : std::string();
}

bool Config::Set(const std::string& key, const json& value) {
    std::lock_guard<std::recursive_mutex> lock(mMutex);
    auto info = mRegistry.find(key);
    if (info == mRegistry.end() || !TypeMatches(info->second.defaultValue, value)) {
        return false;
    }
    if (value == info->second.defaultValue) {
        mValues.erase(key); // defaults are not stored, so a later change of default reaches the player
    } else {
        mValues[key] = value;
    }
    return true;
}

bool Config::IsDefault(const std::string& key) const {
    std::lock_guard<std::recursive_mutex> lock(mMutex);
    return mValues.find(key) == mValues.end();
}

void Config::ResetToDefaults(const std::string& prefix) {
    std::lock_guard<std::recursive_mutex> lock(mMutex);
    for (auto it = mValues.begin(); it != mValues.end();) {
        if (it->first.rfind(prefix, 0) == 0) {
            it = mValues.erase(it);
        } else {
            ++it;
        }
    }
}

bool Config::SaveProfile(const std::string& name, const std::string& prefix) {
    std::lock_guard<std::recursive_mutex> lock(mMutex);
    if (name.empty()) {
        return false;
    }
    json settings = json::object();
    for (const auto& [key, info] : mRegistry) {
        if (key.rfind(prefix, 0) == 0) {
            settings[key] = Get(key);
        }
    }
    mProfiles[name] = json{ { "prefix", prefix }, { "settings", settings } };
    return true;
}

bool Config::ApplyProfile(const std::string& name) {
    std::lock_guard<std::recursive_mutex> lock(mMutex);
    auto profile = mProfiles.find(name);
    if (profile == mProfiles.end()) {
        return false;
    }
    // Settings of the profile's area that the profile does not list (added later) go back to their defaults.
    const std::string prefix = profile->second.value("prefix", std::string());
    ResetToDefaults(prefix);
    for (auto& [key, value] : profile->second["settings"].items()) {
        Set(key, value);
    }
    return true;
}

bool Config::DeleteProfile(const std::string& name) {
    std::lock_guard<std::recursive_mutex> lock(mMutex);
    return mProfiles.erase(name) > 0;
}

std::vector<std::string> Config::ListProfiles() const {
    std::lock_guard<std::recursive_mutex> lock(mMutex);
    std::vector<std::string> names;
    for (const auto& [name, profile] : mProfiles) {
        names.push_back(name);
    }
    return names;
}

} // namespace DE
