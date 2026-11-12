#pragma once
// OOT D.E. Definitive Expansion: safe file helpers shared by the expansion systems (settings, save manager, and later
// achievements, records and the explorer). Plain C++17, no libultraship dependency, so it is unit tested on the host
// (scripts/tests/run_de_expansion_tests.sh).
#include <cstdint>
#include <filesystem>
#include <string>

namespace DE {

// Writes `data` to `path` through a temporary file in the same folder that is flushed to disk and then renamed over
// the target. A crash or a full disk leaves either the old file or the new one, never a half-written file.
// Returns false (and leaves the old file untouched) if anything fails; `error` says what.
bool AtomicWriteFile(const std::filesystem::path& path, const std::string& data, std::string* error = nullptr);

// Copies `from` to `to` with the same guarantee as AtomicWriteFile.
bool AtomicCopyFile(const std::filesystem::path& from, const std::filesystem::path& to, std::string* error = nullptr);

// Replaces `to` with the already complete file `from` (same folder), atomically where the platform allows it.
bool AtomicReplace(const std::filesystem::path& from, const std::filesystem::path& to, std::string* error = nullptr);

bool ReadWholeFile(const std::filesystem::path& path, std::string& out);

// Local time as "YYYYMMDD-HHMMSS" (sortable, safe in file names) and as "DD/MM/YYYY HH:MM" (for the menu).
std::string TimestampForFileName(int64_t unixSeconds);
std::string TimestampForDisplay(int64_t unixSeconds);
int64_t NowUnixSeconds();
int64_t NowUnixMillis();

} // namespace DE
