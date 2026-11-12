#pragma once

// OOT D.E.: entry points of the Definitive Edition interface for the game's C code.
//
// Main menu (screen 1): drawn over the title screen, which keeps running behind it. The title logo actor (EnMag)
// hands its input to the menu once the logo is shown; the menu chooses what happens and the game does it through its
// own code paths: the file select screen (z_file_choose.c) starts or loads a file exactly as when the player does it
// there, and the pause menu (z_kaleido_setup.c) opens the Diário de Quests exactly as the Start button does.

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

struct PlayState;

// The menu (drawn over the whole title screen, with the art of the reference image) replaces "PRESS START" (Configurações > Interface; on by default).
bool DefinitiveUI_TitleMenuEnabled(void);
// Called by EnMag every frame once the logo is shown. Returns true when the game must go to the file select screen.
bool DefinitiveUI_TitleMenuUpdate(struct PlayState* play);
// The title screen is gone (EnMag destroyed).
void DefinitiveUI_TitleMenuClose(void);
// The menu is on screen (the touch controls are hidden meanwhile).
bool DefinitiveUI_TitleMenuVisible(void);

// File select: what the main menu asked for. Returns the file (0-2) to act on and sets *newGame, or -1 for nothing.
int32_t DefinitiveUI_TakeFileRequest(bool* newGame);
// File select: confirm the chosen file without asking (Continuar).
bool DefinitiveUI_TakeAutoConfirm(void);
// Pause menu: open the Diário de Quests now (asked from the main menu; waits until the game can pause).
bool DefinitiveUI_TakeJournalRequest(void);

#ifdef __cplusplus
}

namespace DefinitiveUI {
void LoadFonts();
void RegisterWindow();
void UnregisterWindow();
} // namespace DefinitiveUI
#endif
