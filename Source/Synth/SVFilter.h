#pragma once
#include <cmath>
#include <algorithm>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace dvds {

enum class FilterType { LowPass, HighPass, BandPass, Notch, FilterTypeCount };

inline const char* filterTypeName(FilterType t) {
    switch (t) {
        case FilterType::LowPass:  return "Low Pass";
        case FilterType::HighPass: return "High Pass";
        case FilterType::BandPass: return "Band Pass";
        case FilterType::Notch:    return "Notch";
        default: return "?";
    }
}

// State-variable filter (Chamberlin topology, 2x oversampled)
class SVFilter {
public:
    void setSampleRate(double sr) { sampleRate_ = sr; }
    void setType(FilterType t) { type_ = t; }
    void setCutoff(float hz) { cutoff_ = std::clamp(hz, 20.0f, 20000.0f); }
    void setResonance(float r) { resonance_ = std::clamp(r, 0.0f, 1.0f); }
    void setKeyTracking(float amt) { keyTracking_ = amt; }

    void setNoteFreq(float noteHz) {
        noteFreq_ = noteHz;
    }

    void reset() {
        low_ = high_ = band_ = notch_ = 0.0f;
    }

    float process(float input) {
        float effectiveCutoff = cutoff_;
        if (keyTracking_ > 0.0f)
            effectiveCutoff += (noteFreq_ - 261.63f) * keyTracking_;
        effectiveCutoff = std::clamp(effectiveCutoff, 20.0f, (float)(sampleRate_ * 0.45));

        float f = 2.0f * (float)std::sin(M_PI * effectiveCutoff / sampleRate_);
        f = std::min(f, 0.99f);
        float q = 1.0f - resonance_ * 0.99f;

        // 2x oversample
        for (int i = 0; i < 2; ++i) {
            float in = (i == 0) ? input : input;
            low_ += f * band_;
            high_ = in - low_ - q * band_;
            band_ += f * high_;
            notch_ = high_ + low_;
        }

        switch (type_) {
            case FilterType::LowPass:  return low_;
            case FilterType::HighPass: return high_;
            case FilterType::BandPass: return band_;
            case FilterType::Notch:    return notch_;
            default: return input;
        }
    }

private:
    double sampleRate_ = 44100.0;
    FilterType type_ = FilterType::LowPass;
    float cutoff_ = 8000.0f;
    float resonance_ = 0.0f;
    float keyTracking_ = 0.0f;
    float noteFreq_ = 261.63f;
    float low_ = 0.0f, high_ = 0.0f, band_ = 0.0f, notch_ = 0.0f;
};

} // namespace dvds
