// OOT D.E. Definitive Expansion: host tests of the expansion core. Run with scripts/tests/run_de_expansion_tests.sh.
#include "DEAchievementsCore.h"
#include "DERecords.h"
#include "DESaveManager.h"
#include "DEStorage.h"
#include "OOTDEConfig.h"

#include <cstdio>
#include <filesystem>
#include <string>
#include <nlohmann/json.hpp>

namespace fs = std::filesystem;
using nlohmann::json;

static int sFailures = 0;
static int sChecks = 0;
#define CHECK(cond)                                                                \
    do {                                                                           \
        sChecks++;                                                                 \
        if (!(cond)) {                                                             \
            sFailures++;                                                           \
            std::printf("FALHOU %s:%d: %s\n", __FILE__, __LINE__, #cond);          \
        }                                                                          \
    } while (0)

static std::string Read(const fs::path& p) {
    std::string s;
    DE::ReadWholeFile(p, s);
    return s;
}

static void Write(const fs::path& p, const std::string& s) {
    fs::create_directories(p.parent_path());
    FILE* f = std::fopen(p.string().c_str(), "wb");
    std::fwrite(s.data(), 1, s.size(), f);
    std::fclose(f);
}

// A save shaped like the ones SaveManager::SaveFileThreaded writes ("LINK" in the NTSC table).
static std::string MakeSave(int healthCapacity, int deaths, bool rando = false, int baseVersion = 4) {
    json data = { { "playerName", { 0xB6, 0xB3, 0xB8, 0xB5, 0xDF, 0xDF, 0xDF, 0xDF } },
                  { "healthCapacity", healthCapacity },
                  { "deaths", deaths },
                  { "rupees", 99 },
                  { "n64ddFlag", rando },
                  { "isMasterQuest", false },
                  { "filenameLanguage", 2 },
                  { "inventory", { { "gsTokens", 12 } } } };
    json doc = { { "version", 1 }, { "sections", { { "base", { { "version", baseVersion }, { "data", data } } } } } };
    return doc.dump(4);
}

static void TestStorage(const fs::path& dir) {
    const fs::path file = dir / "storage" / "a.json";
    CHECK(DE::AtomicWriteFile(file, "um"));
    CHECK(Read(file) == "um");
    CHECK(DE::AtomicWriteFile(file, "dois"));
    CHECK(Read(file) == "dois");
    // No temporary file is left behind.
    int count = 0;
    for (auto& e : fs::directory_iterator(file.parent_path())) {
        (void)e;
        count++;
    }
    CHECK(count == 1);
    CHECK(DE::AtomicCopyFile(file, dir / "storage" / "b.json"));
    CHECK(Read(dir / "storage" / "b.json") == "dois");
    CHECK(!DE::AtomicCopyFile(dir / "storage" / "nao-existe", dir / "storage" / "c.json"));
    CHECK(!fs::exists(dir / "storage" / "c.json"));
    CHECK(DE::TimestampForFileName(0).size() == 15);
}

static void RegisterTestSettings(DE::Config& c) {
    c.RegisterSetting("saves.autoBackup", true, "");
    c.RegisterSetting("saves.keep", 10, "");
    c.RegisterSetting("touch.opacity", 0.8f, "");
    c.RegisterSetting("touch.layout", "classico", "");
}

static void TestConfig(const fs::path& dir) {
    const fs::path path = dir / "config" / "config.json";
    {
        DE::Config c;
        RegisterTestSettings(c);
        CHECK(c.Load(path) == DE::Config::LoadResult::CreatedNew);
        CHECK(c.GetBool("saves.autoBackup") == true);
        CHECK(c.GetInt("saves.keep") == 10);
        CHECK(c.IsDefault("saves.keep"));
        CHECK(c.Set("saves.keep", 5));
        CHECK(!c.Set("saves.keep", "cinco")); // wrong type
        CHECK(!c.Set("saves.keep", 2.5));     // an int setting stays an int
        CHECK(c.Set("touch.opacity", 1));     // a float setting accepts an int
        CHECK(!c.Set("nao.existe", 1));
        CHECK(c.SaveProfile("meu", "touch."));
        CHECK(c.Set("touch.opacity", 0.3f));
        CHECK(c.Set("touch.layout", "minimo"));
        CHECK(c.Save());
    }
    {
        DE::Config c;
        RegisterTestSettings(c);
        CHECK(c.Load(path) == DE::Config::LoadResult::Loaded);
        CHECK(c.GetInt("saves.keep") == 5);
        CHECK(c.GetString("touch.layout") == "minimo");
        CHECK(c.ListProfiles().size() == 1);
        CHECK(c.ApplyProfile("meu"));
        CHECK(c.GetFloat("touch.opacity") == 1.0f);
        CHECK(c.GetString("touch.layout") == "classico");
        CHECK(c.GetInt("saves.keep") == 5); // outside the profile's prefix: untouched
        c.ResetToDefaults("saves.");
        CHECK(c.GetInt("saves.keep") == 10);
        CHECK(c.GetFloat("touch.opacity") == 1.0f);
        c.ResetToDefaults();
        CHECK(c.IsDefault("touch.opacity"));
        CHECK(c.ListProfiles().size() == 1); // profiles survive a reset
        CHECK(c.DeleteProfile("meu"));
        CHECK(c.ListProfiles().empty());
    }
    // Broken file: kept aside, defaults used.
    {
        Write(path, "{ isto não é json");
        DE::Config c;
        RegisterTestSettings(c);
        CHECK(c.Load(path) == DE::Config::LoadResult::RecoveredBroken);
        CHECK(!fs::exists(path));
        bool kept = false;
        for (auto& e : fs::directory_iterator(path.parent_path())) {
            kept |= e.path().filename().string().find("config.json.corrompido-") == 0;
        }
        CHECK(kept);
        CHECK(c.GetInt("saves.keep") == 10);
    }
    // Version 0 (bare object) migrates to 1, the original is kept.
    {
        Write(path, R"({ "saves.keep": 3 })");
        DE::Config c;
        RegisterTestSettings(c);
        CHECK(c.Load(path) == DE::Config::LoadResult::Migrated);
        CHECK(c.GetInt("saves.keep") == 3);
        CHECK(fs::exists(path.string() + ".v0.bak"));
        CHECK(c.Save());
        CHECK(json::parse(Read(path))["schemaVersion"] == DE::Config::kSchemaVersion);
    }
    // A file from a newer app: kept as backup, known values used, unknown ones written back.
    {
        Write(path, R"({ "schemaVersion": 99, "settings": { "saves.keep": 7, "futuro.recurso": true } })");
        DE::Config c;
        RegisterTestSettings(c);
        CHECK(c.Load(path) == DE::Config::LoadResult::NewerVersion);
        CHECK(fs::exists(path.string() + ".v99.bak"));
        CHECK(c.GetInt("saves.keep") == 7);
        CHECK(c.Save());
        CHECK(json::parse(Read(path))["settings"]["futuro.recurso"] == true);
    }
}

static void TestSaves(const fs::path& dir) {
    const fs::path saveDir = dir / "app" / "Save";
    const fs::path dataDir = dir / "app" / "ootde";
    DE::SaveVault vault(saveDir, dataDir);

    // Inspect.
    Write(vault.SlotPath(0), MakeSave(5 * 16, 2));
    DE::SaveInfo info = DE::InspectSave(vault.SlotPath(0));
    CHECK(info.valid);
    CHECK(info.playerName == "LINK");
    CHECK(info.hearts == 5);
    CHECK(info.deaths == 2);
    CHECK(info.skulltulas == 12);
    CHECK(!info.randomizer);
    CHECK(DE::InspectSave(saveDir / "file9.sav").exists == false);
    Write(dir / "lixo.sav", "\x01\x02 nada");
    CHECK(!DE::InspectSave(dir / "lixo.sav").valid);
    Write(dir / "v2.sav", R"({ "version": 2, "sections": {} })");
    CHECK(!DE::InspectSave(dir / "v2.sav").valid);
    Write(dir / "novo.sav", MakeSave(48, 0, false, DE::kMaxBaseSectionVersion + 1));
    CHECK(!DE::InspectSave(dir / "novo.sav").valid);
    Write(dir / "rando.sav", MakeSave(48, 0, true));
    CHECK(DE::InspectSave(dir / "rando.sav").randomizer);
    CHECK(DE::DecodePlayerName({ 0x15, 0x2C, 0x31, 0x2E }, 0) == "Link");

    // Backups.
    CHECK(!vault.CreateBackup(1, DE::BackupKind::Manual)); // empty slot
    CHECK(vault.CreateBackup(0, DE::BackupKind::Manual));
    CHECK(vault.CreateBackup(0, DE::BackupKind::Manual)); // same second: a second file, not an overwrite
    CHECK(vault.ListBackups(0).size() == 2);
    CHECK(vault.AutoBackupBeforeOverwrite(0, 600, 3));
    CHECK(!vault.AutoBackupBeforeOverwrite(0, 600, 3)); // too soon after the last automatic one
    CHECK(vault.AutoBackupBeforeOverwrite(0, 0, 3));
    CHECK(vault.AutoBackupBeforeOverwrite(0, 0, 3));
    CHECK(vault.AutoBackupBeforeOverwrite(0, 0, 3));
    int autos = 0;
    for (const auto& b : vault.ListBackups(0)) {
        autos += b.kind.rfind("auto", 0) == 0;
    }
    CHECK(autos == 3); // pruned to `keep`
    Write(vault.SlotPath(2), "quebrado");
    CHECK(!vault.AutoBackupBeforeOverwrite(2, 0, 3)); // a broken save is not rotated into the automatic backups

    // Restore: an invalid source changes nothing.
    const std::string before = Read(vault.SlotPath(0));
    std::string error;
    CHECK(!vault.RestoreInto(0, dir / "lixo.sav", DE::BackupKind::BeforeRestore, &error));
    CHECK(!error.empty());
    CHECK(Read(vault.SlotPath(0)) == before);

    // Restore: a valid source replaces the save, the old one is kept as a backup.
    Write(dir / "outro.sav", MakeSave(10 * 16, 30));
    const size_t backupsBefore = vault.ListBackups(0).size();
    CHECK(vault.RestoreInto(0, dir / "outro.sav", DE::BackupKind::BeforeRestore, &error));
    CHECK(DE::InspectSave(vault.SlotPath(0)).hearts == 10);
    CHECK(vault.ListBackups(0).size() == backupsBefore + 1);
    CHECK(vault.ListBackups(0).front().kind == "antes-restaurar");
    CHECK(Read(vault.ListBackups(0).front().path) == before);
    CHECK(!fs::exists(saveDir / "file1.restaurando"));

    // Restore into an empty slot needs no safety backup.
    CHECK(vault.RestoreInto(1, dir / "outro.sav", DE::BackupKind::BeforeImport, &error));
    CHECK(DE::InspectSave(vault.SlotPath(1)).valid);

    // Export / import list.
    fs::path exported;
    CHECK(vault.Export(0, &error, &exported));
    CHECK(DE::InspectSave(exported).valid);
    CHECK(exported.filename().string().find("OOTDE_slot1_LINK_") == 0);
    CHECK(vault.ListImportable().size() == 1);
    CHECK(!vault.Export(2, &error)); // broken save is not exported

    // Delete only inside the backup folder.
    DE::BackupEntry outside;
    outside.path = dir / "outro.sav";
    outside.slot = 0;
    CHECK(!vault.DeleteBackup(outside));
    CHECK(fs::exists(dir / "outro.sav"));
    const auto list = vault.ListBackups(0);
    CHECK(vault.DeleteBackup(list.back()));
    CHECK(vault.ListBackups(0).size() == list.size() - 1);
}

static void TestAchievements(const fs::path& dir) {
    const fs::path path = dir / "ach" / "achievements.json";
    std::vector<DE::AchievementDef> defs = {
        { "first", DE::AchievementCategory::Story, "Primeiro passo", "", 1, false, "" },
        { "skulls", DE::AchievementCategory::Collectibles, "Caçador", "", 100, false, "" },
        { "kills", DE::AchievementCategory::Combat, "Guerreiro", "", 3, true, "" },
    };
    {
        DE::AchievementStore store(defs);
        CHECK(store.Load(path));
        CHECK(store.Percent() == 0);
        CHECK(!store.Report("skulls", 40, 1000));
        CHECK(store.State("skulls").progress == 40);
        CHECK(!store.Report("skulls", 10, 1001)); // progress never goes down
        CHECK(store.State("skulls").progress == 40);
        CHECK(store.Report("first", 1, 2000));
        CHECK(!store.Report("first", 1, 3000)); // only unlocks once
        CHECK(store.State("first").unlockedAt == 2000);
        CHECK(!store.Report("nao-existe", 1, 1));
        CHECK(!store.Add("kills", 1, 10));
        CHECK(!store.Add("kills", 1, 11));
        CHECK(store.Add("kills", 5, 2500)); // clamps at the target and unlocks
        CHECK(store.State("kills").progress == 3);
        CHECK(store.UnlockedCount() == 2);
        CHECK(store.Percent() == 66);
        CHECK(store.History().size() == 2 && store.History().front()->id == "kills");
        CHECK(store.Dirty());
        CHECK(store.Save());
        CHECK(!store.Dirty());
    }
    {
        DE::AchievementStore store(defs);
        CHECK(store.Load(path));
        CHECK(store.State("skulls").progress == 40 && !store.State("skulls").Unlocked());
        CHECK(store.State("first").unlockedAt == 2000);
    }
    // A hand-edited file cannot unlock without progress; unknown ids are kept.
    Write(path, R"({ "schemaVersion": 1, "achievements": { "skulls": { "progress": 5, "unlockedAt": 99 },
                     "futuro": { "progress": 2, "unlockedAt": 0 } } })");
    {
        DE::AchievementStore store(defs);
        CHECK(store.Load(path));
        CHECK(!store.State("skulls").Unlocked());
        CHECK(store.Report("first", 1, 5));
        CHECK(store.Save());
        CHECK(json::parse(Read(path))["achievements"]["futuro"]["progress"] == 2);
    }
    Write(path, "quebrado");
    {
        DE::AchievementStore store(defs);
        CHECK(!store.Load(path));
        CHECK(!fs::exists(path));
        CHECK(store.Percent() == 0);
    }
}

static void TestRecords(const fs::path& dir) {
    const fs::path path = dir / "rec" / "records.json";
    CHECK(DE::FormatRunTime(0) == "0:00.0");
    CHECK(DE::FormatRunTime(83450) == "1:23.4");
    CHECK(DE::FormatRunTime(3723900) == "1:02:03.9");
    CHECK(DE::ComputeRunScore(600000, 0, 160, 160, 100) == 9000);
    CHECK(DE::ComputeRunScore(600000, 2, 80, 160, 100) == 7000);
    CHECK(DE::ComputeRunScore(600000, 0, 160, 160, 150) == 13500);
    CHECK(DE::ComputeRunScore(99999999, 50, 0, 160, 100) == 100);
    {
        DE::RecordBook book;
        CHECK(book.Load(path));
        DE::RunRecord a{ "bossrush/normal/all", 500000, 1, 100, 160, 1, 1000, "" };
        DE::RunRecord b{ "bossrush/normal/all", 400000, 0, 100, 160, 1, 2000, "" };
        DE::RunRecord c{ "bossrush/normal/all", 450000, 0, 100, 160, 1, 3000, "" };
        DE::RunRecord d{ "bossrush/dificil/all", 900000, 0, 100, 160, 1, 4000, "x" };
        CHECK(book.Add(a) == 1);
        CHECK(book.Add(b) == 1);
        CHECK(book.Add(c) == 2);
        CHECK(book.Add(d) == 1); // its own board
        CHECK(book.Ranking("bossrush/normal/all").size() == 3);
        CHECK(book.Ranking("bossrush/normal/all").front().timeMs == 400000);
        CHECK(book.History().front().date == 4000);
        CHECK(book.Boards().size() == 2);
        CHECK(book.Save());
    }
    {
        DE::RecordBook book;
        CHECK(book.Load(path));
        DE::RunRecord best;
        CHECK(book.Best("bossrush/dificil/all", best) && best.details == "x");
        CHECK(!book.Best("nada", best));
        // The history is capped but the record of a board survives.
        for (int i = 0; i < 300; i++) {
            book.Add(DE::RunRecord{ "lotado", 100000 + i, 0, 0, 0, 0, 10000 + i, "" });
        }
        CHECK(book.History().size() == DE::RecordBook::kMaxHistory);
        CHECK(book.Best("lotado", best) && best.timeMs == 100000);
        CHECK(book.Best("bossrush/normal/all", best) && best.timeMs == 400000);
    }
    Write(path, "[]");
    {
        DE::RecordBook book;
        CHECK(!book.Load(path));
        CHECK(book.History().empty());
    }
}

int main(int argc, char** argv) {
    const fs::path dir = argc > 1 ? fs::path(argv[1]) : fs::temp_directory_path() / "de_expansion_test";
    fs::remove_all(dir);
    fs::create_directories(dir);
    TestStorage(dir);
    TestConfig(dir);
    TestSaves(dir);
    TestAchievements(dir);
    TestRecords(dir);
    std::printf("%d verificações, %d falhas\n", sChecks, sFailures);
    return sFailures == 0 ? 0 : 1;
}
