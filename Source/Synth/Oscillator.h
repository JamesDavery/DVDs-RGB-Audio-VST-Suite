#pragma once
#include <cmath>
#include <cstdlib>
#include <array>
#include <algorithm>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace dvds {

enum class WaveShape { Sine, Saw, Square, Triangle, Noise, WaveShapeCount };
enum class SynthEngine { Subtractive, FM, Wavetable, EngineCount };

inline const char* waveShapeName(WaveShape w) {
    switch (w) {
        case WaveShape::Sine:     return "Sine";
        case WaveShape::Saw:      return "Saw";
        case WaveShape::Square:   return "Square";
        case WaveShape::Triangle: return "Triangle";
        case WaveShape::Noise:    return "Noise";
        default: return "?";
    }
}

inline const char* synthEngineName(SynthEngine e) {
    switch (e) {
        case SynthEngine::Subtractive: return "Subtractive";
        case SynthEngine::FM:          return "FM";
        case SynthEngine::Wavetable:   return "Wavetable";
        default: return "?";
    }
}

class Oscillator {
public:
    void setSampleRate(double sr) { sampleRate_ = sr; }
    void setFrequency(double freq) { frequency_ = freq; }
    void setShape(WaveShape s) { shape_ = s; }
    void setDetune(float cents) { detuneFactor_ = std::pow(2.0, cents / 1200.0); }
    void setPulseWidth(float pw) { pulseWidth_ = std::clamp(pw, 0.05f, 0.95f); }
    void reset() { phase_ = 0.0; }

    float process() {
        double freq = frequency_ * detuneFactor_;
        double inc = freq / sampleRate_;
        float out = 0.0f;

        switch (shape_) {
            case WaveShape::Sine:
                out = (float)std::sin(phase_ * 2.0 * M_PI);
                break;
            case WaveShape::Saw:
                out = polyBLEPSaw();
                break;
            case WaveShape::Square:
                out = polyBLEPSquare();
                break;
            case WaveShape::Triangle:
                out = polyBLEPTriangle();
                break;
            case WaveShape::Noise:
                out = ((float)rand() / (float)RAND_MAX) * 2.0f - 1.0f;
                break;
            default:
                break;
        }

        phase_ += inc;
        if (phase_ >= 1.0) phase_ -= 1.0;
        return out;
    }

private:
    double sampleRate_ = 44100.0;
    double frequency_ = 440.0;
    double phase_ = 0.0;
    double detuneFactor_ = 1.0;
    float pulseWidth_ = 0.5f;
    WaveShape shape_ = WaveShape::Saw;

    float polyBLEP(double t) const {
        double dt = frequency_ * detuneFactor_ / sampleRate_;
        if (t < dt) {
            t /= dt;
            return (float)(t + t - t * t - 1.0);
        } else if (t > 1.0 - dt) {
            t = (t - 1.0) / dt;
            return (float)(t * t + t + t + 1.0);
        }
        return 0.0f;
    }

    float polyBLEPSaw() {
        float out = (float)(2.0 * phase_ - 1.0);
        out -= polyBLEP(phase_);
        return out;
    }

    float polyBLEPSquare() {
        float out = (phase_ < (double)pulseWidth_) ? 1.0f : -1.0f;
        out += polyBLEP(phase_);
        double shiftedPhase = phase_ + (1.0 - (double)pulseWidth_);
        if (shiftedPhase >= 1.0) shiftedPhase -= 1.0;
        out -= polyBLEP(shiftedPhase);
        return out;
    }

    float polyBLEPTriangle() {
        float sq = polyBLEPSquare();
        triState_ = 0.99f * triState_ + (float)(4.0 * frequency_ * detuneFactor_ / sampleRate_) * sq;
        return triState_;
    }

    float triState_ = 0.0f;
};

// Wavetable with 256-sample frames
class WavetableOscillator {
public:
    static constexpr int kFrameSize = 256;
    static constexpr int kMaxFrames = 64;

    void setSampleRate(double sr) { sampleRate_ = sr; }
    void setFrequency(double freq) { frequency_ = freq; }
    void setPosition(float pos) { position_ = std::clamp(pos, 0.0f, 1.0f); }
    void reset() { phase_ = 0.0; }

    void initDefaultTables() {
        numFrames_ = 8;
        for (int f = 0; f < numFrames_; ++f) {
            for (int i = 0; i < kFrameSize; ++i) {
                double t = (double)i / kFrameSize;
                double val = 0.0;
                int harmonics = 1 + f * 4;
                for (int h = 1; h <= harmonics; ++h) {
                    val += std::sin(t * 2.0 * M_PI * h) / h;
                }
                table_[f][i] = (float)(val * 0.5);
            }
        }
    }

    float process() {
        double inc = frequency_ / sampleRate_;
        float framePos = position_ * (float)(numFrames_ - 1);
        int f0 = std::clamp((int)framePos, 0, numFrames_ - 1);
        int f1 = std::min(f0 + 1, numFrames_ - 1);
        float frac = framePos - (float)f0;

        double idx = phase_ * kFrameSize;
        int i0 = (int)idx;
        int i1 = (i0 + 1) & (kFrameSize - 1);
        float iFrac = (float)(idx - i0);
        i0 &= (kFrameSize - 1);

        float s0 = table_[f0][i0] + (table_[f0][i1] - table_[f0][i0]) * iFrac;
        float s1 = table_[f1][i0] + (table_[f1][i1] - table_[f1][i0]) * iFrac;
        float out = s0 + (s1 - s0) * frac;

        phase_ += inc;
        if (phase_ >= 1.0) phase_ -= 1.0;
        return out;
    }

private:
    double sampleRate_ = 44100.0;
    double frequency_ = 440.0;
    double phase_ = 0.0;
    float position_ = 0.0f;
    int numFrames_ = 1;
    float table_[kMaxFrames][kFrameSize]{};
};

// FM Operator
class FMOperator {
public:
    void setSampleRate(double sr) { sampleRate_ = sr; }
    void setFrequency(double freq) { frequency_ = freq; }
    void setRatio(float r) { ratio_ = r; }
    void setIndex(float idx) { modIndex_ = idx; }
    void setFeedback(float fb) { feedback_ = fb; }
    void reset() { phase_ = 0.0; lastOut_ = 0.0f; }

    float process(float modInput = 0.0f) {
        double freq = frequency_ * (double)ratio_;
        double mod = (double)modInput + (double)(lastOut_ * feedback_);
        float out = (float)std::sin(phase_ * 2.0 * M_PI + mod * (double)modIndex_);
        lastOut_ = out;
        phase_ += freq / sampleRate_;
        if (phase_ >= 1.0) phase_ -= 1.0;
        return out;
    }

private:
    double sampleRate_ = 44100.0;
    double frequency_ = 440.0;
    double phase_ = 0.0;
    float ratio_ = 1.0f;
    float modIndex_ = 1.0f;
    float feedback_ = 0.0f;
    float lastOut_ = 0.0f;
};

} // namespace dvds
