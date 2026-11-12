#include "DERecords.h"
#include "DEStorage.h"

#include <algorithm>
#include <cstdio>
#include <system_error>
#include <nlohmann/json.hpp>

namespace fs = std::filesystem;
using nlohmann::json;

namespace DE {

int ComputeRunScore(int64_t timeMs, int deaths, int healthLeft, int healthMax, int difficultyPercent) {
    // 10000 points, minus 5 per second, minus 500 per death, plus up to 2000 for the share of health left.
    int64_t score = 10000 - (timeMs / 1000) * 5 - static_cast<int64_t>(deaths) * 500;
    if (healthMax > 0) {
        score += static_cast<int64_t>(std::clamp(healthLeft, 0, healthMax)) * 2000 / healthMax;
    }
    score = std::max<int64_t>(score, 100); // finishing is always worth something
    score = score * std::max(difficultyPercent, 1) / 100;
    return static_cast<int>(std::min<int64_t>(score, 999999));
}

std::string FormatRunTime(int64_t timeMs) {
    if (timeMs < 0) {
        timeMs = 0;
    }
    const int64_t tenths = timeMs / 100;
    const int64_t hours = tenths / 36000;
    const int minutes = static_cast<int>((tenths / 600) % 60);
    const int seconds = static_cast<int>((tenths / 10) % 60);
    const int tenth = static_cast<int>(tenths % 10);
    char buffer[48];
    if (hours > 0) {
        std::snprintf(buffer, sizeof(buffer), "%lld:%02d:%02d.%d", static_cast<long long>(hours), minutes, seconds,
                      tenth);
    } else {
        std::snprintf(buffer, sizeof(buffer), "%d:%02d.%d", minutes, seconds, tenth);
    }
    return buffer;
}

static bool Better(const RunRecord& a, const RunRecord& b) {
    if (a.timeMs != b.timeMs) {
        return a.timeMs < b.timeMs;
    }
    if (a.deaths != b.deaths) {
        return a.deaths < b.deaths;
    }
    return a.date < b.date; // the earlier run keeps the place
}

bool RecordBook::Load(const fs::path& path) {
    std::lock_guard<std::recursive_mutex> lock(mMutex);
    mPath = path;
    mRuns.clear();
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
    const int version = doc["schemaVersion"].get<int>();
    if (version > kSchemaVersion) {
        // A newer app wrote this: keep it untouched and start an empty book in memory only until saved.
        AtomicCopyFile(path, path.string() + ".v" + std::to_string(version) + ".bak");
        return false;
    }
    if (doc.contains("runs") && doc["runs"].is_array()) {
        for (const json& entry : doc["runs"]) {
            if (!entry.is_object() || !entry.contains("board") || !entry["board"].is_string()) {
                continue;
            }
            RunRecord run;
            run.board = entry["board"].get<std::string>();
            auto integer = [&entry](const char* key) -> int64_t {
                return entry.contains(key) && entry[key].is_number_integer() ? entry[key].get<int64_t>() : 0;
            };
            run.timeMs = std::max<int64_t>(0, integer("timeMs"));
            run.deaths = static_cast<int>(std::max<int64_t>(0, integer("deaths")));
            run.healthLeft = static_cast<int>(integer("healthLeft"));
            run.healthMax = static_cast<int>(integer("healthMax"));
            run.score = static_cast<int>(integer("score"));
            run.date = integer("date");
            if (entry.contains("details") && entry["details"].is_string()) {
                run.details = entry["details"].get<std::string>();
            }
            mRuns.push_back(run);
        }
    }
    return true;
}

bool RecordBook::Save(std::string* error) {
    std::lock_guard<std::recursive_mutex> lock(mMutex);
    if (mPath.empty()) {
        return false;
    }
    json runs = json::array();
    for (const RunRecord& run : mRuns) {
        runs.push_back(json{ { "board", run.board },
                             { "timeMs", run.timeMs },
                             { "deaths", run.deaths },
                             { "healthLeft", run.healthLeft },
                             { "healthMax", run.healthMax },
                             { "score", run.score },
                             { "date", run.date },
                             { "details", run.details } });
    }
    const json doc = { { "schemaVersion", kSchemaVersion }, { "runs", runs } };
    return AtomicWriteFile(mPath, doc.dump(2), error);
}

int RecordBook::Add(const RunRecord& run) {
    std::lock_guard<std::recursive_mutex> lock(mMutex);
    mRuns.push_back(run);
    // The history is capped, but a board's best run is never dropped.
    while (mRuns.size() > kMaxHistory) {
        bool removed = false;
        for (size_t i = 0; i < mRuns.size(); i++) {
            RunRecord best;
            if (Best(mRuns[i].board, best) && best.date == mRuns[i].date && best.timeMs == mRuns[i].timeMs) {
                continue;
            }
            mRuns.erase(mRuns.begin() + static_cast<std::ptrdiff_t>(i));
            removed = true;
            break;
        }
        if (!removed) {
            break;
        }
    }
    int position = 1;
    for (const RunRecord& other : Ranking(run.board)) {
        if (other.date == run.date && other.timeMs == run.timeMs && other.deaths == run.deaths) {
            break;
        }
        position++;
    }
    return position;
}

std::vector<RunRecord> RecordBook::History() const {
    std::lock_guard<std::recursive_mutex> lock(mMutex);
    return std::vector<RunRecord>(mRuns.rbegin(), mRuns.rend());
}

std::vector<RunRecord> RecordBook::Ranking(const std::string& board) const {
    std::lock_guard<std::recursive_mutex> lock(mMutex);
    std::vector<RunRecord> ranking;
    for (const RunRecord& run : mRuns) {
        if (run.board == board) {
            ranking.push_back(run);
        }
    }
    std::stable_sort(ranking.begin(), ranking.end(), Better);
    return ranking;
}

std::vector<std::string> RecordBook::Boards() const {
    std::lock_guard<std::recursive_mutex> lock(mMutex);
    std::vector<std::string> boards;
    for (const RunRecord& run : mRuns) {
        if (std::find(boards.begin(), boards.end(), run.board) == boards.end()) {
            boards.push_back(run.board);
        }
    }
    std::sort(boards.begin(), boards.end());
    return boards;
}

bool RecordBook::Best(const std::string& board, RunRecord& out) const {
    const std::vector<RunRecord> ranking = Ranking(board);
    if (ranking.empty()) {
        return false;
    }
    out = ranking.front();
    return true;
}

} // namespace DE
