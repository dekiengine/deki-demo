/**
 * @file ParticleNodeGizmos.cpp
 * @brief Pictures of particle nodes, drawn in the Node Graph properties panel.
 *
 * "radius 0.4, angle 0 to 6.28" is a fact about a node, not a picture of it.
 * These draw what each node MEANS: the shape particles spawn in, the arc they
 * leave along, the ramp a size follows, the ramp a color fades down. The band
 * sits between the node's title and its fields and redraws every frame from
 * the live instance, so it answers a slider while the slider is being dragged.
 *
 * Two rules shape everything here:
 *
 *  - PROPORTION, NOT MEASURE. A gizmo auto-fits its band, so a 0.2 m circle and
 *    a 5 m circle are drawn the same size. What it shows is the SHAPE and the
 *    relationships between values (this edge twice that one, this arc a third
 *    of the way round, this size doubling as it ages); the numeric value is in
 *    the row directly underneath and needs no second copy. True-to-scale
 *    belongs in the live preview, which has a px/m slider and draws the
 *    emitter's shape at the same scale as the particles leaving it.
 *
 *  - EVERY CURVE IS A POLYLINE. NodeGraphPreviewCanvas offers a filled circle,
 *    a filled rect and a line, and that is deliberately all: a provider lives
 *    in another DLL and draws through function pointers, so the primitive set
 *    is a compatibility surface and not a place to add a shape whenever one is
 *    convenient. Arcs, rings, outlines and gradients here are built from those
 *    three.
 */

#ifdef DEKI_EDITOR

#include "ParticleNodes.h"

#include "deki-nodegraph/NodeGraphPreview.h"

#include <cmath>
#include <cstdint>

namespace
{

constexpr float kPi    = DekiMath::kPi;
constexpr float kTwoPi = DekiMath::kTwoPi;

// ---------------------------------------------------------------------------
// Palette. Local on purpose: a package must not reach into the editor's theme
// for drawing it does through the canvas ops, and these read against the dark
// recessed band the window puts behind a gizmo.
// ---------------------------------------------------------------------------

inline uint32_t Ink(uint8_t a)      { return NodeGraphPreviewRgba(122, 190, 255, a); }  // the subject
inline uint32_t Warm(uint8_t a)     { return NodeGraphPreviewRgba(255, 186, 110, a); }  // motion / vectors
inline uint32_t Neutral(uint8_t a)  { return NodeGraphPreviewRgba(255, 255, 255, a); }  // axes, particles

// ---------------------------------------------------------------------------
// Drawing helpers over the three primitives.
// ---------------------------------------------------------------------------

// Angles follow the domain's convention (0 = +X, counter-clockwise, +Y up), so
// every conversion to screen flips Y in one place: here.
struct Painter
{
    const NodeGraphPreviewCanvas& c;
    float dpi = 1.0f;

    void Line(float x0, float y0, float x1, float y1, uint32_t col, float th = 1.0f) const
    {
        c.line(c.ctx, x0, y0, x1, y1, col, th * dpi);
    }
    void Dot(float x, float y, float r, uint32_t col) const
    {
        if (r > 0.0f) c.circleFilled(c.ctx, x, y, r, col);
    }
    void FillRect(float x0, float y0, float x1, float y1, uint32_t col) const
    {
        c.rectFilled(c.ctx, x0, y0, x1, y1, col);
    }
    void Rect(float x0, float y0, float x1, float y1, uint32_t col, float th = 1.0f) const
    {
        Line(x0, y0, x1, y0, col, th);
        Line(x1, y0, x1, y1, col, th);
        Line(x1, y1, x0, y1, col, th);
        Line(x0, y1, x0, y0, col, th);
    }

    // Segment count from the radius: a 4 px ring needs nothing like the 40
    // pieces a 60 px one does, and a fixed count is either coarse or wasteful.
    int SegmentsFor(float r, float sweep) const
    {
        int n = static_cast<int>(r * std::fabs(sweep) * 0.25f);
        if (n < 8)  n = 8;
        if (n > 96) n = 96;
        return n;
    }

    void Arc(float cx, float cy, float r, float a0, float a1, uint32_t col, float th = 1.0f) const
    {
        if (r <= 0.0f) return;
        const int seg = SegmentsFor(r, a1 - a0);
        float px = cx + std::cos(a0) * r;
        float py = cy - std::sin(a0) * r;
        for (int i = 1; i <= seg; ++i)
        {
            const float a = a0 + (a1 - a0) * (static_cast<float>(i) / static_cast<float>(seg));
            const float qx = cx + std::cos(a) * r;
            const float qy = cy - std::sin(a) * r;
            Line(px, py, qx, qy, col, th);
            px = qx;
            py = qy;
        }
    }
    void Ring(float cx, float cy, float r, uint32_t col, float th = 1.0f) const
    {
        Arc(cx, cy, r, 0.0f, kTwoPi, col, th);
    }

    // Ray from (cx,cy) at a domain angle.
    void Ray(float cx, float cy, float a, float r0, float r1, uint32_t col, float th = 1.0f) const
    {
        const float ca = std::cos(a), sa = std::sin(a);
        Line(cx + ca * r0, cy - sa * r0, cx + ca * r1, cy - sa * r1, col, th);
    }

    void Arrow(float x0, float y0, float x1, float y1, uint32_t col, float th = 1.5f) const
    {
        Line(x0, y0, x1, y1, col, th);
        const float dx = x1 - x0, dy = y1 - y0;
        const float len = std::sqrt(dx * dx + dy * dy);
        if (len < 1.0f) return;
        const float ux = dx / len, uy = dy / len;
        float head = 7.0f * dpi;
        if (head > len * 0.45f) head = len * 0.45f;
        const float wx = -uy * head * 0.55f, wy = ux * head * 0.55f;
        Line(x1, y1, x1 - ux * head + wx, y1 - uy * head + wy, col, th);
        Line(x1, y1, x1 - ux * head - wx, y1 - uy * head - wy, col, th);
    }

    // The emitter origin: a small cross, the same mark in every gizmo that has
    // one, so "this is where the effect is" never has to be worked out.
    void Origin(float cx, float cy) const
    {
        const float k = 4.0f * dpi;
        Line(cx - k, cy, cx + k, cy, Neutral(90), 1.0f);
        Line(cx, cy - k, cx, cy + k, Neutral(90), 1.0f);
    }

    // A square standing in for one particle, rotated by a domain angle. Squares
    // rather than dots because a dot cannot show rotation.
    void ParticleSquare(float cx, float cy, float half, float angle, uint32_t col, float th = 1.5f) const
    {
        const float ca = std::cos(angle), sa = std::sin(angle);
        float px[4], py[4];
        const float ox[4] = { -half, half, half, -half };
        const float oy[4] = { -half, -half, half, half };
        for (int i = 0; i < 4; ++i)
        {
            px[i] = cx + ox[i] * ca - oy[i] * sa;
            py[i] = cy - (ox[i] * sa + oy[i] * ca);
        }
        for (int i = 0; i < 4; ++i)
            Line(px[i], py[i], px[(i + 1) % 4], py[(i + 1) % 4], col, th);
    }
};

// Stable scatter for "particles spawn in here". Fixed at first use rather than
// re-rolled per frame: dots that crawl while a radius is dragged read as the
// effect doing something, which is exactly what a gizmo must not invent.
struct Sample { float x, y; };

const Sample* UnitSamples(int& count)
{
    static Sample s[28];
    static bool built = false;
    if (!built)
    {
        uint32_t seed = 0x9E3779B9u;
        auto next = [&seed]() {
            seed = seed * 1664525u + 1013904223u;
            return static_cast<float>((seed >> 8) & 0xFFFFFFu) * (1.0f / 16777216.0f);
        };
        for (Sample& p : s)
        {
            p.x = next() * 2.0f - 1.0f;
            p.y = next() * 2.0f - 1.0f;
        }
        built = true;
    }
    count = static_cast<int>(sizeof(s) / sizeof(s[0]));
    return s;
}

// ---------------------------------------------------------------------------
// Emission: where particles are born.
// ---------------------------------------------------------------------------

// True when an Emission node has no extent to draw: a Point shape, or one of
// the sized shapes left at zero, which spawns from a single spot just the same.
// Both the drawing and the band's height ask this, so it is asked in one place.
bool EmissionIsPointLike(const ParticleEmissionNode& n)
{
    switch (n.shape)
    {
        case EmitterShapeKind::Circle: return n.radius <= 0.0f;
        case EmitterShapeKind::Rect:   return n.width <= 0.0f && n.height <= 0.0f;
        case EmitterShapeKind::Point:
        default:                       return true;
    }
}

void DrawEmission(const ParticleEmissionNode& n, const Painter& p,
                  float x, float y, float w, float h)
{
    const float pad = 12.0f * p.dpi;
    const float cx = x + w * 0.5f;
    const float cy = y + h * 0.5f;
    const float roomX = w * 0.5f - pad;
    const float roomY = h * 0.5f - pad;

    if (EmissionIsPointLike(n))
    {
        // Every particle starts at one place. Rings pulsing outward would be a
        // lie about direction, so this is just the spot, marked.
        p.Ring(cx, cy, 9.0f * p.dpi, Ink(70), 1.0f);
        p.Dot(cx, cy, 3.5f * p.dpi, Ink(235));
        p.Origin(cx, cy);
        return;
    }

    int sampleCount = 0;
    const Sample* samples = UnitSamples(sampleCount);
    const float dotR = 1.6f * p.dpi;

    switch (n.shape)
    {
        case EmitterShapeKind::Circle:
        {
            const float r = (roomX < roomY ? roomX : roomY);
            p.Dot(cx, cy, r, Ink(26));
            p.Ring(cx, cy, r, Ink(230), 1.5f);
            for (int i = 0; i < sampleCount; ++i)
            {
                // Rejection-free: push the square samples onto the disc.
                const float sx = samples[i].x, sy = samples[i].y;
                const float len = std::sqrt(sx * sx + sy * sy);
                if (len > 1.0f || len <= 0.0001f) continue;
                p.Dot(cx + sx * r, cy - sy * r, dotR, Neutral(150));
            }
            // The radius itself, as the measured thing it is.
            p.Arrow(cx, cy, cx + r, cy, Warm(220), 1.5f);
            p.Origin(cx, cy);
            return;
        }
        case EmitterShapeKind::Rect:
        {
            // Fit the box preserving its aspect, so a wide emitter looks wide -
            // and so one side left at zero draws the line emitter it really is.
            const float rw = n.width  > 0.0f ? n.width  * 0.5f : 0.0001f;
            const float rh = n.height > 0.0f ? n.height * 0.5f : 0.0001f;
            const float scale = (roomX / rw < roomY / rh) ? (roomX / rw) : (roomY / rh);
            const float hw = rw * scale;
            const float hh = rh * scale;
            p.FillRect(cx - hw, cy - hh, cx + hw, cy + hh, Ink(26));
            p.Rect(cx - hw, cy - hh, cx + hw, cy + hh, Ink(230), 1.5f);
            for (int i = 0; i < sampleCount; ++i)
                p.Dot(cx + samples[i].x * hw, cy - samples[i].y * hh, dotR, Neutral(150));
            p.Arrow(cx, cy, cx + hw, cy, Warm(220), 1.5f);
            p.Arrow(cx, cy, cx, cy - hh, Warm(220), 1.5f);
            p.Origin(cx, cy);
            return;
        }
        case EmitterShapeKind::Point:
        default:
            break;   // point-like, and already drawn above
    }
}

// ---------------------------------------------------------------------------
// Initial Velocity: the arc particles leave along, and how fast.
// ---------------------------------------------------------------------------

void DrawInitialVelocity(const ParticleInitialVelocityNode& n, const Painter& p,
                         float x, float y, float w, float h)
{
    const float pad = 14.0f * p.dpi;
    const float cx = x + w * 0.5f;
    const float cy = y + h * 0.5f;
    float R = (w * 0.5f - pad < h * 0.5f - pad) ? (w * 0.5f - pad) : (h * 0.5f - pad);
    if (R < 4.0f) return;

    float a0 = n.angleMin, a1 = n.angleMax;
    if (a1 < a0) { const float t = a0; a0 = a1; a1 = t; }
    const bool full = (a1 - a0) >= kTwoPi - 0.001f;
    if (full) { a0 = 0.0f; a1 = kTwoPi; }

    // Speeds set the ring radii. Sign is direction, not distance, so the rings
    // use magnitude and the arrows point the way the sign says.
    const float m0 = std::fabs(n.speedMin), m1 = std::fabs(n.speedMax);
    const float mMax = (m0 > m1 ? m0 : m1);
    const bool inward = (n.speedMin + n.speedMax) < 0.0f;

    if (mMax <= 0.0001f)
    {
        // No speed: particles stay where they are born. Say that plainly.
        p.Ring(cx, cy, R * 0.25f, Ink(50), 1.0f);
        p.Dot(cx, cy, 3.5f * p.dpi, Ink(200));
        p.Origin(cx, cy);
        return;
    }

    float rIn  = R * ((m0 < m1 ? m0 : m1) / mMax);
    float rOut = R;
    if (rIn > rOut) rIn = rOut;

    // The band of possible speeds, then its edges.
    if (rOut - rIn > 1.0f)
    {
        const int seg = 26;
        for (int i = 0; i <= seg; ++i)
        {
            const float a = a0 + (a1 - a0) * (static_cast<float>(i) / static_cast<float>(seg));
            p.Ray(cx, cy, a, rIn, rOut, Ink(30), 1.0f);
        }
    }
    p.Arc(cx, cy, rOut, a0, a1, Ink(230), 1.5f);
    if (rIn > 1.0f)
        p.Arc(cx, cy, rIn, a0, a1, Ink(110), 1.0f);
    if (!full)
    {
        p.Ray(cx, cy, a0, 0.0f, rOut, Ink(120), 1.0f);
        p.Ray(cx, cy, a1, 0.0f, rOut, Ink(120), 1.0f);
    }

    // A handful of sample particles along the sweep, at the fast edge.
    const int arrows = full ? 8 : 5;
    for (int i = 0; i < arrows; ++i)
    {
        const float t = (arrows == 1) ? 0.5f
                                      : static_cast<float>(i) / static_cast<float>(arrows - 1);
        const float a = full ? (a0 + (a1 - a0) * (static_cast<float>(i) / static_cast<float>(arrows)))
                             : (a0 + (a1 - a0) * t);
        const float ca = std::cos(a), sa = std::sin(a);
        const float tipR = inward ? rIn : rOut;
        const float tailR = inward ? rOut : rIn;
        p.Arrow(cx + ca * tailR, cy - sa * tailR,
                cx + ca * tipR,  cy - sa * tipR, Warm(225), 1.5f);
    }
    p.Origin(cx, cy);
}

// ---------------------------------------------------------------------------
// Initial Rotation: the angles particles are born at, and the spin they carry.
// ---------------------------------------------------------------------------

void DrawInitialRotation(const ParticleInitialRotationNode& n, const Painter& p,
                         float x, float y, float w, float h)
{
    const float cx = x + w * 0.5f;
    const float cy = y + h * 0.5f;
    const float pad = 14.0f * p.dpi;
    float R = (w * 0.5f - pad < h * 0.5f - pad) ? (w * 0.5f - pad) : (h * 0.5f - pad);
    if (R < 6.0f) return;
    const float half = R * 0.52f;

    // The two ends of the birth-angle range. Identical values draw one square
    // over the other, which is the honest picture of "every particle the same".
    p.ParticleSquare(cx, cy, half, n.rotationMin, Neutral(80), 1.0f);
    p.ParticleSquare(cx, cy, half, n.rotationMax, Ink(235), 1.5f);

    // Spin, as an arc around the squares with an arrowhead the way it turns.
    const float spin = (n.spinSpeedMin + n.spinSpeedMax) * 0.5f;
    if (std::fabs(spin) > 0.0001f)
    {
        const float rr = R * 0.94f;
        float sweep = spin / (2.0f * kTwoPi);          // full range = a full turn
        if (sweep >  1.0f) sweep =  1.0f;
        if (sweep < -1.0f) sweep = -1.0f;
        sweep *= kTwoPi * 0.75f;
        const float start = kPi * 0.5f;
        p.Arc(cx, cy, rr, start, start + sweep, Warm(210), 1.5f);
        // Arrowhead: a short chord at the moving end, along the tangent.
        const float aEnd = start + sweep;
        const float tangent = aEnd + (sweep > 0.0f ? kPi * 0.5f : -kPi * 0.5f);
        const float ex = cx + std::cos(aEnd) * rr;
        const float ey = cy - std::sin(aEnd) * rr;
        p.Arrow(ex - std::cos(tangent) * 6.0f * p.dpi, ey + std::sin(tangent) * 6.0f * p.dpi,
                ex, ey, Warm(210), 1.5f);
    }
}

// ---------------------------------------------------------------------------
// Gravity: a constant pull, drawn as the fall it produces.
// ---------------------------------------------------------------------------

void DrawGravity(const ParticleGravityNode& n, const Painter& p,
                 float x, float y, float w, float h)
{
    const float cx = x + w * 0.5f;
    const float cy = y + h * 0.5f;
    const float pad = 14.0f * p.dpi;
    float R = (w * 0.5f - pad < h * 0.5f - pad) ? (w * 0.5f - pad) : (h * 0.5f - pad);
    if (R < 6.0f) return;

    // Faint axes, so a sideways pull is visibly sideways.
    p.Line(cx - R, cy, cx + R, cy, Neutral(24), 1.0f);
    p.Line(cx, cy - R, cx, cy + R, Neutral(24), 1.0f);

    const float mag = std::sqrt(n.gravityX * n.gravityX + n.gravityY * n.gravityY);
    if (mag <= 0.0001f)
    {
        p.Ring(cx, cy, 8.0f * p.dpi, Ink(70), 1.0f);
        p.Dot(cx, cy, 3.0f * p.dpi, Ink(200));
        return;
    }

    // Length by magnitude, saturating: 20 m/s^2 is already twice earth, and a
    // gizmo that keeps growing past the band tells you less, not more.
    float t = mag / 20.0f;
    if (t > 1.0f) t = 1.0f;
    const float len = R * (0.25f + 0.75f * t);
    const float ux =  n.gravityX / mag;
    const float uy = -n.gravityY / mag;   // domain Y is up, screen Y is down

    // Three ghosts spaced as t^2, which is what constant acceleration does.
    const float startX = cx - ux * len * 0.55f;
    const float startY = cy - uy * len * 0.55f;
    for (int i = 1; i <= 3; ++i)
    {
        const float f = static_cast<float>(i) / 3.0f;
        const float d = len * 1.55f * f * f;
        p.Dot(startX + ux * d, startY + uy * d, 2.6f * p.dpi,
              Neutral(static_cast<uint8_t>(60 + 55 * i)));
    }
    p.Arrow(cx, cy, cx + ux * len, cy + uy * len, Warm(235), 2.0f);
}

// ---------------------------------------------------------------------------
// Plot helpers for the curve-shaped nodes (drag, size, spin).
// ---------------------------------------------------------------------------

struct Plot
{
    float x0, y0, x1, y1;   // the plotting rect
    float W() const { return x1 - x0; }
    float H() const { return y1 - y0; }
};

Plot PlotRect(const Painter& p, float x, float y, float w, float h)
{
    const float padX = 14.0f * p.dpi;
    const float padY = 12.0f * p.dpi;
    return Plot{ x + padX, y + padY, x + w - padX, y + h - padY };
}

// The life axis every one of these shares: birth at the left, death at the
// right, with the ends ticked so the direction is not a guess.
void DrawLifeAxis(const Painter& p, const Plot& pl, float baselineY)
{
    p.Line(pl.x0, baselineY, pl.x1, baselineY, Neutral(34), 1.0f);
    const float tick = 3.0f * p.dpi;
    p.Line(pl.x0, baselineY - tick, pl.x0, baselineY + tick, Neutral(60), 1.0f);
    p.Line(pl.x1, baselineY - tick, pl.x1, baselineY + tick, Neutral(60), 1.0f);
}

// ---------------------------------------------------------------------------
// Drag: speed decaying over the seconds after birth.
// ---------------------------------------------------------------------------

void DrawDrag(const ParticleDragNode& n, const Painter& p,
              float x, float y, float w, float h)
{
    const Plot pl = PlotRect(p, x, y, w, h);
    if (pl.W() < 8.0f || pl.H() < 8.0f) return;

    DrawLifeAxis(p, pl, pl.y1);

    // Two seconds of it: long enough that a drag of 1 has visibly bled off,
    // short enough that a drag of 8 is not a vertical wall against the axis.
    const int steps = 48;
    float prevX = pl.x0, prevY = pl.y0;
    for (int i = 0; i <= steps; ++i)
    {
        const float f = static_cast<float>(i) / static_cast<float>(steps);
        const float v = std::exp(-n.drag * (f * 2.0f));
        const float px = pl.x0 + pl.W() * f;
        const float py = pl.y1 - pl.H() * v;
        p.FillRect(px, py, px + pl.W() / steps + 1.0f, pl.y1, Ink(22));
        if (i > 0)
            p.Line(prevX, prevY, px, py, Ink(235), 1.5f);
        prevX = px;
        prevY = py;
    }

    // Ghost particles at equal times: the bunching IS the slowing down.
    for (int i = 1; i <= 4; ++i)
    {
        const float f = static_cast<float>(i) / 4.0f;
        const float v = std::exp(-n.drag * (f * 2.0f));
        p.Dot(pl.x0 + pl.W() * f, pl.y1 - pl.H() * v, 2.4f * p.dpi, Warm(200));
    }
}

// ---------------------------------------------------------------------------
// Size over Lifetime: the taper, with particles drawn along it.
// ---------------------------------------------------------------------------

void DrawSizeOverLifetime(const ParticleSizeOverLifetimeNode& n, const Painter& p,
                          float x, float y, float w, float h)
{
    const Plot pl = PlotRect(p, x, y, w, h);
    if (pl.W() < 8.0f || pl.H() < 8.0f) return;

    const float midY = (pl.y0 + pl.y1) * 0.5f;
    DrawLifeAxis(p, pl, midY);

    float sMax = (n.sizeAt0 > n.sizeAt1 ? n.sizeAt0 : n.sizeAt1);
    if (sMax <= 0.0001f)
    {
        // Zero at both ends: nothing is ever drawn. Show the axis and stop.
        p.Dot(pl.x0, midY, 2.0f * p.dpi, Ink(160));
        p.Dot(pl.x1, midY, 2.0f * p.dpi, Ink(160));
        return;
    }

    const float maxHalf = pl.H() * 0.46f;
    const int steps = 40;
    float prevTop = 0.0f, prevBot = 0.0f, prevX = 0.0f;
    for (int i = 0; i <= steps; ++i)
    {
        const float f = static_cast<float>(i) / static_cast<float>(steps);
        const float s = n.sizeAt0 + (n.sizeAt1 - n.sizeAt0) * f;
        const float half = maxHalf * (s / sMax);
        const float px = pl.x0 + pl.W() * f;
        p.FillRect(px, midY - half, px + pl.W() / steps + 1.0f, midY + half, Ink(26));
        if (i > 0)
        {
            p.Line(prevX, prevTop, px, midY - half, Ink(235), 1.5f);
            p.Line(prevX, prevBot, px, midY + half, Ink(235), 1.5f);
        }
        prevX = px;
        prevTop = midY - half;
        prevBot = midY + half;
    }

    for (int i = 0; i <= 4; ++i)
    {
        const float f = static_cast<float>(i) / 4.0f;
        const float s = n.sizeAt0 + (n.sizeAt1 - n.sizeAt0) * f;
        p.Dot(pl.x0 + pl.W() * f, midY, maxHalf * (s / sMax) * 0.8f, Neutral(45));
    }
}

// ---------------------------------------------------------------------------
// Color over Lifetime: the ramp, alpha included.
// ---------------------------------------------------------------------------

void DrawColorOverLifetime(const ParticleColorOverLifetimeNode& n, const Painter& p,
                           float x, float y, float w, float h)
{
    const float padX = 14.0f * p.dpi;
    const float padY = 12.0f * p.dpi;
    const float x0 = x + padX, x1 = x + w - padX;
    const float y0 = y + padY, y1 = y + h - padY;
    if (x1 - x0 < 8.0f || y1 - y0 < 6.0f) return;

    // Checkerboard first: a fade to transparent and a fade to black look the
    // same on a flat backing, and telling them apart is the whole point of
    // having alpha in this node.
    const float sq = 6.0f * p.dpi;
    int row = 0;
    for (float cy = y0; cy < y1; cy += sq, ++row)
    {
        int col = row;
        for (float cx = x0; cx < x1; cx += sq, ++col)
        {
            const float ex = (cx + sq < x1) ? cx + sq : x1;
            const float ey = (cy + sq < y1) ? cy + sq : y1;
            p.FillRect(cx, cy, ex, ey,
                       (col & 1) ? NodeGraphPreviewRgba(58, 58, 62, 255)
                                 : NodeGraphPreviewRgba(42, 42, 46, 255));
        }
    }

    // Lerped by hand rather than through Color::Lerp: the same channel-wise
    // blend the runtime modifier does, and it keeps this file from depending on
    // an engine symbol being exported to package DLLs.
    const int steps = 64;
    const float sw = (x1 - x0) / static_cast<float>(steps);
    const int r0 = n.colorAt0.r, g0 = n.colorAt0.g, b0 = n.colorAt0.b, a0 = n.colorAt0.a;
    const int dr = static_cast<int>(n.colorAt1.r) - r0;
    const int dg = static_cast<int>(n.colorAt1.g) - g0;
    const int db = static_cast<int>(n.colorAt1.b) - b0;
    const int da = static_cast<int>(n.colorAt1.a) - a0;
    for (int i = 0; i < steps; ++i)
    {
        const float f = (static_cast<float>(i) + 0.5f) / static_cast<float>(steps);
        p.FillRect(x0 + sw * i, y0, x0 + sw * (i + 1) + 1.0f, y1,
                   NodeGraphPreviewRgba(static_cast<uint8_t>(r0 + dr * f),
                                        static_cast<uint8_t>(g0 + dg * f),
                                        static_cast<uint8_t>(b0 + db * f),
                                        static_cast<uint8_t>(a0 + da * f)));
    }
    p.Rect(x0, y0, x1, y1, Neutral(40), 1.0f);
}

// ---------------------------------------------------------------------------
// Rotation over Lifetime: the spin-rate ramp, and the turning it adds up to.
// ---------------------------------------------------------------------------

void DrawRotationOverLifetime(const ParticleRotationOverLifetimeNode& n, const Painter& p,
                              float x, float y, float w, float h)
{
    const Plot pl = PlotRect(p, x, y, w, h);
    if (pl.W() < 8.0f || pl.H() < 8.0f) return;

    const float midY = (pl.y0 + pl.y1) * 0.5f;
    DrawLifeAxis(p, pl, midY);

    const float a0 = std::fabs(n.spinSpeedAt0), a1 = std::fabs(n.spinSpeedAt1);
    const float aMax = (a0 > a1 ? a0 : a1);
    const float half = pl.H() * 0.42f;

    if (aMax > 0.0001f)
    {
        // The rate ramp, above the line for counter-clockwise and below for
        // clockwise, so a sign flip is visible as a crossing.
        const float py0 = midY - half * (n.spinSpeedAt0 / aMax);
        const float py1 = midY - half * (n.spinSpeedAt1 / aMax);
        p.Line(pl.x0, py0, pl.x1, py1, Ink(235), 1.5f);
        const int steps = 24;
        for (int i = 0; i <= steps; ++i)
        {
            const float f = static_cast<float>(i) / static_cast<float>(steps);
            const float px = pl.x0 + pl.W() * f;
            p.Line(px, midY, px, py0 + (py1 - py0) * f, Ink(26), 1.0f);
        }
    }

    // What that rate ADDS UP TO over one second of life: the integral of the
    // ramp. A rate plot alone never shows that a small rate held for a whole
    // life is still half a turn.
    const float squareHalf = 5.0f * p.dpi;
    for (int i = 0; i <= 3; ++i)
    {
        const float f = static_cast<float>(i) / 3.0f;
        const float theta = n.spinSpeedAt0 * f +
                            (n.spinSpeedAt1 - n.spinSpeedAt0) * f * f * 0.5f;
        p.ParticleSquare(pl.x0 + pl.W() * f, midY, squareHalf, theta,
                         Warm(static_cast<uint8_t>(120 + 35 * i)), 1.5f);
    }
}

// ---------------------------------------------------------------------------
// Dispatch
// ---------------------------------------------------------------------------

constexpr uint32_t kEmissionId  = DekiHashString(ParticleEmissionNode::StaticNodeName);
constexpr uint32_t kVelocityId  = DekiHashString(ParticleInitialVelocityNode::StaticNodeName);
constexpr uint32_t kRotationId  = DekiHashString(ParticleInitialRotationNode::StaticNodeName);
constexpr uint32_t kGravityId   = DekiHashString(ParticleGravityNode::StaticNodeName);
constexpr uint32_t kDragId      = DekiHashString(ParticleDragNode::StaticNodeName);
constexpr uint32_t kSizeId      = DekiHashString(ParticleSizeOverLifetimeNode::StaticNodeName);
constexpr uint32_t kColorId     = DekiHashString(ParticleColorOverLifetimeNode::StaticNodeName);
constexpr uint32_t kSpinId      = DekiHashString(ParticleRotationOverLifetimeNode::StaticNodeName);

float GizmoHeight(uint32_t typeId, const void* instance)
{
    if (!instance)
        return 0.0f;
    if (typeId == kEmissionId)
    {
        // A dot does not need the room a shape does; the band shrinks with it
        // rather than framing one mark in a lot of empty plate.
        return EmissionIsPointLike(*static_cast<const ParticleEmissionNode*>(instance))
             ? 76.0f : 132.0f;
    }
    if (typeId == kVelocityId) return 132.0f;
    if (typeId == kRotationId) return 104.0f;
    if (typeId == kGravityId)  return 112.0f;
    if (typeId == kDragId)     return 88.0f;
    if (typeId == kSizeId)     return 92.0f;
    if (typeId == kColorId)    return 56.0f;
    if (typeId == kSpinId)     return 92.0f;
    return 0.0f;   // Emitter, and any node a picture would not help.
}

void GizmoDraw(uint32_t typeId, const void* instance,
               float x, float y, float w, float h, float dpi,
               const NodeGraphPreviewCanvas& canvas)
{
    if (!instance || !canvas.line || !canvas.circleFilled || !canvas.rectFilled)
        return;

    const Painter p{ canvas, dpi > 0.0f ? dpi : 1.0f };

    if (typeId == kEmissionId)
        DrawEmission(*static_cast<const ParticleEmissionNode*>(instance), p, x, y, w, h);
    else if (typeId == kVelocityId)
        DrawInitialVelocity(*static_cast<const ParticleInitialVelocityNode*>(instance), p, x, y, w, h);
    else if (typeId == kRotationId)
        DrawInitialRotation(*static_cast<const ParticleInitialRotationNode*>(instance), p, x, y, w, h);
    else if (typeId == kGravityId)
        DrawGravity(*static_cast<const ParticleGravityNode*>(instance), p, x, y, w, h);
    else if (typeId == kDragId)
        DrawDrag(*static_cast<const ParticleDragNode*>(instance), p, x, y, w, h);
    else if (typeId == kSizeId)
        DrawSizeOverLifetime(*static_cast<const ParticleSizeOverLifetimeNode*>(instance), p, x, y, w, h);
    else if (typeId == kColorId)
        DrawColorOverLifetime(*static_cast<const ParticleColorOverLifetimeNode*>(instance), p, x, y, w, h);
    else if (typeId == kSpinId)
        DrawRotationOverLifetime(*static_cast<const ParticleRotationOverLifetimeNode*>(instance), p, x, y, w, h);
}

} // namespace

NodeGraphNodeGizmoOps DekiParticles_GizmoOps()
{
    NodeGraphNodeGizmoOps ops;
    ops.height = &GizmoHeight;
    ops.draw   = &GizmoDraw;
    return ops;
}

#endif // DEKI_EDITOR
