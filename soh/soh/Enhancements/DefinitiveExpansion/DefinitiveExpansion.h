#pragma once
// OOT D.E. Definitive Expansion: entry points used by the game (SaveManager) and by the port menu (SohMenuDefinitive).
// The expansion's own data lives in <app folder>/ootde/ (see OOTDEConfig.h and DESaveManager.h).
#ifdef __cplusplus
#include "OOTDEConfig.h"
#include "DESaveManager.h"

namespace DefinitiveExpansion {

// Both are created on first use (thread safe: saves are written on a worker thread).
DE::Config& Config();
DE::SaveVault& Vault();

// Writes the settings file; shows nothing on failure, the reason is logged.
void SaveConfig();

// Called by SaveManager right before a save file is replaced by a newer one (worker thread).
void BeforeSaveOverwrite(int fileNum);
// Called by SaveManager before file select deletes a save or copies another one over it (main thread).
void BeforeSlotDeleted(int fileNum);
void BeforeSlotCopiedOver(int fileNum);

// Menu pages (port menu > Definitive).
void DrawSaveManager();
void DrawFeatures();
void DrawTouchAndMotion();
void DrawHudEditor();
void DrawAchievements();
void DrawBossRush();
void DrawMasterMode();

// Settings of each phase are registered before the settings file is read.
void RegisterControlSettings(DE::Config& config);
void RegisterAchievementSettings(DE::Config& config);
void RegisterChallengeSettings(DE::Config& config);

// <app folder>/ootde: where every file of the expansion lives.
std::filesystem::path DataDir();

// Unlocks an achievement that is tied to an event (see DEAchievements.cpp); does nothing if it is already unlocked.
void ReportAchievement(const char* id);
// Marks a scene as visited in the statistics of the save being played (used by the explorer and by achievements).
void MarkPlaceVisited(int sceneNum);

} // namespace DefinitiveExpansion
#endif
