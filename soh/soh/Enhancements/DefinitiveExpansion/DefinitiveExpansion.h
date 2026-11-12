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

// Settings of each phase are registered before the settings file is read.
void RegisterControlSettings(DE::Config& config);

} // namespace DefinitiveExpansion
#endif
