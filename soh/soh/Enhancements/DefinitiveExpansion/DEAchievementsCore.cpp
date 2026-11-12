#include "DEAchievementsCore.h"
#include "DEStorage.h"

#include <algorithm>
#include <system_error>
#include <nlohmann/json.hpp>

namespace fs = std::filesystem;
using nlohmann::json;

namespace DE {

const char* AchievementCategoryLabel(AchievementCategory category) {
    switch (category) {
        case AchievementCategory::Story:
            return "História";
        case AchievementCategory::Exploration:
            return "Exploração";
        case AchievementCategory::Combat:
            return "Combate";
        case AchievementCategory::Collectibles:
            return "Colecionáveis";
        case AchievementCategory::Dungeons:
            return "Masmorras";
        case AchievementCategory::Challenges:
            return "Desafios";
        case AchievementCategory::Secrets:
            return "Segredos";
    }
    return "";
}

AchievementStore::AchievementStore(std::vector<AchievementDef> defs) : mDefs(std::move(defs)) {
}

const AchievementDef* AchievementStore::Find(const std::string& id) const {
    for (const AchievementDef& def : mDefs) {
        if (def.id == id) {
            return &def;
        }
    }
    return nullptr;
}

bool AchievementStore::Load(const fs::path& path) {
    std::lock_guard<std::recursive_mutex> lock(mMutex);
    mPath = path;
    mStates.clear();
    mUnknown.clear();
    mDirty = false;

    std::error_code ec;
    if (!fs::exists(path, ec)) {
        return true;
    }
    std::string text;
    json doc;
    bool parsed = ReadWholeFile(path, text);
    if (parsed) {
        doc = json::parse(text, nullptr, false);
        parsed = !doc.is_discarded() && doc.is_object() && doc.contains("schemaVersion") &&
                 doc["schemaVersion"].is_number_integer();
    }
    if (!parsed) {
        fs::rename(path, path.string() + ".corrompido-" + TimestampForFileName(NowUnixSeconds()), ec);
        return false;
    }
    bool ok = true;
    const int version = doc["schemaVersion"].get<int>();
    if (version > kSchemaVersion) {
        AtomicCopyFile(path, path.string() + ".v" + std::to_string(version) + ".bak");
        ok = false; // what is understood below is still used
    }
    // version 1 is the only layout so far; older layouts would be converted here.
    if (doc.contains("achievements") && doc["achievements"].is_object()) {
        for (auto& [id, entry] : doc["achievements"].items()) {
            if (!entry.is_object()) {
                continue;
            }
            AchievementState state;
            if (entry.contains("progress") && entry["progress"].is_number_integer()) {
                state.progress = std::max(0, entry["progress"].get<int>());
            }
            if (entry.contains("unlockedAt") && entry["unlockedAt"].is_number_integer()) {
                state.unlockedAt = std::max<int64_t>(0, entry["unlockedAt"].get<int64_t>());
            }
            const AchievementDef* def = Find(id);
            if (def == nullptr) {
                mUnknown[id] = state;
                continue;
            }
            // A file cannot claim an unlock without the progress for it.
            if (state.unlockedAt != 0 && state.progress < def->target) {
                state.unlockedAt = 0;
            }
            mStates[id] = state;
        }
    }
    return ok;
}

bool AchievementStore::Save(std::string* error) {
    std::lock_guard<std::recursive_mutex> lock(mMutex);
    if (mPath.empty()) {
        return false;
    }
    json achievements = json::object();
    for (const auto* states : { &mUnknown, &mStates }) {
        for (const auto& [id, state] : *states) {
            if (state.progress > 0 || state.unlockedAt != 0) {
                achievements[id] = json{ { "progress", state.progress }, { "unlockedAt", state.unlockedAt } };
            }
        }
    }
    const json doc = { { "schemaVersion", kSchemaVersion }, { "achievements", achievements } };
    if (!AtomicWriteFile(mPath, doc.dump(2), error)) {
        return false;
    }
    mDirty = false;
    return true;
}

bool AchievementStore::Dirty() const {
    std::lock_guard<std::recursive_mutex> lock(mMutex);
    return mDirty;
}

bool AchievementStore::Report(const std::string& id, int progress, int64_t now) {
    std::lock_guard<std::recursive_mutex> lock(mMutex);
    const AchievementDef* def = Find(id);
    if (def == nullptr) {
        return false;
    }
    AchievementState& state = mStates[id];
    progress = std::min(progress, def->target);
    if (progress <= state.progress) {
        return false;
    }
    state.progress = progress;
    mDirty = true;
    if (!state.Unlocked() && state.progress >= def->target) {
        state.unlockedAt = now > 0 ? now : 1;
        return true;
    }
    return false;
}

bool AchievementStore::Add(const std::string& id, int amount, int64_t now) {
    std::lock_guard<std::recursive_mutex> lock(mMutex);
    if (amount <= 0) {
        return false;
    }
    return Report(id, State(id).progress + amount, now);
}

AchievementState AchievementStore::State(const std::string& id) const {
    std::lock_guard<std::recursive_mutex> lock(mMutex);
    auto it = mStates.find(id);
    return it != mStates.end() ? it->second : AchievementState{};
}

int AchievementStore::UnlockedCount() const {
    std::lock_guard<std::recursive_mutex> lock(mMutex);
    int count = 0;
    for (const AchievementDef& def : mDefs) {
        count += State(def.id).Unlocked() ? 1 : 0;
    }
    return count;
}

int AchievementStore::Percent() const {
    return mDefs.empty() ? 0 : UnlockedCount() * 100 / static_cast<int>(mDefs.size());
}

std::vector<const AchievementDef*> AchievementStore::History() const {
    std::lock_guard<std::recursive_mutex> lock(mMutex);
    std::vector<const AchievementDef*> unlocked;
    for (const AchievementDef& def : mDefs) {
        if (State(def.id).Unlocked()) {
            unlocked.push_back(&def);
        }
    }
    std::stable_sort(unlocked.begin(), unlocked.end(), [this](const AchievementDef* a, const AchievementDef* b) {
        return State(a->id).unlockedAt > State(b->id).unlockedAt;
    });
    return unlocked;
}

} // namespace DE
