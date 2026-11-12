// OOT D.E.: renders the Definitive Edition interface outside the game, with the same drawing code the game uses
// (soh/soh/Enhancements/DefinitiveUI), to check the screens on a computer without a ROM, GPU or phone.
//
// Dear ImGui builds the draw lists exactly as in the game; a small software rasterizer below turns them into an
// image, with the same textures (soh/assets/custom/ootde/ui) and fonts. See build.sh.
#include <imgui.h>
#include <imgui_internal.h>

#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

#include "DeTheme.h"
#include "DeTitleMenuText.h"
#include "DeTitleMenuView.h"

using namespace DefinitiveUI;

struct Image {
    int w = 0, h = 0;
    std::vector<unsigned char> px; // RGBA
};

// Raw RGBA file: "w h\n" header followed by w*h*4 bytes (written by build.sh).
static bool LoadRaw(const std::string& path, Image& img) {
    FILE* f = fopen(path.c_str(), "rb");
    if (f == nullptr) {
        return false;
    }
    if (fscanf(f, "%d %d", &img.w, &img.h) != 2) {
        fclose(f);
        return false;
    }
    fgetc(f);
    img.px.resize((size_t)img.w * img.h * 4);
    const size_t n = fread(img.px.data(), 1, img.px.size(), f);
    fclose(f);
    return n == img.px.size();
}

static std::vector<Image> sTextures; // id = index + 1

static ImTextureID AddTexture(Image img) {
    sTextures.push_back(std::move(img));
    return (ImTextureID)(intptr_t)sTextures.size();
}

static ImTextureID sThemeTex[TEX_COUNT];
static ImTextureID LookupTex(Texture t) {
    return sThemeTex[t];
}

// ---- Software rasterizer for ImDrawData (bilinear textures, straight alpha blending, clip rects).

struct Canvas {
    int w, h;
    std::vector<float> rgb;
};

static void Sample(const Image& img, float u, float v, float out[4]) {
    // Clamp to edge, bilinear.
    float x = u * img.w - 0.5f;
    float y = v * img.h - 0.5f;
    int x0 = (int)floorf(x), y0 = (int)floorf(y);
    float fx = x - x0, fy = y - y0;
    auto px = [&](int xx, int yy, int c) {
        xx = xx < 0 ? 0 : (xx >= img.w ? img.w - 1 : xx);
        yy = yy < 0 ? 0 : (yy >= img.h ? img.h - 1 : yy);
        return img.px[((size_t)yy * img.w + xx) * 4 + c] / 255.0f;
    };
    for (int c = 0; c < 4; c++) {
        const float a = px(x0, y0, c) * (1 - fx) + px(x0 + 1, y0, c) * fx;
        const float b = px(x0, y0 + 1, c) * (1 - fx) + px(x0 + 1, y0 + 1, c) * fx;
        out[c] = a * (1 - fy) + b * fy;
    }
}

static void RasterTri(Canvas& cv, const ImDrawVert& A, const ImDrawVert& B, const ImDrawVert& C, const Image* tex,
                      const ImVec4& clip) {
    const float minx = std::fmax(clip.x, std::fmin(A.pos.x, std::fmin(B.pos.x, C.pos.x)));
    const float maxx = std::fmin(clip.z, std::fmax(A.pos.x, std::fmax(B.pos.x, C.pos.x)));
    const float miny = std::fmax(clip.y, std::fmin(A.pos.y, std::fmin(B.pos.y, C.pos.y)));
    const float maxy = std::fmin(clip.w, std::fmax(A.pos.y, std::fmax(B.pos.y, C.pos.y)));
    if (minx >= maxx || miny >= maxy) {
        return;
    }
    const float area = (B.pos.x - A.pos.x) * (C.pos.y - A.pos.y) - (B.pos.y - A.pos.y) * (C.pos.x - A.pos.x);
    if (fabsf(area) < 1e-8f) {
        return;
    }
    auto col = [](ImU32 c, int i) { return ((c >> (i * 8)) & 0xFF) / 255.0f; };
    const int x0 = std::max(0, (int)floorf(minx)), x1 = std::min(cv.w - 1, (int)ceilf(maxx));
    const int y0 = std::max(0, (int)floorf(miny)), y1 = std::min(cv.h - 1, (int)ceilf(maxy));
    for (int y = y0; y <= y1; y++) {
        const float py = y + 0.5f;
        if (py < clip.y || py >= clip.w) {
            continue;
        }
        for (int x = x0; x <= x1; x++) {
            const float px = x + 0.5f;
            if (px < clip.x || px >= clip.z) {
                continue;
            }
            float w0 = ((B.pos.x - px) * (C.pos.y - py) - (B.pos.y - py) * (C.pos.x - px)) / area;
            float w1 = ((C.pos.x - px) * (A.pos.y - py) - (C.pos.y - py) * (A.pos.x - px)) / area;
            float w2 = 1.0f - w0 - w1;
            if (w0 < -1e-5f || w1 < -1e-5f || w2 < -1e-5f) {
                continue;
            }
            float c[4];
            for (int i = 0; i < 4; i++) {
                c[i] = col(A.col, i) * w0 + col(B.col, i) * w1 + col(C.col, i) * w2;
            }
            if (tex != nullptr) {
                float t[4];
                Sample(*tex, A.uv.x * w0 + B.uv.x * w1 + C.uv.x * w2, A.uv.y * w0 + B.uv.y * w1 + C.uv.y * w2, t);
                for (int i = 0; i < 4; i++) {
                    c[i] *= t[i];
                }
            }
            float* d = &cv.rgb[((size_t)y * cv.w + x) * 3];
            for (int i = 0; i < 3; i++) {
                d[i] = d[i] * (1 - c[3]) + c[i] * c[3];
            }
        }
    }
}

static void Render(Canvas& cv, ImDrawData* dd) {
    for (int l = 0; l < dd->CmdListsCount; l++) {
        const ImDrawList* list = dd->CmdLists[l];
        for (const ImDrawCmd& cmd : list->CmdBuffer) {
            if (cmd.UserCallback != nullptr) {
                continue;
            }
            const intptr_t id = (intptr_t)cmd.GetTexID();
            const Image* tex = (id >= 1 && id <= (intptr_t)sTextures.size()) ? &sTextures[id - 1] : nullptr;
            for (unsigned i = 0; i + 2 < cmd.ElemCount; i += 3) {
                const ImDrawIdx* idx = &list->IdxBuffer[cmd.IdxOffset + i];
                RasterTri(cv, list->VtxBuffer[cmd.VtxOffset + idx[0]], list->VtxBuffer[cmd.VtxOffset + idx[1]],
                          list->VtxBuffer[cmd.VtxOffset + idx[2]], tex, cmd.ClipRect);
            }
        }
    }
}

static void WritePPM(const Canvas& cv, const std::string& path) {
    FILE* f = fopen(path.c_str(), "wb");
    fprintf(f, "P6\n%d %d\n255\n", cv.w, cv.h);
    std::vector<unsigned char> out((size_t)cv.w * cv.h * 3);
    for (size_t i = 0; i < out.size(); i++) {
        float v = cv.rgb[i];
        out[i] = (unsigned char)(std::fmin(1.0f, std::fmax(0.0f, v)) * 255.0f + 0.5f);
    }
    fwrite(out.data(), 1, out.size(), f);
    fclose(f);
}

// ---- Scenes

struct Scene {
    const char* name;
    bool extras;
    int selected;
};

int main(int argc, char** argv) {
    if (argc < 4) {
        fprintf(stderr, "usage: preview <raw dir> <out dir> <width> [height] [video]\n");
        return 1;
    }
    const std::string raw = argv[1];
    const std::string out = argv[2];
    const int W = atoi(argv[3]);
    const int H = argc > 4 ? atoi(argv[4]) : 1080;
    const bool video = argc > 5 && strcmp(argv[5], "video") == 0;

    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.DisplaySize = ImVec2((float)W, (float)H);
    io.IniFilename = nullptr;
    const std::string fonts = raw + "/fonts/";
    Fonts& f = GetFonts();
    f.display = io.Fonts->AddFontFromFileTTF((fonts + "Cinzel-Bold.ttf").c_str(), 64.0f);
    f.displayRegular = io.Fonts->AddFontFromFileTTF((fonts + "Cinzel-Regular.ttf").c_str(), 32.0f);
    f.body = io.Fonts->AddFontFromFileTTF((fonts + "EBGaramond-Medium.ttf").c_str(), 44.0f);
    unsigned char* atlasPx;
    int aw, ah;
    io.Fonts->GetTexDataAsRGBA32(&atlasPx, &aw, &ah);
    Image atlas;
    atlas.w = aw;
    atlas.h = ah;
    atlas.px.assign(atlasPx, atlasPx + (size_t)aw * ah * 4);
    io.Fonts->SetTexID(AddTexture(atlas));

    const char* texNames[TEX_COUNT] = { "gDEStone",          "gDEGlow",           "gDESpark",
                                        "gDENaviWing",       "gDETitleBg",        "gDELogo",
                                        "gDEButtonNormal",   "gDEButtonSelected", "gDEButtonPressed",
                                        "gDEFrame" };
    for (int i = 0; i < TEX_COUNT; i++) {
        Image img;
        sThemeTex[i] = LoadRaw(raw + "/" + texNames[i] + ".raw", img) ? AddTexture(img) : (ImTextureID)0;
    }
    SetTextureLookup(LookupTex);

    TitleMenuItem mainItems[MAIN_COUNT];
    const char* mainDetails[MAIN_COUNT] = { Strings::kContinue, nullptr,         Strings::kFileSelect, Strings::kJournal,
                                            Strings::kMods,     Strings::kSettings, Strings::kExtras };
    static char newGame[160];
    snprintf(newGame, sizeof(newGame), Strings::kNewGame, 3);
    mainDetails[MAIN_NEW_GAME] = newGame;
    for (int i = 0; i < MAIN_COUNT; i++) {
        mainItems[i] = { Strings::kMainLabels[i], mainDetails[i], true };
    }
    static char modsLine[200], versionLine[300];
    snprintf(modsLine, sizeof(modsLine), Strings::kExtrasMods, 2, 3);
    snprintf(versionLine, sizeof(versionLine), Strings::kExtrasVersion, "0.9-menu", "9.0.2");
    TitleMenuItem extrasItems[EXTRAS_COUNT] = {
        { Strings::kExtrasLabels[EXTRAS_MODS], modsLine, true },
        { Strings::kExtrasLabels[EXTRAS_VERSION], versionLine, true },
        { Strings::kExtrasLabels[EXTRAS_CREDITS], Strings::kExtrasCredits, false },
        { Strings::kExtrasLabels[EXTRAS_BACK], Strings::kExtrasBack, true },
    };

    const Scene scenes[] = {
        { "menu-principal-continuar", false, MAIN_CONTINUE },
        { "menu-principal-configuracoes", false, MAIN_SETTINGS },
        { "menu-extras-versao", true, EXTRAS_VERSION },
    };

    if (video) {
        // A scripted visit, 30 frames per second: the menu opens, the cursor goes down to Extras, Extras opens,
        // the cursor goes to Voltar and the main menu comes back.
        struct Step {
            float at;
            bool extras;
            int selected;
            bool press;
        };
        const Step steps[] = {
            { 0.0f, false, MAIN_CONTINUE, false },  { 3.2f, false, MAIN_NEW_GAME, false },
            { 3.6f, false, MAIN_FILE_SELECT, false }, { 4.0f, false, MAIN_JOURNAL, false },
            { 5.0f, false, MAIN_MODS, false },      { 5.4f, false, MAIN_SETTINGS, false },
            { 5.8f, false, MAIN_EXTRAS, false },    { 7.0f, false, MAIN_EXTRAS, true },
            { 7.25f, true, EXTRAS_MODS, false },    { 8.6f, true, EXTRAS_VERSION, false },
            { 10.0f, true, EXTRAS_CREDITS, false }, { 10.4f, true, EXTRAS_BACK, false },
            { 11.4f, true, EXTRAS_BACK, true },     { 11.65f, false, MAIN_EXTRAS, false },
            { 13.0f, false, MAIN_EXTRAS, false },
        };
        const int nSteps = (int)(sizeof(steps) / sizeof(steps[0]));
        const float fps = 30.0f;
        const int frames = (int)(steps[nSteps - 1].at * fps);
        TitleMenuRenderer renderer;
        int fileNo = 0;
        for (int fr = 0; fr < frames; fr++) {
            const float t = fr / fps;
            const Step* st = &steps[0];
            for (int i = 0; i < nSteps; i++) {
                if (steps[i].at <= t) {
                    st = &steps[i];
                }
            }
            TitleMenuView view;
            view.screen = st->extras ? 1 : 0;
            view.items = st->extras ? extrasItems : mainItems;
            view.count = st->extras ? (int)EXTRAS_COUNT : (int)MAIN_COUNT;
            view.title = st->extras ? Strings::kExtrasTitle : nullptr;
            view.showBack = st->extras;
            view.footer = "OOT D.E. 0.9-menu · Ship of Harkinian 9.0.2 · © Nintendo";
            view.selected = st->selected;
            view.pressed = (st->press && t - st->at < 0.17f) ? st->selected : -1;
            if (!st->extras && (st->selected == MAIN_CONTINUE || st->selected == MAIN_JOURNAL)) {
                view.showSave = true;
                snprintf(view.saveTitle, sizeof(view.saveTitle), Strings::kSaveTitle, 1, "Link");
                view.heartsCapacity = 13;
                view.health = 13 * 16 - 6;
            }
            io.DeltaTime = 1.0f / fps;
            ImGui::NewFrame();
            renderer.Draw(ImGui::GetBackgroundDrawList(), ImVec2(0, 0), io.DisplaySize, view, io.DeltaTime, t,
                          PointerState());
            ImGui::Render();
            Canvas cv{ W, H, std::vector<float>((size_t)W * H * 3, 0.0f) };
            Render(cv, ImGui::GetDrawData());
            char name[64];
            snprintf(name, sizeof(name), "/frame%04d.ppm", fileNo++);
            WritePPM(cv, out + name);
        }
        printf("wrote %d frames to %s\n", fileNo, out.c_str());
        ImGui::DestroyContext();
        return 0;
    }

    for (const Scene& sc : scenes) {
        TitleMenuRenderer renderer;
        TitleMenuView view;
        view.items = sc.extras ? extrasItems : mainItems;
        view.count = sc.extras ? (int)EXTRAS_COUNT : (int)MAIN_COUNT;
        view.title = sc.extras ? Strings::kExtrasTitle : nullptr;
        view.showBack = sc.extras;
        view.footer = "OOT D.E. 0.9-menu · Ship of Harkinian 9.0.2 · © Nintendo";
        if (!sc.extras && (sc.selected == MAIN_CONTINUE || sc.selected == MAIN_JOURNAL)) {
            view.showSave = true;
            snprintf(view.saveTitle, sizeof(view.saveTitle), Strings::kSaveTitle, 1, "Link");
            view.heartsCapacity = 13;
            view.health = 13 * 16 - 6;
        }
        const int frames = 150;
        for (int fr = 0; fr < frames; fr++) {
            const float t = fr / 60.0f;
            io.DeltaTime = 1.0f / 60.0f;
            ImGui::NewFrame();
            ImDrawList* dl = ImGui::GetBackgroundDrawList();
            view.selected = (fr < 40) ? 0 : sc.selected; // let Navi fly from the first button
            view.alpha = ImMin(1.0f, t / 0.4f);
            view.slide = ImMax(0.0f, 1.0f - t / 0.35f);
            view.slide = view.slide * view.slide;
            renderer.Draw(dl, ImVec2(0, 0), io.DisplaySize, view, io.DeltaTime, t, PointerState());
            ImGui::Render();
            if (fr == frames - 1) {
                Canvas cv{ W, H, std::vector<float>((size_t)W * H * 3, 0.0f) };
                Render(cv, ImGui::GetDrawData());
                WritePPM(cv, out + "/" + sc.name + ".ppm");
                printf("wrote %s/%s.ppm\n", out.c_str(), sc.name);
            }
        }
    }
    ImGui::DestroyContext();
    return 0;
}
