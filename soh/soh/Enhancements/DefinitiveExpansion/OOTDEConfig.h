#pragma once
// OOT D.E. Definitive Expansion: central settings of the expansion systems ("OOTDEConfig").
//
// - Kept in its own file (<app folder>/ootde/config.json), apart from the game saves (Save/fileN.sav) and from the
//   Ship of Harkinian settings (shipofharkinian.json), so turning the expansion off or deleting this file never
//   touches a campaign.
// - Versioned: "schemaVersion" says how the file is laid out. Older layouts are migrated step by step on load
//   (Migrate), the original is kept as config.json.v<N>.bak first. A file from a newer app is kept as a backup and
//   not overwritten silently.
// - Every setting has a registered default, so "restore defaults" is exact and a missing or broken file just means
//   defaults.
// - Profiles: named copies of the settings that can be applied later (control layouts, HUD layouts, ...).
// Plain C++17 + nlohmann::json, no libultraship dependency: unit tested on the host.
#include <filesystem>
#include <map>
#include <mutex>
#include <string>
#include <vector>
#include <nlohmann/json.hpp>

namespace DE {

class Config {
  public:
    static constexpr int kSchemaVersion = 1;

    struct SettingInfo {
        nlohmann::json defaultValue;
        std::string description;
    };

    enum class LoadResult {
        CreatedNew,    // no file yet: defaults
        Loaded,        // read as is
        Migrated,      // read an older layout and converted it (original kept as .bak)
        RecoveredBroken, // file was unreadable: kept as .corrompido-<time>, defaults used
        NewerVersion,  // written by a newer app: kept as .bak, values that are known here are used
    };

    // Settings must be registered before Load so unknown keys can be told apart from missing ones.
    void RegisterSetting(const std::string& key, const nlohmann::json& defaultValue, const std::string& description);

    LoadResult Load(const std::filesystem::path& path);
    bool Save(std::string* error = nullptr);

    nlohmann::json Get(const std::string& key) const;
    bool GetBool(const std::string& key) const;
    int GetInt(const std::string& key) const;
    float GetFloat(const std::string& key) const;
    std::string GetString(const std::string& key) const;
    // Sets a value in memory; returns false if the key is unknown or the type differs from the default's.
    bool Set(const std::string& key, const nlohmann::json& value);
    bool IsDefault(const std::string& key) const;

    // Restores every setting whose key starts with `prefix` ("" = all). Profiles are kept.
    void ResetToDefaults(const std::string& prefix = "");

    // Profiles hold the settings under `prefix` (for example "touch." or "hud.") at the time they were saved.
    bool SaveProfile(const std::string& name, const std::string& prefix = "");
    bool ApplyProfile(const std::string& name);
    bool DeleteProfile(const std::string& name);
    std::vector<std::string> ListProfiles() const;

    const std::map<std::string, SettingInfo>& Registry() const {
        return mRegistry;
    }
    const std::filesystem::path& Path() const {
        return mPath;
    }
    int LoadedSchemaVersion() const {
        return mLoadedSchemaVersion;
    }

    // Converts `doc` from `fromVersion` to kSchemaVersion. Public for the tests.
    static bool Migrate(nlohmann::json& doc, int fromVersion);

  private:
    bool TypeMatches(const nlohmann::json& a, const nlohmann::json& b) const;
    nlohmann::json ToJsonLocked() const;

    mutable std::recursive_mutex mMutex;
    std::filesystem::path mPath;
    std::map<std::string, SettingInfo> mRegistry;
    std::map<std::string, nlohmann::json> mValues;   // values the player changed (defaults are not stored)
    std::map<std::string, nlohmann::json> mProfiles; // name -> { "prefix": ..., "settings": {...} }
    nlohmann::json mUnknown = nlohmann::json::object(); // keys from a newer app, written back unchanged
    int mLoadedSchemaVersion = kSchemaVersion;
};

} // namespace DE
