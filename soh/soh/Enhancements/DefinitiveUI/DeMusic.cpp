// OOT D.E.: music of the Definitive Edition main menu. See DeMusic.h.
#include "DeMusic.h"

#include <libultraship/libultraship.h>
#include <algorithm>
#include <atomic>
#include <cmath>
#include <cstdio>
#include <fstream>
#include <mutex>
#include <string>
#include <vector>

#include "soh/cvar_prefixes.h"

#define STB_VORBIS_HEADER_ONLY
#include "stb_vorbis.c"

namespace DefinitiveUI {

static const char* const kBundledTrack = "ootde/audio/gDEMenuTheme.ogg";
static const char* const kModsTrack = "ootde-menu.ogg";
static constexpr int kOutputRate = 44100;

// Shared between the game thread (load, play/stop) and the audio thread (mix).
static std::mutex sMutex;
static std::vector<unsigned char> sData; // the whole .ogg file
static stb_vorbis* sVorbis = nullptr;
static int sChannels = 0;
static int sRate = 0;
static std::atomic<bool> sWanted{ false };
static std::atomic<float> sVolume{ 0.8f };
static bool sTried = false;

// Audio thread only.
static float sLevel = 0.0f;         // current fade level (0..1)
static bool sRewind = false;        // start from the beginning the next time it fades in
static short sBlock[4096 * 2];      // decoded frames (stereo)
static int sBlockFrames = 0;
static double sBlockPos = 0.0;      // read position inside sBlock, in source frames

static bool ReadFile(const std::string& path, std::vector<unsigned char>& out) {
    std::ifstream f(path, std::ios::binary);
    if (!f) {
        return false;
    }
    out.assign(std::istreambuf_iterator<char>(f), std::istreambuf_iterator<char>());
    return !out.empty();
}

void MusicLoad() {
    if (sTried) {
        return;
    }
    sTried = true;
    std::vector<unsigned char> data;
    // A track of the player's own in the mods folder wins over the one that comes with the game.
    const std::string mods = Ship::Context::LocateFileAcrossAppDirs("mods", Ship::Context::GetInstance()->GetShortName());
    std::string from = mods + "/" + kModsTrack;
    if (!ReadFile(from, data)) {
        from = kBundledTrack;
        auto file = Ship::Context::GetInstance()->GetResourceManager()->GetArchiveManager()->LoadFile(kBundledTrack);
        if (file != nullptr && file->Buffer != nullptr && !file->Buffer->empty()) {
            data.assign(file->Buffer->begin(), file->Buffer->end());
        }
    }
    if (data.empty()) {
        SPDLOG_INFO("OOT D.E.: no menu music ({} or {})", kModsTrack, kBundledTrack);
        return;
    }
    int error = 0;
    std::lock_guard<std::mutex> lock(sMutex);
    sData = std::move(data);
    sVorbis = stb_vorbis_open_memory(sData.data(), (int)sData.size(), &error, nullptr);
    if (sVorbis == nullptr) {
        SPDLOG_ERROR("OOT D.E.: could not read the menu music {} (error {})", from, error);
        sData.clear();
        return;
    }
    const stb_vorbis_info info = stb_vorbis_get_info(sVorbis);
    sChannels = info.channels;
    sRate = (int)info.sample_rate;
    sBlockFrames = 0;
    sBlockPos = 0.0;
    SPDLOG_INFO("OOT D.E.: menu music {} ({} Hz, {} channels)", from, sRate, sChannels);
}

bool MusicAvailable() {
    std::lock_guard<std::mutex> lock(sMutex);
    return sVorbis != nullptr;
}

void MusicSetPlaying(bool playing) {
    const int percent = CVarGetInteger(CVAR_SETTING("DefinitiveUI.MenuMusicVolume"), 0);
    sWanted = playing && percent > 0;
    // Like the game's own music, it follows the master volume.
    const int master = CVarGetInteger(CVAR_SETTING("Volume.Master"), 40);
    sVolume = std::min(100, std::max(0, percent)) / 100.0f * std::min(100, std::max(0, master)) / 100.0f;
}

// Decodes the next block of the track, going back to the start at its end.
static bool NextBlock() {
    for (int attempt = 0; attempt < 2; attempt++) {
        sBlockFrames = stb_vorbis_get_samples_short_interleaved(sVorbis, 2, sBlock, 4096 * 2);
        if (sBlockFrames > 0) {
            return true;
        }
        stb_vorbis_seek_start(sVorbis);
    }
    return false;
}

} // namespace DefinitiveUI

using namespace DefinitiveUI;

extern "C" void DefinitiveUI_MusicMix(int16_t* buf, uint32_t frames) {
    std::unique_lock<std::mutex> lock(sMutex, std::try_to_lock);
    if (!lock.owns_lock() || sVorbis == nullptr) {
        return;
    }
    const bool wanted = sWanted;
    if (!wanted && sLevel <= 0.0f) {
        sRewind = true;
        return;
    }
    if (wanted && sLevel <= 0.0f && sRewind) {
        stb_vorbis_seek_start(sVorbis);
        sBlockFrames = 0;
        sBlockPos = 0.0;
        sRewind = false;
    }
    const float volume = sVolume;
    const double step = (double)sRate / kOutputRate;
    const float fade = 1.0f / (kOutputRate * (wanted ? 1.5f : 1.0f)); // 1.5 s in, 1 s out
    for (uint32_t i = 0; i < frames; i++) {
        sLevel = wanted ? std::min(1.0f, sLevel + fade) : std::max(0.0f, sLevel - fade);
        while ((int)sBlockPos + 1 >= sBlockFrames) {
            // The last frame of a block is only blended with the one before it (inaudible).
            sBlockPos = std::max(0.0, sBlockPos - std::max(0, sBlockFrames - 1));
            if (!NextBlock()) {
                return;
            }
        }
        const int k = (int)sBlockPos;
        const float t = (float)(sBlockPos - k);
        const float l = sBlock[k * 2] + (sBlock[k * 2 + 2] - sBlock[k * 2]) * t;
        const float r = sBlock[k * 2 + 1] + (sBlock[k * 2 + 3] - sBlock[k * 2 + 1]) * t;
        const float g = volume * sLevel * sLevel; // eased fade
        const float outL = buf[i * 2] + l * g;
        const float outR = buf[i * 2 + 1] + r * g;
        buf[i * 2] = (int16_t)std::max(-32768.0f, std::min(32767.0f, outL));
        buf[i * 2 + 1] = (int16_t)std::max(-32768.0f, std::min(32767.0f, outR));
        sBlockPos += step;
    }
}
