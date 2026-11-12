#include "DEStorage.h"

#include <atomic>
#include <chrono>
#include <cstdio>
#include <ctime>
#include <fstream>
#include <sstream>
#include <system_error>

#if defined(_WIN32)
#include <io.h>
#else
#include <unistd.h>
#endif

namespace fs = std::filesystem;

namespace DE {

static void SetError(std::string* error, const std::string& message) {
    if (error != nullptr) {
        *error = message;
    }
}

static fs::path TempPathFor(const fs::path& path) {
    // Unique per process and call, so two writers never share a temporary file.
    static std::atomic<uint32_t> sCounter{ 0 };
    const uint32_t id = ++sCounter;
    return path.parent_path() / (path.filename().string() + ".detmp" + std::to_string(id));
}

static bool WriteAndSync(const fs::path& path, const std::string& data, std::string* error) {
    FILE* file = std::fopen(path.string().c_str(), "wb");
    if (file == nullptr) {
        SetError(error, "não foi possível criar " + path.string());
        return false;
    }
    bool ok = data.empty() || std::fwrite(data.data(), 1, data.size(), file) == data.size();
    ok = (std::fflush(file) == 0) && ok;
#if defined(_WIN32)
    ok = (_commit(_fileno(file)) == 0) && ok;
#else
    ok = (fsync(fileno(file)) == 0) && ok;
#endif
    ok = (std::fclose(file) == 0) && ok;
    if (!ok) {
        SetError(error, "falha ao gravar " + path.string() + " (armazenamento cheio?)");
    }
    return ok;
}

bool AtomicReplace(const fs::path& from, const fs::path& to, std::string* error) {
    std::error_code ec;
    // POSIX rename replaces the target atomically; the MSVC standard library uses MoveFileEx(REPLACE_EXISTING).
    fs::rename(from, to, ec);
    if (!ec) {
        return true;
    }
    // Some file systems refuse to rename over an existing file. Fall back to remove + rename: the new data is already
    // complete on disk, so the window without a target file is tiny and the data is never lost (`from` remains).
    std::error_code removeEc;
    fs::remove(to, removeEc);
    fs::rename(from, to, ec);
    if (ec) {
        SetError(error, "não foi possível substituir " + to.string() + ": " + ec.message());
        return false;
    }
    return true;
}

bool AtomicWriteFile(const fs::path& path, const std::string& data, std::string* error) {
    std::error_code ec;
    if (!path.parent_path().empty()) {
        fs::create_directories(path.parent_path(), ec);
    }
    const fs::path temp = TempPathFor(path);
    if (!WriteAndSync(temp, data, error)) {
        fs::remove(temp, ec);
        return false;
    }
    if (!AtomicReplace(temp, path, error)) {
        fs::remove(temp, ec);
        return false;
    }
    return true;
}

bool ReadWholeFile(const fs::path& path, std::string& out) {
    std::ifstream input(path, std::ios::binary);
    if (!input) {
        return false;
    }
    std::ostringstream buffer;
    buffer << input.rdbuf();
    if (input.bad()) {
        return false;
    }
    out = buffer.str();
    return true;
}

bool AtomicCopyFile(const fs::path& from, const fs::path& to, std::string* error) {
    std::string data;
    if (!ReadWholeFile(from, data)) {
        SetError(error, "não foi possível ler " + from.string());
        return false;
    }
    return AtomicWriteFile(to, data, error);
}

static std::tm LocalTime(int64_t unixSeconds) {
    std::time_t t = static_cast<std::time_t>(unixSeconds);
    std::tm result{};
#if defined(_WIN32)
    localtime_s(&result, &t);
#else
    localtime_r(&t, &result);
#endif
    return result;
}

std::string TimestampForFileName(int64_t unixSeconds) {
    std::tm tm = LocalTime(unixSeconds);
    char buffer[32];
    std::strftime(buffer, sizeof(buffer), "%Y%m%d-%H%M%S", &tm);
    return buffer;
}

std::string TimestampForDisplay(int64_t unixSeconds) {
    std::tm tm = LocalTime(unixSeconds);
    char buffer[32];
    std::strftime(buffer, sizeof(buffer), "%d/%m/%Y %H:%M", &tm);
    return buffer;
}

int64_t NowUnixSeconds() {
    return std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch())
        .count();
}

int64_t NowUnixMillis() {
    return std::chrono::duration_cast<std::chrono::milliseconds>(
               std::chrono::system_clock::now().time_since_epoch())
        .count();
}

} // namespace DE
