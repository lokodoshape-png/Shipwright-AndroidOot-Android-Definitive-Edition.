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

enum SceneKind { SCENE_MAIN, SCENE_FEATURES, SCENE_FEATURES_MORE, SCENE_EXTRAS, SCENE_FILES, SCENE_FILE, SCENE_QUEST, SCENE_NAME, SCENE_SPLASH, SCENE_LOADING };

struct Scene {
    const char* name;
    SceneKind kind;
    int selected;
};

static const char* const kFooter = "OOT D.E. 0.9 · Ship of Harkinian 9.0.2 · © Nintendo";

static TitleMenuItem sMainItems[MAIN_COUNT];
static TitleMenuItem sExtrasItems[EXTRAS_COUNT];
static TitleMenuItem sFeatureItems[Strings::kFeaturesFirstPage + 2];
static TitleMenuItem sFeatureMoreItems[FEATURE_COUNT - Strings::kFeaturesFirstPage + 1];
static TitleMenuItem sFilesItems[4];
static TitleMenuItem sFileItems[4];
static TitleMenuItem sQuestItems[4];
static KeyCell sKeys[KEY_COUNT];
static char sKeyLabels[KEY_CHARS][2];
static char sTitle[64];

// Fills the view as the game does for each screen (texts of DeTitleMenuText.h, made-up save data).
static void FillView(TitleMenuView& view, SceneKind kind, int selected, bool touch) {
    view = TitleMenuView();
    view.screen = (int)kind;
    view.selected = selected;
    view.footer = kFooter;
    view.touchMode = touch;
    view.showBack = kind != SCENE_MAIN;
    bool save = false;
    switch (kind) {
        case SCENE_FEATURES:
            view.items = sFeatureItems;
            view.count = Strings::kFeaturesFirstPage + 2;
            view.title = Strings::kFeaturesTitle;
            break;
        case SCENE_FEATURES_MORE:
            view.items = sFeatureMoreItems;
            view.count = FEATURE_COUNT - Strings::kFeaturesFirstPage + 1;
            view.title = Strings::kFeaturesMoreTitle;
            break;
        case SCENE_EXTRAS:
            view.items = sExtrasItems;
            view.count = EXTRAS_COUNT;
            view.title = Strings::kExtrasTitle;
            break;
        case SCENE_FILES:
            view.items = sFilesItems;
            view.count = 4;
            view.title = Strings::kFilesTitle;
            save = selected < 2;
            break;
        case SCENE_FILE:
            view.items = sFileItems;
            view.count = 4;
            snprintf(sTitle, sizeof(sTitle), Strings::kFileLabel, 1, "Link");
            view.title = sTitle;
            save = true;
            break;
        case SCENE_QUEST:
            view.items = sQuestItems;
            view.count = 4;
            snprintf(sTitle, sizeof(sTitle), Strings::kNewTitle, 3);
            view.title = sTitle;
            break;
        case SCENE_NAME:
            view.title = Strings::kNameTitle;
            view.keys = sKeys;
            view.keyCount = KEY_COUNT;
            view.keySelected = selected;
            view.nameText = "Rafael";
            view.nameMax = 8;
            view.nameHint = Strings::kNameHint;
            break;
        default:
            view.items = sMainItems;
            view.count = MAIN_COUNT;
            save = selected == MAIN_CONTINUE || selected == MAIN_JOURNAL;
            break;
    }
    if (save) {
        view.showSave = true;
        snprintf(view.saveTitle, sizeof(view.saveTitle), Strings::kSaveTitle, 1, "Link");
        view.heartsCapacity = 13;
        view.health = 13 * 16 - 6;
    }
}

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

    const char* mainDetails[MAIN_COUNT] = { Strings::kContinue, nullptr,        Strings::kFiles,
                                            Strings::kJournal,  Strings::kFeatures, Strings::kMods,
                                            Strings::kSettings, Strings::kExtras };
    for (int i = 0; i < Strings::kFeaturesFirstPage; i++) {
        sFeatureItems[i] = { Strings::kFeatureLabels[i], Strings::kFeatureDetails[i], true };
    }
    sFeatureItems[Strings::kFeaturesFirstPage] = { Strings::kFeaturesMore, Strings::kFeaturesMoreDetail, true };
    sFeatureItems[Strings::kFeaturesFirstPage + 1] = { Strings::kBack, Strings::kExtrasBack, true };
    for (int i = Strings::kFeaturesFirstPage; i < FEATURE_COUNT; i++) {
        sFeatureMoreItems[i - Strings::kFeaturesFirstPage] = { Strings::kFeatureLabels[i], Strings::kFeatureDetails[i],
                                                               true };
    }
    sFeatureMoreItems[FEATURE_COUNT - Strings::kFeaturesFirstPage] = { Strings::kBack, Strings::kBackDetail, true };
    static char newGame[160];
    snprintf(newGame, sizeof(newGame), Strings::kNewGame, 3);
    mainDetails[MAIN_NEW_GAME] = newGame;
    for (int i = 0; i < MAIN_COUNT; i++) {
        sMainItems[i] = { Strings::kMainLabels[i], mainDetails[i], true };
    }
    static char modsLine[200], versionLine[300];
    snprintf(modsLine, sizeof(modsLine), Strings::kExtrasMods, 2, 3);
    snprintf(versionLine, sizeof(versionLine), Strings::kExtrasVersion, "0.9", "9.0.2");
    sExtrasItems[EXTRAS_MODS] = { Strings::kExtrasLabels[EXTRAS_MODS], modsLine, true };
    sExtrasItems[EXTRAS_VERSION] = { Strings::kExtrasLabels[EXTRAS_VERSION], versionLine, true };
    sExtrasItems[EXTRAS_CREDITS] = { Strings::kExtrasLabels[EXTRAS_CREDITS], Strings::kExtrasCredits, false };
    sExtrasItems[EXTRAS_BACK] = { Strings::kExtrasLabels[EXTRAS_BACK], Strings::kExtrasBack, true };

    static char fileLabels[3][64], fileDetails[3][200];
    snprintf(fileLabels[0], 64, Strings::kFileLabel, 1, "Link");
    snprintf(fileDetails[0], 200, Strings::kFileInfo, Strings::kQuestNormal, 4);
    snprintf(fileLabels[1], 64, Strings::kFileLabel, 2, "Zelda");
    snprintf(fileDetails[1], 200, Strings::kFileInfo, Strings::kQuestMaster, 0);
    snprintf(fileLabels[2], 64, Strings::kFileEmptyLabel, 3);
    snprintf(fileDetails[2], 200, "%s", Strings::kFileEmpty);
    for (int i = 0; i < 3; i++) {
        sFilesItems[i] = { fileLabels[i], fileDetails[i], true };
    }
    sFilesItems[3] = { Strings::kBack, Strings::kBackDetail, true };
    sFileItems[0] = { Strings::kPlay, Strings::kPlayDetail, true };
    sFileItems[1] = { Strings::kCopy, Strings::kCopyDetail, true };
    sFileItems[2] = { Strings::kErase, Strings::kEraseDetail, true };
    sFileItems[3] = { Strings::kBack, Strings::kBackDetail, true };
    sQuestItems[0] = { Strings::kQuestNormal, Strings::kQuestNormalDetail, true };
    sQuestItems[1] = { Strings::kQuestMaster, Strings::kQuestMasterDetail, true };
    sQuestItems[2] = { Strings::kClassic, Strings::kClassicDetail, true };
    sQuestItems[3] = { Strings::kBack, Strings::kBackDetail, true };
    BuildNameKeys(false, sKeys, sKeyLabels);

    const Scene scenes[] = {
        { "abertura-definitive-edition", SCENE_SPLASH, 0 },
        { "menu-principal-continuar", SCENE_MAIN, MAIN_CONTINUE },
        { "menu-principal-configuracoes", SCENE_MAIN, MAIN_SETTINGS },
        { "menu-extras-versao", SCENE_EXTRAS, EXTRAS_VERSION },
        { "menu-principal-definitive-features", SCENE_MAIN, MAIN_FEATURES },
        { "definitive-features-1", SCENE_FEATURES, FEATURE_SAVES },
        { "definitive-features-2", SCENE_FEATURES_MORE, 1 },
        { "arquivos-de-jogo", SCENE_FILES, 0 },
        { "arquivo-opcoes", SCENE_FILE, 0 },
        { "novo-jogo-aventura", SCENE_QUEST, 0 },
        { "novo-jogo-nome", SCENE_NAME, 5 },
        { "carregando", SCENE_LOADING, 0 },
    };

    if (video) {
        // A scripted visit, 30 frames per second: the opening image, the menu, Arquivos de Jogo, back, Novo Jogo,
        // the name of the hero and the loading screen.
        struct Step {
            float at;
            SceneKind kind;
            int selected;
            bool press;
        };
        const Step steps[] = {
            { 0.0f, SCENE_SPLASH, 0, false },          { 4.5f, SCENE_MAIN, MAIN_CONTINUE, false },
            { 7.0f, SCENE_MAIN, MAIN_CONTINUE, false },
            { 7.2f, SCENE_MAIN, MAIN_NEW_GAME, false }, { 7.6f, SCENE_MAIN, MAIN_FILES, false },
            { 8.6f, SCENE_MAIN, MAIN_FILES, true },    { 8.85f, SCENE_FILES, 0, false },
            { 10.0f, SCENE_FILES, 1, false },          { 10.6f, SCENE_FILES, 2, false },
            { 11.4f, SCENE_FILES, 3, false },          { 12.0f, SCENE_FILES, 3, true },
            { 12.25f, SCENE_MAIN, MAIN_FILES, false }, { 12.9f, SCENE_MAIN, MAIN_NEW_GAME, false },
            { 13.5f, SCENE_MAIN, MAIN_NEW_GAME, true }, { 13.75f, SCENE_QUEST, 0, false },
            { 15.0f, SCENE_QUEST, 0, true },           { 15.25f, SCENE_NAME, 0, false },
            { 16.0f, SCENE_NAME, 11, false },          { 16.5f, SCENE_NAME, 5, false },
            { 17.0f, SCENE_NAME, KEY_DONE, false },    { 18.0f, SCENE_LOADING, 0, false },
            { 20.0f, SCENE_LOADING, 0, false },
        };
        const int nSteps = (int)(sizeof(steps) / sizeof(steps[0]));
        const float fps = 30.0f;
        const int frames = (int)(steps[nSteps - 1].at * fps);
        TitleMenuRenderer renderer;
        int fileNo = 0;
        float menuStart = -1.0f;
        Fx::CinematicReveal reveal;
        bool revealDone = false;
        for (int fr = 0; fr < frames; fr++) {
            const float t = fr / fps;
            const Step* st = &steps[0];
            for (int i = 0; i < nSteps; i++) {
                if (steps[i].at <= t) {
                    st = &steps[i];
                }
            }
            io.DeltaTime = 1.0f / fps;
            ImGui::NewFrame();
            ImDrawList* dl = ImGui::GetBackgroundDrawList();
            // As in the game: the opening image, then the light (Fx::CinematicReveal) opening on the menu.
            if (st->kind != SCENE_SPLASH && !reveal.Running() && !revealDone) {
                reveal.Start();
            }
            if (reveal.Running()) {
                reveal.Update(io.DeltaTime, ImVec2(0, 0), io.DisplaySize);
            }
            if (st->kind == SCENE_SPLASH || (reveal.Running() && !reveal.MenuShown())) {
                dl->AddRectFilled(ImVec2(0, 0), io.DisplaySize, IM_COL32_BLACK);
            } else {
                if (menuStart < 0.0f) {
                    menuStart = t;
                    renderer.Reset();
                }
                TitleMenuView view;
                const SceneKind kind = st->kind == SCENE_LOADING ? SCENE_NAME : st->kind;
                FillView(view, kind, st->kind == SCENE_LOADING ? (int)KEY_DONE : st->selected, false);
                view.pressed = (st->press && t - st->at < 0.17f) ? st->selected : -1;
                view.alpha = ImMin(1.0f, (t - menuStart) / 0.4f);
                renderer.Draw(dl, ImVec2(0, 0), io.DisplaySize, view, io.DeltaTime, t - menuStart, PointerState());
                if (st->kind == SCENE_LOADING) {
                    renderer.DrawLoading(dl, ImVec2(0, 0), io.DisplaySize, t, ImMin(1.0f, (t - st->at) / 0.3f),
                                         Strings::kCreating);
                }
            }
            if (st->kind == SCENE_SPLASH || reveal.Running()) {
                if (reveal.Running()) {
                    reveal.DrawCover(dl, ImVec2(0, 0), io.DisplaySize);
                }
                const float imageAlpha = reveal.Running() ? reveal.SplashAlpha() : ImMin(1.0f, t / 1.5f);
                if (imageAlpha > 0.0f) {
                    renderer.DrawSplash(dl, ImVec2(0, 0), io.DisplaySize, io.DeltaTime, t, imageAlpha);
                }
                reveal.DrawEffects(dl, ImVec2(0, 0), io.DisplaySize);
                if (reveal.Done()) {
                    reveal.Release();
                    revealDone = true;
                }
            }
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
        // The screens a phone shows are drawn with the touch hints.
        FillView(view, sc.kind == SCENE_LOADING || sc.kind == SCENE_SPLASH ? SCENE_MAIN : sc.kind, sc.selected,
                 sc.kind == SCENE_FILES || sc.kind == SCENE_NAME);
        const int target = view.selected;
        const int frames = 150;
        for (int fr = 0; fr < frames; fr++) {
            const float t = fr / 60.0f;
            io.DeltaTime = 1.0f / 60.0f;
            ImGui::NewFrame();
            ImDrawList* dl = ImGui::GetBackgroundDrawList();
            if (sc.kind == SCENE_SPLASH) {
                renderer.DrawSplash(dl, ImVec2(0, 0), io.DisplaySize, io.DeltaTime, t, ImMin(1.0f, t / 1.5f));
            } else {
                if (view.keys == nullptr) {
                    view.selected = (fr < 40) ? 0 : target; // let Navi fly from the first button
                }
                view.alpha = ImMin(1.0f, t / 0.4f);
                view.slide = ImMax(0.0f, 1.0f - t / 0.35f);
                view.slide = view.slide * view.slide;
                renderer.Draw(dl, ImVec2(0, 0), io.DisplaySize, view, io.DeltaTime, t, PointerState());
                if (sc.kind == SCENE_LOADING) {
                    renderer.DrawLoading(dl, ImVec2(0, 0), io.DisplaySize, t, 1.0f, Strings::kLoading);
                }
            }
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
