#pragma once
// ReChop - pattern engine (no JUCE dependency, so audio thread, GUI and MIDI export share one source of truth)
#include <vector>
#include <string>
#include <sstream>
#include <cstdint>
#include <cctype>
#include <cmath>
#include <algorithm>

namespace rechop
{
enum class Family { Dance = 0, Pop = 1, Synthwave = 2 };
enum class Kind   { Melodic, Slices };

struct Step
{
    bool on = false;
    int  degree = 1;      // 1 = tonic (melodic) or slice number (slices)
    bool lower = false;   // one octave down (melodic only)
    bool accent = false;
    int  len = 1;         // length in 16ths
};

struct Pattern
{
    std::string name, style;
    Family family = Family::Dance;
    Kind   kind   = Kind::Melodic;
    std::vector<Step> steps;   // one entry per 16th note
};

// Pattern text: one token per 16th, separated by spaces.
//   .   rest        _   hold previous note one more 16th
//   n   degree n (1 = tonic, 8 = tonic an octave up)  /  slice n
//   bn  same, one octave down       n!  accented
inline Pattern makePattern (const char* name, const char* style, Family f, Kind k, const char* dsl)
{
    Pattern p;
    p.name = name; p.style = style; p.family = f; p.kind = k;

    std::istringstream in (dsl);
    std::string tok;
    int lastOn = -1;

    while (in >> tok)
    {
        Step s;
        if (tok == ".")
        {
            lastOn = -1;
        }
        else if (tok == "_")
        {
            if (lastOn >= 0) p.steps[(size_t) lastOn].len++;
        }
        else
        {
            size_t i = 0;
            if (tok[i] == 'b') { s.lower = true; ++i; }
            int d = 0;
            while (i < tok.size() && std::isdigit ((unsigned char) tok[i])) d = d * 10 + (tok[i++] - '0');
            s.accent = (i < tok.size() && tok[i] == '!');
            s.degree = std::max (1, d);
            s.on = true;
            lastOn = (int) p.steps.size();
        }
        p.steps.push_back (s);
    }
    return p;
}

inline const std::vector<Pattern>& library()
{
    static const std::vector<Pattern> lib = []
    {
        using F = Family; using K = Kind;
        std::vector<Pattern> v;

        // ---------------- DANCE (EDM / Electro Dance / House / Deep House) ----------------
        v.push_back (makePattern ("Dotted Hook",     "EDM",          F::Dance, K::Melodic, "1! . . 1 . . 3! . . 5 . . 3 . 2 ."));
        v.push_back (makePattern ("Drop Stutter",    "EDM",          F::Dance, K::Melodic, "1! 1 1 1 1! 1 1 1 3! 3 3 3 5! 5 3 3"));
        v.push_back (makePattern ("Offbeat Stab",    "House",        F::Dance, K::Melodic, ". . 1! . . . 3! . . . 5! . . . 3! ."));
        v.push_back (makePattern ("Question Answer", "Deep House",   F::Dance, K::Melodic, "1! _ . 3 _ . 5 . 6! _ _ . 5 . . .   5! _ . 3 _ . 2 . 1! _ _ _ . . . ."));
        v.push_back (makePattern ("Rolling 16ths",   "Electro Dance",F::Dance, K::Melodic, "1! . 1 1 . 1 3! . 1! . 1 1 . 5! 3 ."));
        v.push_back (makePattern ("Riser Climb",     "EDM",          F::Dance, K::Melodic, "1 . 2 . 3 . 4 . 5 . 6 . 7 . 8! ."));
        v.push_back (makePattern ("Slice Shuffle",   "House",        F::Dance, K::Slices,  "1! . 2 . 3! . 2 . 4! . 1 . 3 . 2! ."));
        v.push_back (makePattern ("Slice Fill",      "Electro Dance",F::Dance, K::Slices,  "1! 1 2 2 3! 3 4 4 1! 1 2 2 5! 6 7 8"));

        // ---------------- POP (Pop / Retro Pop / Indie Pop / Dark Pop) ----------------
        v.push_back (makePattern ("Pop Hook",        "Pop",          F::Pop, K::Melodic, "1! . . 3 _ . 5 . . 3 . . 2 _ . .   1! . . 3 _ . 5 . 6! _ _ _ 5 . . ."));
        v.push_back (makePattern ("Airy Sustain",    "Pop",          F::Pop, K::Melodic, "1! _ _ _ _ _ _ _ 5! _ _ _ _ _ _ _"));
        v.push_back (makePattern ("Syncopated Hook", "Pop",          F::Pop, K::Melodic, ". . 1! . . 3 . . 5! . . 3 . 2 . ."));
        v.push_back (makePattern ("Call Response",   "Indie Pop",    F::Pop, K::Melodic, "3! _ 5 _ 6! _ 5 . . . . . . . . .   5! _ 3 _ 2! _ 1 _ _ _ . . . . . ."));
        v.push_back (makePattern ("Italo Bounce",    "Retro Pop",    F::Pop, K::Melodic, "1! . . 1 . 5 . 1! . . 1 . 5 . 3 ."));
        v.push_back (makePattern ("Indie Skip",      "Indie Pop",    F::Pop, K::Melodic, "1! . 3 . . 5 . 3 2! . . 1 . . 3 ."));
        v.push_back (makePattern ("Halftime Low",    "Dark Pop",     F::Pop, K::Melodic, "1! _ _ _ . . . . 3 _ . . . . . .   b5! _ _ _ . . . . 1 _ . . . . . ."));
        v.push_back (makePattern ("Pop Slice Hook",  "Pop",          F::Pop, K::Slices,  "1! . . 2 . . 3! . . 1 . 4! . . 2 ."));

        // ---------------- SYNTHWAVE ----------------
        v.push_back (makePattern ("Neon Arp",        "Synthwave",    F::Synthwave, K::Melodic, "1! . 3 . 5 . 8! . 5 . 3 . 5 . 3 ."));
        v.push_back (makePattern ("Drive 16ths",     "Synthwave",    F::Synthwave, K::Melodic, "1! 1 5 1 1! 1 5 1 b5! b5 1 b5 b5! b5 1 3"));
        v.push_back (makePattern ("Dreamy Pad",      "Synthwave",    F::Synthwave, K::Melodic, "1! _ _ _ _ _ _ _ 3 _ _ _ _ _ _ _   6! _ _ _ _ _ _ _ 5 _ _ _ _ _ _ _"));
        v.push_back (makePattern ("Outrun Line",     "Synthwave",    F::Synthwave, K::Melodic, "5! . . 5 . 6! . 5 . . 3 . . 3! . .   2 . . 2 . 3! . 2 . . 1! _ _ . . ."));
        v.push_back (makePattern ("Gated Pulse",     "Synthwave",    F::Synthwave, K::Melodic, "1! . 1 . 1! . 1 . 1! . 1 . 1! . 1 ."));
        v.push_back (makePattern ("Sunset Rise",     "Synthwave",    F::Synthwave, K::Melodic, "1! _ _ _ 2 _ _ _ 3 _ _ _ 4 _ _ _   5! _ _ _ 6 _ _ _ 7 _ _ _ 8! _ _ _"));
        v.push_back (makePattern ("Retro Slices",    "Synthwave",    F::Synthwave, K::Slices,  "1! . . 2 . . 3! . 1 . . 2 . 4! . ."));
        v.push_back (makePattern ("Echo Tail",       "Synthwave",    F::Synthwave, K::Melodic, "1! . . . . . 1 . . . . . 5! . . ."));
        return v;
    }();
    return lib;
}

// ---------------------------------------------------------------- scales
inline const char* scaleName (int i)
{
    static const char* n[] = { "Major", "Minor", "Harmonic Minor", "Dorian", "Phrygian" };
    return n[std::clamp (i, 0, 4)];
}

inline int scaleInterval (int scale, int idx)
{
    static const int t[5][7] = { { 0, 2, 4, 5, 7, 9, 11 },   // major
                                 { 0, 2, 3, 5, 7, 8, 10 },   // natural minor
                                 { 0, 2, 3, 5, 7, 8, 11 },   // harmonic minor ("Spanish minor")
                                 { 0, 2, 3, 5, 7, 9, 10 },   // dorian
                                 { 0, 1, 3, 5, 7, 8, 10 } }; // phrygian
    return t[std::clamp (scale, 0, 4)][std::clamp (idx, 0, 6)];
}

// ---------------------------------------------------------------- settings + events
struct Settings
{
    int   root = 0;          // 0..11  (C..B)
    int   scale = 1;         // see scaleName
    int   octave = 0;        // -2..2
    int   sliceBase = 36;    // MIDI note of slice 1 (C1 in Cubase naming)
    float density = 1.0f;    // 0..1
    float swing = 0.0f;      // 0..1
    float humanize = 0.0f;   // 0..1
    float gate = 1.0f;       // note length multiplier
    bool  fill = false;      // 16th stutter fill in the last beat of every 4th bar
};

struct Ev
{
    double start = 0.0;  // in 16ths, absolute (from host ppq 0)
    double len = 1.0;    // in 16ths
    int note = 60;
    int vel = 90;
};

inline float hash01 (uint32_t a, uint32_t b)
{
    uint32_t h = a * 2654435761u ^ (b + 0x9e3779b9u) * 2246822519u;
    h ^= h >> 15; h *= 2246822519u; h ^= h >> 13; h *= 3266489917u; h ^= h >> 16;
    return (float) (h & 0xFFFFFFu) / 16777216.0f;
}

inline int tonicNote (const Settings& s)
{
    const int r = s.root > 6 ? s.root - 12 : s.root;   // keep the tonic near C3 (note 60)
    return 60 + 12 * s.octave + r;
}

// Returns true if this 16th step carries a note. Delays only (never early), so a block scan can't miss events.
inline bool stepEvent (const Pattern& p, int patternId, const Settings& s, int64_t absStep, Ev& out)
{
    const int L = (int) p.steps.size();
    if (L == 0) return false;

    const int k = (int) (((absStep % L) + L) % L);
    const int posInBar = (int) (((absStep % 16) + 16) % 16);
    const int64_t bar = (absStep - posInBar) / 16;

    Step st = p.steps[(size_t) k];
    bool fillStep = false;

    if (s.fill && (((bar % 4) + 4) % 4) == 3 && posInBar >= 12)
    {
        fillStep = true;
        for (const auto& q : p.steps) if (q.on) { st = q; break; }
        st.on = true; st.len = 1; st.accent = false;
    }

    if (! st.on) return false;

    if (! fillStep && s.density < 0.999f)
    {
        const float h = hash01 ((uint32_t) patternId + 1u, (uint32_t) k);
        const float thr = st.accent ? (s.density > 0.12f ? 1.0f : 0.0f) : s.density;
        if (! (h < thr)) return false;
    }

    int note;
    if (p.kind == Kind::Melodic)
    {
        const int d = st.degree - 1;
        int semis = scaleInterval (s.scale, d % 7) + 12 * (d / 7);
        if (st.lower) semis -= 12;
        note = tonicNote (s) + semis;
    }
    else
    {
        note = s.sliceBase + (st.degree - 1);
    }
    note = std::clamp (note, 0, 127);

    const uint32_t absLow = (uint32_t) (absStep & 0xFFFFFFFF);
    double delay = (absStep & 1) ? (double) s.swing * 0.5 : 0.0;
    delay += (double) s.humanize * 0.10 * hash01 (777u, absLow);

    int vel = st.accent ? 118 : 92;
    if (fillStep) vel = 80 + (posInBar - 12) * 12;
    vel += (int) ((hash01 (4242u, absLow) - 0.5f) * 2.0f * s.humanize * 16.0f);
    vel = std::clamp (vel, 1, 127);

    out.start = (double) absStep + delay;
    out.len   = std::max (0.1, (double) st.len * (double) s.gate);
    out.note  = note;
    out.vel   = vel;
    return true;
}
} // namespace rechop
