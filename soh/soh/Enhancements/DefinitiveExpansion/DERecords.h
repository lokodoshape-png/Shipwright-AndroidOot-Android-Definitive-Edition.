#pragma once
// OOT D.E. Definitive Expansion: local records and history of finished runs (Boss Rush, challenges).
//
// Kept in <app folder>/ootde/records.json, apart from the game saves, versioned and written atomically.
// A "board" is one ranking (for example "bossrush/normal/all"); runs are ranked inside their own board only.
// Plain C++17 + nlohmann::json, no libultraship dependency: unit tested on the host.
#include <cstdint>
#include <filesystem>
#include <mutex>
#include <string>
#include <vector>

namespace DE {

struct RunRecord {
    std::string board;    // ranking this run belongs to
    int64_t timeMs = 0;   // real time, from a monotonic clock
    int deaths = 0;
    int healthLeft = 0;   // quarter hearts left at the end (16 per heart)
    int healthMax = 0;
    int score = 0;
    int64_t date = 0;     // unix seconds
    std::string details;  // free text shown in the history (options used)
};

// Score of a finished run: faster, fewer deaths and more health left score more. `difficultyPercent` scales it
// (100 = normal). Documented in docs/DEFINITIVE-EXPANSION.md; change it only with a new schema version.
int ComputeRunScore(int64_t timeMs, int deaths, int healthLeft, int healthMax, int difficultyPercent);

std::string FormatRunTime(int64_t timeMs); // "M:SS.d" or "H:MM:SS.d"

class RecordBook {
  public:
    static constexpr int kSchemaVersion = 1;
    static constexpr size_t kMaxHistory = 200;

    bool Load(const std::filesystem::path& path);
    bool Save(std::string* error = nullptr);

    // Adds a finished run. Returns its 1-based position in its board's ranking (1 = new record).
    int Add(const RunRecord& run);

    std::vector<RunRecord> History() const;                          // newest first
    std::vector<RunRecord> Ranking(const std::string& board) const;  // best first (time, then deaths, then date)
    std::vector<std::string> Boards() const;
    bool Best(const std::string& board, RunRecord& out) const;

  private:
    std::vector<RunRecord> mRuns; // oldest first
    std::filesystem::path mPath;
    mutable std::recursive_mutex mMutex;
};

} // namespace DE
