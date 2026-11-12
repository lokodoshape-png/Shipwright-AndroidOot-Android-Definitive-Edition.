#include "DESaveManager.h"
#include "DEStorage.h"

#include <algorithm>
#include <cstdio>
#include <system_error>
#include <nlohmann/json.hpp>

namespace fs = std::filesystem;
using nlohmann::json;

namespace DE {

// FilenameLanguage in soh/SaveManager.h: 0 = PAL table, 1 = NTSC JPN, 2 = NTSC ENG.
std::string DecodePlayerName(const std::vector<int>& chars, int filenameLanguage) {
    std::string name;
    for (int c : chars) {
        char out = '?';
        if (c >= 0x00 && c <= 0x09) {
            out = static_cast<char>('0' + c);
        } else if (filenameLanguage == 0) {
            if (c >= 0x0A && c <= 0x23) {
                out = static_cast<char>('A' + (c - 0x0A));
            } else if (c >= 0x24 && c <= 0x3D) {
                out = static_cast<char>('a' + (c - 0x24));
            } else if (c == 0x3E) {
                out = ' ';
            } else if (c == 0x3F) {
                out = '-';
            } else if (c == 0x40) {
                out = '.';
            }
        } else {
            if (c >= 0xAB && c <= 0xC4) {
                out = static_cast<char>('A' + (c - 0xAB));
            } else if (c >= 0xC5 && c <= 0xDE) {
                out = static_cast<char>('a' + (c - 0xC5));
            } else if (c == 0xDF) {
                out = ' ';
            } else if (c == 0xE4) {
                out = '-';
            } else if (c == 0xEA) {
                out = '.';
            }
        }
        name.push_back(out);
    }
    while (!name.empty() && name.back() == ' ') {
        name.pop_back();
    }
    return name;
}

static bool IsInt(const json& object, const char* key) {
    return object.contains(key) && object[key].is_number_integer();
}

SaveInfo InspectSave(const fs::path& path) {
    SaveInfo info;
    std::error_code ec;
    if (!fs::is_regular_file(path, ec)) {
        info.problem = "arquivo não encontrado";
        return info;
    }
    info.exists = true;
    info.sizeBytes = fs::file_size(path, ec);

    std::string text;
    if (!ReadWholeFile(path, text)) {
        info.problem = "não foi possível ler o arquivo";
        return info;
    }
    json doc = json::parse(text, nullptr, false);
    if (doc.is_discarded() || !doc.is_object()) {
        info.problem = "arquivo corrompido (não é um save legível)";
        return info;
    }
    if (!IsInt(doc, "version")) {
        info.problem = "save sem número de versão";
        return info;
    }
    if (doc["version"].get<int>() != 1) {
        info.problem = "formato de save desconhecido (versão " + std::to_string(doc["version"].get<int>()) + ")";
        return info;
    }
    if (!doc.contains("sections") || !doc["sections"].is_object() || !doc["sections"].contains("base") ||
        !doc["sections"]["base"].is_object()) {
        info.problem = "save sem os dados principais da partida";
        return info;
    }
    const json& base = doc["sections"]["base"];
    if (!IsInt(base, "version") || !base.contains("data") || !base["data"].is_object()) {
        info.problem = "dados principais da partida incompletos";
        return info;
    }
    info.baseVersion = base["version"].get<int>();
    if (info.baseVersion < 1 || info.baseVersion > kMaxBaseSectionVersion) {
        info.problem = "save feito por uma versão mais nova do jogo (dados v" + std::to_string(info.baseVersion) + ")";
        return info;
    }
    const json& data = base["data"];
    if (!data.contains("playerName") || !data["playerName"].is_array() || !IsInt(data, "healthCapacity")) {
        info.problem = "save sem nome do jogador ou corações";
        return info;
    }

    std::vector<int> chars;
    for (const json& c : data["playerName"]) {
        chars.push_back(c.is_number_integer() ? c.get<int>() : -1);
    }
    const int language = IsInt(data, "filenameLanguage") ? data["filenameLanguage"].get<int>() : 2;
    info.playerName = DecodePlayerName(chars, language);
    info.hearts = data["healthCapacity"].get<int>() / 16;
    info.deaths = IsInt(data, "deaths") ? data["deaths"].get<int>() : 0;
    info.rupees = IsInt(data, "rupees") ? data["rupees"].get<int>() : 0;
    if (data.contains("inventory") && data["inventory"].is_object() && IsInt(data["inventory"], "gsTokens")) {
        info.skulltulas = data["inventory"]["gsTokens"].get<int>();
    }
    info.randomizer = (data.contains("n64ddFlag") && data["n64ddFlag"].is_boolean() && data["n64ddFlag"].get<bool>()) ||
                      doc["sections"].contains("randomizer");
    info.masterQuest =
        data.contains("isMasterQuest") && data["isMasterQuest"].is_boolean() && data["isMasterQuest"].get<bool>();
    info.valid = true;
    return info;
}

const char* BackupKindId(BackupKind kind) {
    switch (kind) {
        case BackupKind::Auto:
            return "auto";
        case BackupKind::Manual:
            return "manual";
        case BackupKind::BeforeRestore:
            return "antes-restaurar";
        case BackupKind::BeforeImport:
            return "antes-importar";
        case BackupKind::BeforeDelete:
            return "antes-apagar";
        case BackupKind::BeforeCopy:
            return "antes-copiar";
    }
    return "manual";
}

const char* BackupKindLabel(const std::string& kind) {
    // "auto-2" = second automatic backup in the same millisecond: same label.
    std::string id = kind;
    const size_t dash = id.find_last_of('-');
    if (dash != std::string::npos && dash + 1 < id.size() &&
        std::all_of(id.begin() + dash + 1, id.end(), [](char c) { return c >= '0' && c <= '9'; })) {
        id = id.substr(0, dash);
    }
    if (id == "auto") {
        return "Automático";
    }
    if (id == "manual") {
        return "Manual";
    }
    if (id == "antes-restaurar") {
        return "Antes de restaurar";
    }
    if (id == "antes-importar") {
        return "Antes de importar";
    }
    if (id == "antes-apagar") {
        return "Antes de apagar";
    }
    if (id == "antes-copiar") {
        return "Antes de copiar por cima";
    }
    return "Backup";
}

SaveVault::SaveVault(fs::path saveDir, fs::path dataDir) : mSaveDir(std::move(saveDir)), mDataDir(std::move(dataDir)) {
}

fs::path SaveVault::SlotPath(int slot) const {
    return mSaveDir / ("file" + std::to_string(slot + 1) + ".sav");
}

fs::path SaveVault::BackupDir(int slot) const {
    return mDataDir / "backups" / ("file" + std::to_string(slot + 1));
}

fs::path SaveVault::ExportDir() const {
    return mDataDir / "exports";
}

static bool ParseBackupName(const std::string& name, int slot, BackupEntry& entry) {
    // file<N>_<unix ms>_<kind>.sav
    const std::string prefix = "file" + std::to_string(slot + 1) + "_";
    if (name.rfind(prefix, 0) != 0 || name.size() < prefix.size() + 6 || name.substr(name.size() - 4) != ".sav") {
        return false;
    }
    const std::string rest = name.substr(prefix.size(), name.size() - prefix.size() - 4);
    const size_t sep = rest.find('_');
    if (sep == std::string::npos || sep == 0) {
        return false;
    }
    const std::string number = rest.substr(0, sep);
    if (!std::all_of(number.begin(), number.end(), [](char c) { return c >= '0' && c <= '9'; })) {
        return false;
    }
    entry.slot = slot;
    if (number.size() > 15) {
        return false;
    }
    entry.timeMs = std::stoll(number);
    entry.time = entry.timeMs / 1000;
    entry.kind = rest.substr(sep + 1);
    return true;
}

std::vector<BackupEntry> SaveVault::ListBackups(int slot) const {
    std::lock_guard<std::recursive_mutex> lock(mMutex);
    std::vector<BackupEntry> entries;
    std::error_code ec;
    for (fs::directory_iterator it(BackupDir(slot), ec), end; !ec && it != end; it.increment(ec)) {
        BackupEntry entry;
        if (it->is_regular_file(ec) && ParseBackupName(it->path().filename().string(), slot, entry)) {
            entry.path = it->path();
            entries.push_back(entry);
        }
    }
    std::sort(entries.begin(), entries.end(), [](const BackupEntry& a, const BackupEntry& b) {
        return a.timeMs != b.timeMs ? a.timeMs > b.timeMs
                                    : a.path.filename().string() > b.path.filename().string();
    });
    return entries;
}

bool SaveVault::CreateBackup(int slot, BackupKind kind, std::string* error, fs::path* created) {
    std::lock_guard<std::recursive_mutex> lock(mMutex);
    const fs::path source = SlotPath(slot);
    std::error_code ec;
    if (!fs::is_regular_file(source, ec)) {
        if (error != nullptr) {
            *error = "o slot " + std::to_string(slot + 1) + " está vazio";
        }
        return false;
    }
    fs::create_directories(BackupDir(slot), ec);
    // The time in ms keeps names sortable; a counter avoids two backups in the same ms overwriting each other.
    int64_t now = NowUnixMillis();
    for (const BackupEntry& newest : ListBackups(slot)) {
        now = std::max(now, newest.timeMs + 1); // a clock set back never puts a new backup behind older ones
        break;
    }
    fs::path target;
    for (int n = 0;; n++) {
        const std::string kindId = std::string(BackupKindId(kind)) + (n == 0 ? "" : "-" + std::to_string(n + 1));
        target = BackupDir(slot) / ("file" + std::to_string(slot + 1) + "_" + std::to_string(now) + "_" + kindId +
                                    ".sav");
        if (!fs::exists(target, ec)) {
            break;
        }
    }
    if (!AtomicCopyFile(source, target, error)) {
        return false;
    }
    if (created != nullptr) {
        *created = target;
    }
    // Safety backups are capped too, so the folder cannot grow without end; the newest ones are kept.
    if (kind != BackupKind::Auto) {
        PruneBackups(slot, BackupKindId(kind), 20);
    }
    return true;
}

void SaveVault::PruneBackups(int slot, const std::string& kind, int keep) {
    std::lock_guard<std::recursive_mutex> lock(mMutex);
    int kept = 0;
    std::error_code ec;
    for (const BackupEntry& entry : ListBackups(slot)) {
        // "auto-2" (same second) counts as "auto".
        if (entry.kind != kind && entry.kind.rfind(kind + "-", 0) != 0) {
            continue;
        }
        if (++kept > keep) {
            fs::remove(entry.path, ec);
        }
    }
}

bool SaveVault::AutoBackupBeforeOverwrite(int slot, int64_t minIntervalSeconds, int keep) {
    std::lock_guard<std::recursive_mutex> lock(mMutex);
    if (keep <= 0 || !InspectSave(SlotPath(slot)).valid) {
        return false; // nothing worth keeping (new file, or already broken: the load path keeps broken ones)
    }
    const int64_t now = NowUnixMillis();
    for (const BackupEntry& entry : ListBackups(slot)) {
        if (entry.kind.rfind("auto", 0) == 0) {
            if (now - entry.timeMs < minIntervalSeconds * 1000 && entry.timeMs <= now) {
                return false;
            }
            break;
        }
    }
    if (!CreateBackup(slot, BackupKind::Auto)) {
        return false;
    }
    PruneBackups(slot, "auto", keep);
    return true;
}

bool SaveVault::DeleteBackup(const BackupEntry& entry, std::string* error) {
    std::lock_guard<std::recursive_mutex> lock(mMutex);
    std::error_code ec;
    // Only files inside this vault's backup folders can be deleted through it.
    if (entry.path.parent_path() != BackupDir(entry.slot)) {
        if (error != nullptr) {
            *error = "arquivo fora da pasta de backups";
        }
        return false;
    }
    if (!fs::remove(entry.path, ec)) {
        if (error != nullptr) {
            *error = ec ? ec.message() : "arquivo não encontrado";
        }
        return false;
    }
    return true;
}

bool SaveVault::RestoreInto(int slot, const fs::path& source, BackupKind safetyKind, std::string* error) {
    std::lock_guard<std::recursive_mutex> lock(mMutex);
    auto fail = [&](const std::string& message) {
        if (error != nullptr) {
            *error = message;
        }
        return false;
    };

    // 1. The file must be a save the game can load.
    const SaveInfo sourceInfo = InspectSave(source);
    if (!sourceInfo.valid) {
        return fail("o arquivo escolhido não é um save válido: " + sourceInfo.problem);
    }

    // 2. Keep what is in the slot now, valid or not, so this step can always be undone.
    const fs::path target = SlotPath(slot);
    std::error_code ec;
    if (fs::exists(target, ec)) {
        std::string backupError;
        if (!CreateBackup(slot, safetyKind, &backupError)) {
            return fail("não foi possível guardar o save atual antes de substituir (" + backupError +
                        "); nada foi alterado");
        }
    }

    // 3. Complete copy next to the save, checked again, then 4. atomic rename over it.
    fs::create_directories(mSaveDir, ec);
    const fs::path temp = mSaveDir / ("file" + std::to_string(slot + 1) + ".restaurando");
    std::string copyError;
    if (!AtomicCopyFile(source, temp, &copyError)) {
        fs::remove(temp, ec);
        return fail("falha ao copiar: " + copyError + "; o save atual não foi alterado");
    }
    if (!InspectSave(temp).valid) {
        fs::remove(temp, ec);
        return fail("a cópia ficou diferente do original; o save atual não foi alterado");
    }
    if (!AtomicReplace(temp, target, &copyError)) {
        fs::remove(temp, ec);
        return fail(copyError);
    }
    return true;
}

static std::string SafeFilePart(const std::string& text) {
    std::string out;
    for (char c : text) {
        out.push_back((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') ? c : '-');
    }
    return out.empty() ? "Link" : out;
}

bool SaveVault::Export(int slot, std::string* error, fs::path* created) {
    std::lock_guard<std::recursive_mutex> lock(mMutex);
    const SaveInfo info = InspectSave(SlotPath(slot));
    if (!info.valid) {
        if (error != nullptr) {
            *error = info.exists ? "o save do slot não é válido: " + info.problem : "o slot está vazio";
        }
        return false;
    }
    const fs::path target = ExportDir() / ("OOTDE_slot" + std::to_string(slot + 1) + "_" +
                                           SafeFilePart(info.playerName) + "_" +
                                           TimestampForFileName(NowUnixSeconds()) + ".sav");
    if (!AtomicCopyFile(SlotPath(slot), target, error)) {
        return false;
    }
    if (created != nullptr) {
        *created = target;
    }
    return true;
}

std::vector<fs::path> SaveVault::ListImportable() const {
    std::lock_guard<std::recursive_mutex> lock(mMutex);
    std::vector<fs::path> files;
    std::error_code ec;
    for (fs::directory_iterator it(ExportDir(), ec), end; !ec && it != end; it.increment(ec)) {
        if (it->is_regular_file(ec) && it->path().extension() == ".sav") {
            files.push_back(it->path());
        }
    }
    std::sort(files.begin(), files.end(), [](const fs::path& a, const fs::path& b) {
        return a.filename().string() > b.filename().string();
    });
    return files;
}

} // namespace DE
