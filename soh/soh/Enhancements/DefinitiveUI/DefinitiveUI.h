#pragma once

// OOT D.E.: entry points of the Definitive Edition interface for the game's C code.
//
// Main menu: takes the place of the game's title screen ("PRESS START") and of its file select screen. It is drawn
// over the title screen, which keeps running behind it; the title logo actor (EnMag) hands its input to the menu. The
// menu has its own screens for the files (play, new game with the hero's name, copy, erase), and the game does the
// work through its own code: copy and erase with the save manager, and creating or loading a file in the file select
// screen (z_file_choose.c), which runs covered by a "loading" screen and is never seen. The pause menu
// (z_kaleido_setup.c) opens the Diário de Quests exactly as the Start button does. When the game opens, the
// Definitive Edition logo is shown before the menu.

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

// The menu holds the title screen: its opening does not go on to the next scenes (z_demo.c).
bool DefinitiveUI_TitleMenuHoldsTitle(void);

// File select is working for the menu under the "loading" screen: it takes no input and plays no music.
bool DefinitiveUI_CoverActive(void);

// File select works underneath the menu, which already asked everything:
// - a new file to create: returns the file (0-2), its name in the game's letter codes (8 bytes) and the quest; -1 for
//   none;
int32_t DefinitiveUI_TakeNewFileRequest(uint8_t* name, int32_t* quest);
// - the new file exists: play it at the next update;
void DefinitiveUI_NewFileCreated(int32_t file);
// - a file to play: returns the file (0-2) or -1 for nothing (*newGame is always false);
int32_t DefinitiveUI_TakeFileRequest(bool* newGame);
// - what was asked cannot be done (or nothing was asked): file select shows itself and takes over.
void DefinitiveUI_CancelFileRequest(void);
// File select: confirm the chosen file without asking.
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
