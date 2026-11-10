#pragma once
// Sons internos (bateria + riser) para pré-escuta. DSP puro; o ruído usa um LCG de semente fixa (não é geração musical).
#include <array>
#include <cmath>
#include <cstdint>

#include "Zenite.h"

namespace zdsp {

constexpr double kTwoPi = 6.283185307179586;

struct Noise {
    uint32_t s = 22222u;
    float next() {
        s = s * 1664525u + 1013904223u;
        return (float) (s >> 8) * (1.0f / 8388608.0f) - 1.0f;
    }
};

struct DrumVoice {
    bool on = false;
    zenite::Piece piece = zenite::Piece::Kick;
    double age = 0, len = 0;  // em amostras
    float vel = 1.f;
    double phase = 0;
    float lp1 = 0, lp2 = 0;
    Noise noise;
};

// Grupos do mixer: 0 Bombo, 1 Caixa/Palmas, 2 Hats/Perc, 3 Pratos/FX
inline int groupOf(zenite::Piece p) {
    using P = zenite::Piece;
    switch (p) {
        case P::Kick: return 0;
        case P::Snare: case P::Clap: case P::TomL: case P::TomM: case P::TomH: return 1;
        case P::HatC: case P::HatO: case P::Rim: case P::Shaker: return 2;
        default: return 3;
    }
}

class DrumKit {
public:
    std::array<float, 4> groupGain{{1.f, 1.f, 1.f, 1.f}};

    void trigger(zenite::Piece p, int velocity, double lenSamples) {
        auto& v = voices[(size_t) p];
        v.on = true;
        v.piece = p;
        v.age = 0;
        v.len = lenSamples;
        v.vel = std::pow(std::max(1, std::min(127, velocity)) / 127.0f, 1.3f);
        v.phase = 0;
        v.lp1 = v.lp2 = 0;
        if (p == zenite::Piece::HatC) voices[(size_t) zenite::Piece::HatO].on = false;  // choke
    }

    void reset() { for (auto& v : voices) v.on = false; }

    void render(float* L, float* R, int n, double sr) {
        for (auto& v : voices) {
            if (!v.on) continue;
            const float g = v.vel * groupGain[(size_t) groupOf(v.piece)];
            const float hpA = hpCoef(v.piece, sr);
            for (int i = 0; i < n && v.on; ++i) {
                const double t = v.age / sr;
                float o = sample(v, t, sr, hpA);
                v.age += 1.0;
                o *= g;
                L[i] += o;
                R[i] += o;
            }
        }
    }

private:
    std::array<DrumVoice, zenite::kNumPieces> voices;

    static float hpCoef(zenite::Piece p, double sr) {
        double fc = 6000;
        using P = zenite::Piece;
        if (p == P::Snare) fc = 1500;
        if (p == P::Clap) fc = 1200;
        if (p == P::Crash || p == P::RevCrash) fc = 3500;
        if (p == P::Shaker) fc = 5000;
        return (float) (1.0 - std::exp(-kTwoPi * fc / sr));
    }

    static float hp(DrumVoice& v, float x, float a) {
        v.lp1 += a * (x - v.lp1);
        return x - v.lp1;
    }

    float sample(DrumVoice& v, double t, double sr, float hpA) {
        using P = zenite::Piece;
        float n = v.noise.next();
        float out = 0;
        switch (v.piece) {
            case P::Kick: {
                double f = 46.0 + 120.0 * std::exp(-t * 30.0);
                v.phase += kTwoPi * f / sr;
                out = (float) (std::sin(v.phase) * std::exp(-t * 7.5)) * 1.0f + n * (float) std::exp(-t * 500.0) * 0.18f;
                if (t > 0.55) v.on = false;
                break;
            }
            case P::Snare: {
                v.phase += kTwoPi * 190.0 / sr;
                out = (float) (std::sin(v.phase) * std::exp(-t * 26.0)) * 0.45f + hp(v, n, hpA) * (float) std::exp(-t * 17.0) * 0.75f;
                if (t > 0.4) v.on = false;
                break;
            }
            case P::Clap: {
                double e = 0;
                for (int k = 0; k < 3; ++k) {
                    double tk = t - k * 0.011;
                    if (tk >= 0) e += std::exp(-tk * 260.0);
                }
                e += std::exp(-t * 20.0) * 0.5;
                v.lp2 += 0.45f * (hp(v, n, hpA) - v.lp2);
                out = v.lp2 * (float) e * 0.9f;
                if (t > 0.35) v.on = false;
                break;
            }
            case P::HatC:
                out = hp(v, n, hpA) * (float) std::exp(-t * 75.0) * 0.55f;
                if (t > 0.12) v.on = false;
                break;
            case P::HatO:
                out = hp(v, n, hpA) * (float) std::exp(-t * 11.0) * 0.5f;
                if (t > 0.6) v.on = false;
                break;
            case P::Rim:
                v.phase += kTwoPi * 1750.0 / sr;
                out = (float) (std::sin(v.phase) * std::exp(-t * 130.0)) * 0.5f + n * (float) std::exp(-t * 220.0) * 0.3f;
                if (t > 0.1) v.on = false;
                break;
            case P::TomL: case P::TomM: case P::TomH: {
                double base = v.piece == P::TomL ? 100.0 : v.piece == P::TomM ? 140.0 : 190.0;
                v.phase += kTwoPi * base * (1.0 + 0.6 * std::exp(-t * 22.0)) / sr;
                out = (float) (std::sin(v.phase) * std::exp(-t * 9.0)) * 0.8f;
                if (t > 0.7) v.on = false;
                break;
            }
            case P::Crash:
                out = hp(v, n, hpA) * (float) std::exp(-t * 2.3) * 0.5f;
                if (t > 2.6) v.on = false;
                break;
            case P::Shaker:
                out = hp(v, n, hpA) * (float) (std::min(1.0, t / 0.012) * std::exp(-t * 50.0)) * 0.5f;
                if (t > 0.2) v.on = false;
                break;
            case P::Impact: {
                double f = 28.0 + 55.0 * std::exp(-t * 2.0);
                v.phase += kTwoPi * f / sr;
                v.lp2 += 0.08f * (n - v.lp2);
                out = (float) (std::sin(v.phase) * std::exp(-t * 2.2)) * 0.9f + v.lp2 * (float) std::exp(-t * 3.0) * 1.2f;
                if (t > 2.2) v.on = false;
                break;
            }
            case P::RevCrash: {
                double x = v.len > 1 ? v.age / v.len : 1.0;
                if (x >= 1.0) { v.on = false; break; }
                out = hp(v, n, hpA) * (float) std::pow(x, 2.2) * 0.55f;
                break;
            }
        }
        return out;
    }
};

// Riser: saws desafinados + ruído, filtro passa-baixo que abre e tom que sobe até à tónica.
class RiserSynth {
public:
    void reset() { s1 = s2 = 0; ph[0] = ph[1] = ph[2] = 0; }

    // type: 0 ruído, 1 saw, 2 ambos
    void render(float* L, float* R, int n, double sr, const double* amp, const double* cutHz, const double* pitchHz, int type, float gain) {
        for (int i = 0; i < n; ++i) {
            if (amp[i] <= 0.0) { s1 = s2 = 0; continue; }
            float sig = 0;
            if (type != 0) {
                for (int k = 0; k < 3; ++k) {
                    double det = 1.0 + (k - 1) * 0.008;
                    ph[k] += pitchHz[i] * det / sr;
                    ph[k] -= std::floor(ph[k]);
                    sig += (float) (2.0 * ph[k] - 1.0) * 0.28f;
                }
            }
            if (type != 1) sig += noise.next() * (type == 2 ? 0.35f : 0.8f);
            // SVF TPT passa-baixo
            double fc = std::min(cutHz[i], 0.45 * sr);
            double gC = std::tan(3.141592653589793 * fc / sr);
            double k = 0.9;
            double a1 = 1.0 / (1.0 + gC * (gC + k)), a2 = gC * a1, a3 = gC * a2;
            double v3 = sig - s2;
            double v1 = a1 * s1 + a2 * v3;
            double v2 = s2 + a2 * s1 + a3 * v3;
            s1 = 2 * v1 - s1;
            s2 = 2 * v2 - s2;
            float o = (float) (v2 * amp[i]) * gain;
            L[i] += o;
            R[i] += o;
        }
    }

private:
    double ph[3] = {0, 0, 0};
    double s1 = 0, s2 = 0;
    Noise noise;
};

}  // namespace zdsp
