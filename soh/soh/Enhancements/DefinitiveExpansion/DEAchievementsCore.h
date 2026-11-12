#pragma once
// OOT D.E. Definitive Expansion: achievements store (definitions + progress + unlock dates).
//
// Progress lives in <app folder>/ootde/achievements.json, apart from the game saves: it is global to the player, not
// part of a campaign. The file is versioned and written atomically. Progress only ever goes up, and an achievement
// is unlocked only when a reported value reaches its target, so opening a screen never unlocks anything.
// Plain C++17 + nlohmann::json, no libultraship dependency: unit tested on the host.
#include <cstdint>
#include <filesystem>
#include <map>
#include <mutex>
#include <string>
#include <vector>

namespace DE {

enum class AchievementCategory { Story, Exploration, Combat, Collectibles, Dungeons, Challenges, Secrets };
const char* AchievementCategoryLabel(AchievementCategory category); // pt-BR

struct AchievementDef {
    std::string id;       // key in the file: never rename
    AchievementCategory category;
    std::string title;
    std::string description;
    int target = 1;       // progress needed to unlock
    bool secret = false;  // title and description hidden until unlocked
    std::string icon;     // game texture path, may be empty
};

struct AchievementState {
    int progress = 0;
    int64_t unlockedAt = 0; // unix seconds, 0 = locked
    bool Unlocked() const {
        return unlockedAt != 0;
    }
};

class AchievementStore {
  public:
    static constexpr int kSchemaVersion = 1;

    explicit AchievementStore(std::vector<AchievementDef> defs);

    // Returns false if the file was broken (kept aside) or from a newer version (kept as .bak).
    bool Load(const std::filesystem::path& path);
    bool Save(std::string* error = nullptr);
    bool Dirty() const;

    // Reports the current value of an achievement's progress. Lower values than the stored one are ignored.
    // Returns true when this call unlocked the achievement. `now` is the unlock time (unix seconds).
    bool Report(const std::string& id, int progress, int64_t now);
    // Adds to the progress (for counters such as "enemies defeated").
    bool Add(const std::string& id, int amount, int64_t now);

    const std::vector<AchievementDef>& Defs() const {
        return mDefs;
    }
    const AchievementDef* Find(const std::string& id) const;
    AchievementState State(const std::string& id) const;
    int UnlockedCount() const;
    int Percent() const;
    // Unlocked achievements, newest first.
    std::vector<const AchievementDef*> History() const;

  private:
    std::vector<AchievementDef> mDefs;
    std::map<std::string, AchievementState> mStates;
    std::map<std::string, AchievementState> mUnknown; // from a newer app: written back unchanged
    std::filesystem::path mPath;
    bool mDirty = false;
    mutable std::recursive_mutex mMutex;
};

} // namespace DE
