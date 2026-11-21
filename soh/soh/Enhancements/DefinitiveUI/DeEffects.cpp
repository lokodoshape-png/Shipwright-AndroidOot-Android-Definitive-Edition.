// OOT D.E.: cinematic effects of the Definitive Edition interface. See DeEffects.h.
#include "DeEffects.h"
#include "DeTheme.h"

#include <imgui_internal.h>
#include <cmath>

namespace DefinitiveUI {
namespace Fx {

static constexpr float kPi = 3.14159265f;

float EaseInOut(float t) {
    t = ImClamp(t, 0.0f, 1.0f);
    return t * t * (3.0f - 2.0f * t);
}

float EaseIn(float t) {
    t = ImClamp(t, 0.0f, 1.0f);
    return t * t * t;
}

float Window(float time, float start, float duration, float edge) {
    if (time <= start || time >= start + duration) {
        return 0.0f;
    }
    const float in = edge > 0.0f ? (time - start) / edge : 1.0f;
    const float out = edge > 0.0f ? (start + duration - time) / edge : 1.0f;
    return EaseInOut(ImMin(1.0f, ImMin(in, out)));
}

// ---------------------------------------------------------------------------------------------------------------------
// Fades and masks

void ScreenFade(ImDrawList* dl, ImVec2 a, ImVec2 b, ImU32 col, float alpha) {
    if (alpha <= 0.0f) {
        return;
    }
    dl->AddRectFilled(a, b, Fade(col, ImMin(alpha, 1.0f)));
}

void Vignette(ImDrawList* dl, ImVec2 a, ImVec2 b, float strength) {
    if (strength <= 0.0f) {
        return;
    }
    const ImVec2 size = b - a;
    const float bandX = size.x * 0.22f;
    const float bandY = size.y * 0.28f;
    const ImU32 dark = Fade(Col::Ink, ImMin(strength, 1.0f) * 0.85f);
    const ImU32 clear = Fade(Col::Ink, 0.0f);
    dl->AddRectFilledMultiColor(a, ImVec2(a.x + bandX, b.y), dark, clear, clear, dark);
    dl->AddRectFilledMultiColor(ImVec2(b.x - bandX, a.y), b, clear, dark, dark, clear);
    dl->AddRectFilledMultiColor(a, ImVec2(b.x, a.y + bandY), dark, dark, clear, clear);
    dl->AddRectFilledMultiColor(ImVec2(a.x, b.y - bandY), b, clear, clear, dark, dark);
}

// A ring between r0 (color c0) and r1 (color c1), as one strip of triangles.
static void Ring(ImDrawList* dl, ImVec2 c, float r0, float r1, ImU32 c0, ImU32 c1, int segments) {
    const ImVec2 uv = dl->_Data->TexUvWhitePixel;
    dl->PrimReserve(segments * 6, (segments + 1) * 2);
    const ImDrawIdx base = (ImDrawIdx)dl->_VtxCurrentIdx;
    for (int i = 0; i <= segments; i++) {
        const float ang = (float)i / segments * 2.0f * kPi;
        const ImVec2 d(cosf(ang), sinf(ang));
        dl->PrimWriteVtx(c + d * r0, uv, c0);
        dl->PrimWriteVtx(c + d * r1, uv, c1);
    }
    for (int i = 0; i < segments; i++) {
        const ImDrawIdx k = (ImDrawIdx)(base + i * 2);
        dl->PrimWriteIdx(k);
        dl->PrimWriteIdx((ImDrawIdx)(k + 1));
        dl->PrimWriteIdx((ImDrawIdx)(k + 3));
        dl->PrimWriteIdx(k);
        dl->PrimWriteIdx((ImDrawIdx)(k + 3));
        dl->PrimWriteIdx((ImDrawIdx)(k + 2));
    }
}

void IrisMask(ImDrawList* dl, ImVec2 a, ImVec2 b, ImVec2 center, float radius, float soft, ImU32 col) {
    if (((col >> IM_COL32_A_SHIFT) & 0xFF) == 0) {
        return;
    }
    if (radius <= 0.0f) {
        dl->AddRectFilled(a, b, col);
        return;
    }
    // Far enough to cover every corner.
    float far = 0.0f;
    const ImVec2 corners[4] = { a, ImVec2(b.x, a.y), b, ImVec2(a.x, b.y) };
    for (const ImVec2& p : corners) {
        const ImVec2 d = p - center;
        far = ImMax(far, sqrtf(d.x * d.x + d.y * d.y));
    }
    if (radius >= far) {
        return;
    }
    dl->PushClipRect(a, b, true);
    const ImU32 clear = col & ~IM_COL32_A_MASK;
    const float edge = ImMin(radius + ImMax(soft, 1.0f), far + 1.0f);
    Ring(dl, center, radius, edge, clear, col, 64);
    if (edge < far + 1.0f) {
        Ring(dl, center, edge, far + 2.0f, col, col, 64);
    }
    dl->PopClipRect();
}

void WipeMask(ImDrawList* dl, ImVec2 a, ImVec2 b, float t, float soft, ImU32 col) {
    t = ImClamp(t, 0.0f, 1.0f);
    if (t <= 0.0f) {
        return;
    }
    const float x = a.x + (b.x - a.x + soft) * t;
    const ImU32 clear = col & ~IM_COL32_A_MASK;
    const float solid = ImMin(x - soft, b.x);
    if (solid > a.x) {
        dl->AddRectFilled(a, ImVec2(solid, b.y), col);
    }
    const float e0 = ImMax(x - soft, a.x);
    const float e1 = ImMin(x, b.x);
    if (e1 > e0) {
        dl->AddRectFilledMultiColor(ImVec2(e0, a.y), ImVec2(e1, b.y), col, clear, clear, col);
    }
}

// ---------------------------------------------------------------------------------------------------------------------
// Light

void GoldenGlow(ImDrawList* dl, ImVec2 center, float radius, float intensity, float time) {
    if (intensity <= 0.0f || radius <= 0.0f) {
        return;
    }
    const float breathe = 1.0f + 0.05f * sinf(time * 3.1f);
    const float k = ImMin(intensity, 1.0f);
    Glow(dl, center, radius * 1.9f * breathe, Fade(Col::Gold, k * 0.28f));
    Glow(dl, center, radius * 1.2f * breathe, Fade(Col::GoldLight, k * 0.45f));
    Glow(dl, center, radius * 0.7f, Fade(Col::GoldPale, k * 0.7f));
    Glow(dl, center, radius * 0.32f, Fade(IM_COL32_WHITE, ImMin(1.0f, intensity) * 0.9f));
    if (intensity > 1.0f) {
        // Overexposed: the light swallows everything around it.
        Glow(dl, center, radius * 3.0f, Fade(Col::GoldPale, ImMin(1.0f, intensity - 1.0f) * 0.8f));
    }
}

void LensFlare(ImDrawList* dl, ImVec2 c, float size, float alpha, float time) {
    if (alpha <= 0.0f || size <= 0.0f) {
        return;
    }
    alpha = ImMin(alpha, 1.0f);
    const ImVec2 uv = dl->_Data->TexUvWhitePixel;
    // Horizontal streak: a thin diamond, bright in the middle, clear at its ends.
    const float len = size * (2.6f + 0.2f * sinf(time * 2.3f));
    const float th = size * 0.05f;
    const ImU32 mid = Fade(Col::GoldPale, alpha * 0.85f);
    const ImU32 clear = Fade(Col::GoldPale, 0.0f);
    dl->PrimReserve(12, 5);
    const ImDrawIdx base = (ImDrawIdx)dl->_VtxCurrentIdx;
    dl->PrimWriteVtx(c, uv, mid);
    dl->PrimWriteVtx(c + ImVec2(-len, 0.0f), uv, clear);
    dl->PrimWriteVtx(c + ImVec2(0.0f, -th), uv, clear);
    dl->PrimWriteVtx(c + ImVec2(len, 0.0f), uv, clear);
    dl->PrimWriteVtx(c + ImVec2(0.0f, th), uv, clear);
    for (int i = 0; i < 4; i++) {
        dl->PrimWriteIdx(base);
        dl->PrimWriteIdx((ImDrawIdx)(base + 1 + i));
        dl->PrimWriteIdx((ImDrawIdx)(base + 1 + (i + 1) % 4));
    }
    Glow(dl, c, size * 0.9f, Fade(Col::GoldLight, alpha * 0.5f));
    Spark(dl, c, size * 0.55f, Fade(IM_COL32_WHITE, alpha), time * 0.4f);
    // A faint halo ring, as a camera lens would show it.
    dl->AddCircle(c, size * 1.4f, Fade(Col::MagicLight, alpha * 0.18f), 48, ImMax(1.0f, size * 0.04f));
}

void TimeRipple(ImDrawList* dl, ImVec2 c, float maxRadius, float t, float alpha, float s) {
    if (alpha <= 0.0f || t <= 0.0f || t >= 1.0f) {
        return;
    }
    const int kRings = 3;
    const int kSeg = 72;
    ImVec2 pts[kSeg];
    for (int r = 0; r < kRings; r++) {
        const float u = t - r * 0.16f;
        if (u <= 0.0f || u >= 1.0f) {
            continue;
        }
        const float radius = maxRadius * EaseOut(u);
        const float a = alpha * (1.0f - u) * (1.0f - u);
        const float wobble = radius * 0.025f * (1.0f - u);
        for (int i = 0; i < kSeg; i++) {
            const float ang = (float)i / kSeg * 2.0f * kPi;
            const float rr = radius + wobble * sinf(ang * 6.0f + u * 9.0f + r * 1.7f);
            pts[i] = c + ImVec2(cosf(ang), sinf(ang)) * rr;
        }
        const ImU32 col = (r % 2 == 0) ? Col::GoldLight : Col::MagicLight;
        dl->AddPolyline(pts, kSeg, Fade(col, a * 0.75f), ImDrawFlags_Closed, (5.0f - 2.0f * u * 2.0f) * s + 1.0f);
        // A faint echo just inside: the image seems to tremble.
        dl->AddCircle(c, radius * 0.94f, Fade(col, a * 0.25f), 48, 2.0f * s);
    }
}

// ---------------------------------------------------------------------------------------------------------------------
// Magic particles

float MagicParticles::Rand() {
    mSeed = mSeed * 1664525u + 1013904223u;
    return (float)((mSeed >> 8) & 0xFFFFFF) / (float)0x1000000;
}

MagicParticles::P* MagicParticles::Free() {
    for (P& p : mPool) {
        if (p.life <= 0.0f) {
            return &p;
        }
    }
    return nullptr;
}

void MagicParticles::EmitFromEdges(ImVec2 a, ImVec2 b, ImVec2 target, float s, int count) {
    for (int i = 0; i < count; i++) {
        P* p = Free();
        if (p == nullptr) {
            return;
        }
        // A point on the edge (a little inside it), chosen around the whole frame.
        const float e = Rand() * 4.0f;
        const float u = e - floorf(e);
        const float in = (10.0f + Rand() * 60.0f) * s;
        switch ((int)e) {
            case 0: p->pos = ImVec2(ImLerp(a.x, b.x, u), a.y + in); break;
            case 1: p->pos = ImVec2(b.x - in, ImLerp(a.y, b.y, u)); break;
            case 2: p->pos = ImVec2(ImLerp(a.x, b.x, u), b.y - in); break;
            default: p->pos = ImVec2(a.x + in, ImLerp(a.y, b.y, u)); break;
        }
        p->target = target + ImVec2((Rand() - 0.5f) * 40.0f * s, (Rand() - 0.5f) * 40.0f * s);
        const ImVec2 d = p->target - p->pos;
        const float len = ImMax(1.0f, sqrtf(d.x * d.x + d.y * d.y));
        // Sideways at first, so they curl in as they are pulled.
        const ImVec2 side(-d.y / len, d.x / len);
        const float sw = (Rand() - 0.5f) * 2.0f;
        p->vel = side * (120.0f * sw * s) + d / len * (40.0f * s);
        p->maxLife = p->life = 1.4f + Rand() * 0.6f;
        p->size = (10.0f + Rand() * 16.0f) * s;
        p->phase = Rand() * 6.28f;
        p->spark = Rand() < 0.45f;
        p->gather = true;
        p->col = Rand() < 0.78f ? Col::GoldLight : Col::MagicLight;
    }
}

void MagicParticles::EmitRing(ImVec2 c, float radius, float s, int count) {
    for (int i = 0; i < count; i++) {
        P* p = Free();
        if (p == nullptr) {
            return;
        }
        const float ang = Rand() * 2.0f * kPi;
        const ImVec2 d(cosf(ang), sinf(ang));
        p->pos = c + d * (radius * Rand());
        const float speed = (300.0f + Rand() * 700.0f) * s;
        p->vel = d * speed;
        p->target = c;
        p->maxLife = p->life = 0.6f + Rand() * 0.7f;
        p->size = (8.0f + Rand() * 14.0f) * s;
        p->phase = Rand() * 6.28f;
        p->spark = Rand() < 0.7f;
        p->gather = false;
        p->col = Rand() < 0.7f ? Col::GoldPale : Col::MagicLight;
    }
}

void MagicParticles::Update(float dt, float pull) {
    for (P& p : mPool) {
        if (p.life <= 0.0f) {
            continue;
        }
        p.life -= dt;
        if (p.gather && pull > 0.0f) {
            const ImVec2 d = p.target - p.pos;
            const float len = sqrtf(d.x * d.x + d.y * d.y);
            // Steer toward the target; arriving, the mote melts into the light.
            p.vel += d * (pull * 9.0f * dt);
            p.vel *= ImMax(0.0f, 1.0f - 2.6f * dt);
            if (len < 18.0f) {
                p.life = ImMin(p.life, 0.12f);
            }
        } else {
            p.vel *= ImMax(0.0f, 1.0f - 1.8f * dt);
        }
        p.pos += p.vel * dt;
    }
}

void MagicParticles::Draw(ImDrawList* dl, float alpha) const {
    if (alpha <= 0.0f) {
        return;
    }
    for (const P& p : mPool) {
        if (p.life <= 0.0f) {
            continue;
        }
        const float t = p.life / p.maxLife; // 1 -> 0
        const float fade = ImMin(1.0f, (1.0f - t) * 5.0f) * ImMin(1.0f, t * 4.0f);
        const float tw = 0.7f + 0.3f * sinf(p.phase + (1.0f - t) * 18.0f);
        if (p.spark) {
            Spark(dl, p.pos, p.size, Fade(p.col, alpha * fade * tw), p.phase + t * 2.0f);
        } else {
            Glow(dl, p.pos, p.size * 1.3f, Fade(p.col, alpha * fade * tw * 0.85f));
        }
    }
}

void MagicParticles::Clear() {
    for (P& p : mPool) {
        p.life = 0.0f;
    }
}

bool MagicParticles::Active() const {
    for (const P& p : mPool) {
        if (p.life > 0.0f) {
            return true;
        }
    }
    return false;
}

// ---------------------------------------------------------------------------------------------------------------------
// Mist

void Mist::Update(float dt, ImVec2 a, ImVec2 b, float s) {
    const float w = b.x - a.x;
    const float h = b.y - a.y;
    if (!mReady || mA.x != a.x || mA.y != a.y || mB.x != b.x || mB.y != b.y) {
        // Spread the clouds out once (and again if the area changes size).
        for (int i = 0; i < kClouds; i++) {
            const float u = (i + 0.5f) / kClouds;
            mPos[i] = ImVec2(a.x + w * u, a.y + h * (0.55f + 0.4f * fmodf(u * 3.7f, 1.0f)));
            mSpeed[i] = (10.0f + 14.0f * fmodf(u * 5.3f, 1.0f)) * s * ((i % 2) ? 1.0f : -0.7f);
            mRadius[i] = h * (0.28f + 0.2f * fmodf(u * 2.9f, 1.0f));
            mPhase[i] = u * 6.28f;
        }
        mA = a;
        mB = b;
        mReady = true;
    }
    for (int i = 0; i < kClouds; i++) {
        mPos[i].x += mSpeed[i] * dt;
        mPhase[i] += dt * 0.25f;
        const float r = mRadius[i];
        if (mPos[i].x > b.x + r) {
            mPos[i].x = a.x - r;
        } else if (mPos[i].x < a.x - r) {
            mPos[i].x = b.x + r;
        }
    }
}

void Mist::Draw(ImDrawList* dl, float alpha, ImU32 col) const {
    if (!mReady || alpha <= 0.0f) {
        return;
    }
    for (int i = 0; i < kClouds; i++) {
        const float breathe = 0.75f + 0.25f * sinf(mPhase[i]);
        const ImVec2 p = mPos[i] + ImVec2(0.0f, sinf(mPhase[i] * 0.7f) * mRadius[i] * 0.06f);
        Glow(dl, p, mRadius[i], Fade(col, alpha * 0.22f * breathe));
    }
}

void Mist::Clear() {
    mReady = false;
}

// ---------------------------------------------------------------------------------------------------------------------
// The opening of the menu

// Timeline (seconds since Start):
//   0.00-0.90  the opening image fades into the dark
//   0.10-1.35  golden motes come in from the edges, curling toward the middle
//   0.45-1.75  the light gathers in the middle
//   1.00-1.75  the Triforce appears in it, with a ripple of time around it
//   1.75-2.55  the light bursts and opens like an iris on the menu (which starts its own opening animation)
//   2.55-2.80  the last sparks fade
static constexpr float kFadeEnd = 0.9f;
static constexpr float kEmitStart = 0.1f;
static constexpr float kEmitEnd = 1.35f;
static constexpr float kGatherStart = 0.45f;
static constexpr float kTriforceStart = 1.0f;

void CinematicReveal::Start() {
    mRunning = true;
    mTime = 0.0f;
    mEmitTimer = 0.0f;
    mBurst = false;
    mMotes.Clear();
}

void CinematicReveal::Skip() {
    if (mRunning && mTime < kIrisStart) {
        mTime = kIrisStart;
    }
}

void CinematicReveal::Release() {
    mRunning = false;
    mTime = 0.0f;
    mBurst = false;
    mMotes.Clear();
}

float CinematicReveal::SplashAlpha() const {
    if (!mRunning) {
        return 1.0f;
    }
    return 1.0f - EaseInOut(mTime / kFadeEnd);
}

void CinematicReveal::Update(float dt, ImVec2 origin, ImVec2 size) {
    if (!mRunning) {
        return;
    }
    dt = ImClamp(dt, 0.0f, 0.1f);
    mTime += dt;
    const float s = size.y / 1080.0f;
    const ImVec2 c = origin + size * 0.5f;
    if (mTime >= kEmitStart && mTime < kEmitEnd) {
        mEmitTimer -= dt;
        while (mEmitTimer <= 0.0f) {
            mEmitTimer += 0.018f;
            mMotes.EmitFromEdges(origin, origin + size, c, s, 3);
        }
    }
    if (mTime >= kIrisStart && !mBurst) {
        mBurst = true;
        mMotes.EmitRing(c, 60.0f * s, s, 70);
    }
    mMotes.Update(dt, mTime < kIrisStart ? 1.0f + 2.0f * (mTime / kIrisStart) : 0.0f);
}

void CinematicReveal::DrawCover(ImDrawList* dl, ImVec2 origin, ImVec2 size) const {
    if (!mRunning) {
        return;
    }
    const ImVec2 c = origin + size * 0.5f;
    const float far = sqrtf(size.x * size.x + size.y * size.y) * 0.5f;
    const float open = EaseIn(ImClamp((mTime - kIrisStart) / kIrisLength, 0.0f, 1.0f) * 0.85f + 0.0f) / EaseIn(0.85f);
    const float radius = mTime < kIrisStart ? 0.0f : ImMax(1.0f, open * (far + size.y * 0.3f));
    IrisMask(dl, origin, origin + size, c, radius, size.y * 0.3f, IM_COL32_BLACK);
}

void CinematicReveal::DrawEffects(ImDrawList* dl, ImVec2 origin, ImVec2 size) {
    if (!mRunning) {
        return;
    }
    const float s = size.y / 1080.0f;
    const ImVec2 c = origin + size * 0.5f;
    const float t = mTime;

    mMotes.Draw(dl, 1.0f);

    // The light gathering in the middle: wide and dim at first, then tight and bright.
    const float gather = EaseInOut((t - kGatherStart) / (kIrisStart - kGatherStart));
    const float burst = ImClamp((t - kIrisStart) / kIrisLength, 0.0f, 1.0f);
    if (t >= kGatherStart && burst < 1.0f) {
        const float radius = size.y * (0.42f - 0.2f * gather) * (1.0f + 3.5f * EaseOut(burst));
        const float intensity = (0.25f + 0.85f * gather) * (1.0f - burst) + 0.6f * (1.0f - burst) * (burst > 0.0f);
        GoldenGlow(dl, c, radius, intensity, t);
    }

    // The Triforce, made of light, with time rippling around it.
    if (t >= kTriforceStart) {
        const float in = EaseOutBack(ImClamp((t - kTriforceStart) / 0.55f, 0.0f, 1.0f));
        const float out = 1.0f - EaseOut(burst);
        const float size3 = 230.0f * s * (0.7f + 0.3f * in) * (1.0f + 0.6f * EaseOut(burst));
        const float a = ImClamp((t - kTriforceStart) / 0.35f, 0.0f, 1.0f) * out;
        if (a > 0.0f) {
            Triforce(dl, c, size3, a, 1.0f);
            LensFlare(dl, c + ImVec2(0.0f, -size3 * 0.5f), 60.0f * s, a * Window(t, 1.25f, 0.9f, 0.25f), t);
        }
        TimeRipple(dl, c, size.y * 0.55f, (t - kTriforceStart - 0.15f) / 1.1f, 1.0f, s);
    }

    // The flash when the light bursts, fading as the menu shows.
    if (burst > 0.0f) {
        const float flash = (1.0f - burst) * (1.0f - burst);
        // Warm light flooding out of the middle, brightest where the Triforce was.
        ScreenFade(dl, origin, origin + size, Col::Rgba(255, 236, 190), flash * 0.3f);
        Glow(dl, c, size.x * (0.35f + 0.6f * EaseOut(burst)), Fade(Col::GoldPale, flash * 0.9f));
        Glow(dl, c, size.y * 0.35f, Fade(IM_COL32_WHITE, flash * 0.8f));
        TimeRipple(dl, c, size.x * 0.6f, burst * 0.9f + 0.05f, 0.8f, s);
    }
}

} // namespace Fx
} // namespace DefinitiveUI
