#include "Audio/BandSplitter.h"
#include <cmath>
#include <algorithm>

namespace dvds {

static const BandRange kBandRanges[] = {
    { 20.0f,    60.0f,   "Sub"      },
    { 60.0f,    250.0f,  "Low"      },
    { 250.0f,   2000.0f, "Mid"      },
    { 2000.0f,  6000.0f, "High-Mid" },
    { 6000.0f,  12000.0f,"Presence" },
    { 12000.0f, 20000.0f,"Air"      }
};

const BandRange& getBandRange(FrequencyBand band) {
    return kBandRanges[static_cast<int>(band)];
}

std::string bandToString(FrequencyBand band) {
    return kBandRanges[static_cast<int>(band)].name;
}

BandSplitter::BandSplitter() {
    bandEnergy_.fill(0.0f);
    normalizedEnergy_.fill(0.0f);
    maxObserved_.fill(0.001f);
}

void BandSplitter::setSampleRate(double sr) {
    sampleRate_ = sr;
}

void BandSplitter::analyze(const FFTAnalyzer& fft) {
    const auto& spectrum = fft.getMagnitudeSpectrum();

    for (int b = 0; b < kNumBands; ++b) {
        const auto& range = kBandRanges[b];
        int binLow = fft.frequencyToBin(range.lowFreq);
        int binHigh = fft.frequencyToBin(range.highFreq);
        binLow = std::clamp(binLow, 0, FFTAnalyzer::kBinCount - 1);
        binHigh = std::clamp(binHigh, binLow, FFTAnalyzer::kBinCount - 1);

        float sum = 0.0f;
        int count = 0;
        for (int i = binLow; i <= binHigh; ++i) {
            sum += spectrum[i];
            ++count;
        }
        bandEnergy_[b] = (count > 0) ? sum / count : 0.0f;

        // Adaptive normalization with slow decay
        maxObserved_[b] = std::max(maxObserved_[b] * 0.9999f, bandEnergy_[b]);
        normalizedEnergy_[b] = (maxObserved_[b] > 0.0001f)
            ? std::clamp(bandEnergy_[b] / maxObserved_[b], 0.0f, 1.0f)
            : 0.0f;
    }
}

float BandSplitter::getBandEnergy(FrequencyBand band) const {
    return bandEnergy_[static_cast<int>(band)];
}

float BandSplitter::getBandEnergy(int band) const {
    if (band >= 0 && band < kNumBands) return bandEnergy_[band];
    return 0.0f;
}

float BandSplitter::getNormalizedBandEnergy(FrequencyBand band) const {
    return normalizedEnergy_[static_cast<int>(band)];
}

} // namespace dvds
