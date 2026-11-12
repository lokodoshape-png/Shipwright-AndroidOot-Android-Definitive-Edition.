#pragma once

// OOT D.E.: music of the Definitive Edition main menu.
//
// An Ogg Vorbis track mixed over the game's own sound while the menu is on the title screen (the title music of
// the game is muted meanwhile and comes back when the title screen is left). The track is mods/ootde-menu.ogg when
// the player puts one there, otherwise the one that comes with OOT D.E. (ootde/audio/gDEMenuTheme.ogg in soh.otr).
// It loops, fades in and out, and its volume is Configurações > Interface > Música do menu.

#include <stdint.h>

#ifdef __cplusplus
namespace DefinitiveUI {
// Game thread.
void MusicLoad();
bool MusicAvailable();
void MusicSetPlaying(bool playing);
} // namespace DefinitiveUI
extern "C" {
#endif

// Audio thread: adds the music to `frames` stereo frames at 44100 Hz.
void DefinitiveUI_MusicMix(int16_t* buf, uint32_t frames);

#ifdef __cplusplus
}
#endif
