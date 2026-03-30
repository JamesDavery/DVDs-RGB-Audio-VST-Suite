#pragma once
#include "Audio/FFTAnalyzer.h"
#include <array>
#include <string>

namespace dvds {

enum class FrequencyBand {
    Sub = 0,     // 20-60 Hz
    Low,         // 60-250 Hz
    Mid,         // 250-2000 Hz
    HighMid,     // 2-6 kHz
    Presence,    // 6-12 kHz
    Air,         // 12-20 kHz
    Count
};

static constexpr int kNumBands = static_cast<int>(FrequencyBand::Count);

struct BandRange {
    float lowFreq;
    float highFreq;
    const char* name;
};

const BandRange& getBandRange(FrequencyBand band);
std::string bandToString(FrequencyBand band);

class BandSplitter {
public:
    BandSplitter();

    void setSampleRate(double sampleRate);
    void analyze(const FFTAnalyzer& fft);

    float getBandEnergy(FrequencyBand band) const;
    float getBandEnergy(int band) const;
    const std::array<float, kNumBands>& getAllBandEnergies() const { return bandEnergy_; }

    float getNormalizedBandEnergy(FrequencyBand band) const;
    const std::array<float, kNumBands>& getAllNormalizedEnergies() const { return normalizedEnergy_; }

private:
    double sampleRate_ = 44100.0;
    std::array<float, kNumBands> bandEnergy_{};
    std::array<float, kNumBands> normalizedEnergy_{};
    std::array<float, kNumBands> maxObserved_{};
};

} // namespace dvds
