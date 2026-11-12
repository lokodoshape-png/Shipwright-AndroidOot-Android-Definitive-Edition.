#pragma once
// OOT D.E. Definitive Expansion: advanced save manager (backups, integrity check, export, import, restore).
//
// Works on the game's own save files (<app folder>/Save/file1..3.sav, written by soh/SaveManager.cpp) without
// changing their format. Everything it creates lives under <app folder>/ootde/:
//   ootde/backups/file<N>/file<N>_<unix time in ms>_<kind>.sav   backups of slot N
//   ootde/exports/                                           exported saves; files put here can be imported
// Rules: a valid save is never replaced without a backup of it first; a file is validated before it replaces a save;
// replacing goes through a complete temporary copy and an atomic rename, so an interruption leaves the old save.
// Plain C++17 + nlohmann::json, no libultraship dependency: unit tested on the host.
#include <cstdint>
#include <filesystem>
#include <mutex>
#include <string>
#include <vector>

namespace DE {

struct SaveInfo {
    bool exists = false;
    bool valid = false;
    std::string problem; // why it is not valid, in pt-BR
    std::string playerName;
    int baseVersion = 0;
    int hearts = 0;         // heart containers (healthCapacity / 16)
    int deaths = 0;
    int rupees = 0;
    int skulltulas = 0;
    bool randomizer = false;
    bool masterQuest = false;
    uintmax_t sizeBytes = 0;
};

// Highest "base" section version the game can load (SaveManager::LoadBaseVersion1..4).
constexpr int kMaxBaseSectionVersion = 4;

// Reads and checks a save file the way SaveManager::LoadFile will need it.
SaveInfo InspectSave(const std::filesystem::path& path);

// Converts the game's file-name characters (NTSC or PAL table) to readable text.
std::string DecodePlayerName(const std::vector<int>& chars, int filenameLanguage);

enum class BackupKind { Auto, Manual, BeforeRestore, BeforeImport, BeforeDelete, BeforeCopy };
const char* BackupKindId(BackupKind kind);     // used in file names
const char* BackupKindLabel(const std::string& kind); // pt-BR label for the menu

struct BackupEntry {
    std::filesystem::path path;
    int slot = 0;      // 0-based
    int64_t time = 0;  // unix seconds
    int64_t timeMs = 0; // unix milliseconds (order of backups)
    std::string kind;  // BackupKindId
};

class SaveVault {
  public:
    static constexpr int kSlots = 3;

    SaveVault(std::filesystem::path saveDir, std::filesystem::path dataDir);

    std::filesystem::path SlotPath(int slot) const;
    std::filesystem::path BackupDir(int slot) const;
    std::filesystem::path ExportDir() const;

    // Copies the current save of `slot` into its backup folder. Fails (returning false) if the slot is empty.
    bool CreateBackup(int slot, BackupKind kind, std::string* error = nullptr,
                      std::filesystem::path* created = nullptr);
    // Automatic backup before the game overwrites a save: made only if the newest automatic backup is older than
    // `minIntervalSeconds`, the current file is valid, and keeps at most `keep` automatic backups of the slot.
    bool AutoBackupBeforeOverwrite(int slot, int64_t minIntervalSeconds, int keep);

    std::vector<BackupEntry> ListBackups(int slot) const; // newest first
    void PruneBackups(int slot, const std::string& kind, int keep);
    bool DeleteBackup(const BackupEntry& entry, std::string* error = nullptr);

    // Replaces the save of `slot` with `source` (a backup or an imported file). Order: validate source, back up the
    // current save, write a temporary copy, validate it, atomically rename it over the save.
    bool RestoreInto(int slot, const std::filesystem::path& source, BackupKind safetyKind,
                     std::string* error = nullptr);

    bool Export(int slot, std::string* error = nullptr, std::filesystem::path* created = nullptr);
    std::vector<std::filesystem::path> ListImportable() const;

  private:
    std::filesystem::path mSaveDir;
    std::filesystem::path mDataDir;
    mutable std::recursive_mutex mMutex;
};

} // namespace DE
