#pragma once
#include "Audio/FFTAnalyzer.h"
#include <array>
#include <vector>
#include <atomic>

namespace dvds {

class BeatDetector {
public:
    BeatDetector();

    void setSampleRate(double sampleRate);
    void setThreshold(float threshold);
    void setSensitivity(float sensitivity);

    void process(const FFTAnalyzer& fft);

    bool isBeat() const { return beatDetected_.load(); }
    float getBPM() const { return bpm_.load(); }
    float getBeatPhase() const { return beatPhase_.load(); }
    float getSpectralFlux() const { return spectralFlux_; }
    float getOnsetStrength() const { return onsetStrength_; }

    void clearBeat() { beatDetected_.store(false); }

private:
    double sampleRate_ = 44100.0;
    float threshold_ = 1.5f;
    float sensitivity_ = 0.6f;

    std::array<float, FFTAnalyzer::kBinCount> prevSpectrum_{};
    float spectralFlux_ = 0.0f;
    float onsetStrength_ = 0.0f;
    float avgFlux_ = 0.0f;

    std::atomic<bool> beatDetected_{ false };
    std::atomic<float> bpm_{ 120.0f };
    std::atomic<float> beatPhase_{ 0.0f };

    // BPM estimation via onset history
    static constexpr int kOnsetHistorySize = 256;
    std::vector<float> onsetHistory_;
    int onsetWritePos_ = 0;
    int framesSinceLastBeat_ = 0;
    std::vector<int> beatIntervals_;

    void estimateBPM();
};

} // namespace dvds
