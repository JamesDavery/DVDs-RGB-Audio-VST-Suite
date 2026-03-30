#include "Audio/BeatDetector.h"
#include <cmath>
#include <algorithm>
#include <numeric>

namespace dvds {

BeatDetector::BeatDetector() {
    prevSpectrum_.fill(0.0f);
    onsetHistory_.resize(kOnsetHistorySize, 0.0f);
}

void BeatDetector::setSampleRate(double sr) {
    sampleRate_ = sr;
}

void BeatDetector::setThreshold(float t) {
    threshold_ = std::max(0.5f, t);
}

void BeatDetector::setSensitivity(float s) {
    sensitivity_ = std::clamp(s, 0.0f, 1.0f);
}

void BeatDetector::process(const FFTAnalyzer& fft) {
    const auto& spectrum = fft.getMagnitudeSpectrum();

    // Compute spectral flux (half-wave rectified difference)
    float flux = 0.0f;
    for (int i = 0; i < FFTAnalyzer::kBinCount; ++i) {
        float diff = spectrum[i] - prevSpectrum_[i];
        if (diff > 0.0f) flux += diff;
        prevSpectrum_[i] = spectrum[i];
    }
    spectralFlux_ = flux;

    // Adaptive threshold via running average
    onsetHistory_[onsetWritePos_] = flux;
    onsetWritePos_ = (onsetWritePos_ + 1) % kOnsetHistorySize;

    float avgFlux = 0.0f;
    for (float f : onsetHistory_) avgFlux += f;
    avgFlux /= kOnsetHistorySize;
    avgFlux_ = avgFlux;

    float adaptiveThreshold = avgFlux * threshold_ * (2.0f - sensitivity_);
    onsetStrength_ = (avgFlux > 0.0001f) ? flux / avgFlux : 0.0f;

    ++framesSinceLastBeat_;

    // Minimum inter-beat interval (~200 BPM max at typical FFT frame rate)
    int minInterval = 4;

    if (flux > adaptiveThreshold && framesSinceLastBeat_ > minInterval) {
        beatDetected_.store(true);
        beatIntervals_.push_back(framesSinceLastBeat_);
        if (beatIntervals_.size() > 32)
            beatIntervals_.erase(beatIntervals_.begin());
        framesSinceLastBeat_ = 0;
        estimateBPM();
    } else {
        beatDetected_.store(false);
    }

    // Update phase
    float currentBpm = bpm_.load();
    if (currentBpm > 0.0f) {
        float framesPerBeat = static_cast<float>(sampleRate_) / (FFTAnalyzer::kFFTSize) * 60.0f / currentBpm;
        float phase = static_cast<float>(framesSinceLastBeat_) / std::max(framesPerBeat, 1.0f);
        beatPhase_.store(std::fmod(phase, 1.0f));
    }
}

void BeatDetector::estimateBPM() {
    if (beatIntervals_.size() < 4) return;

    // Median interval
    auto sorted = beatIntervals_;
    std::sort(sorted.begin(), sorted.end());
    float medianInterval = static_cast<float>(sorted[sorted.size() / 2]);

    if (medianInterval > 0.0f) {
        float framesPerSecond = static_cast<float>(sampleRate_) / FFTAnalyzer::kFFTSize;
        float beatsPerSecond = framesPerSecond / medianInterval;
        float newBpm = beatsPerSecond * 60.0f;

        // Clamp to reasonable range
        newBpm = std::clamp(newBpm, 40.0f, 220.0f);

        // Smooth
        float currentBpm = bpm_.load();
        bpm_.store(currentBpm * 0.8f + newBpm * 0.2f);
    }
}

} // namespace dvds
