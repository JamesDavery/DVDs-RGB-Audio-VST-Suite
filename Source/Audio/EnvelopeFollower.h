#pragma once
#include <array>
#include <cmath>

namespace dvds {

class EnvelopeFollower {
public:
    EnvelopeFollower();

    void setSampleRate(double sampleRate);
    void setAttack(float ms);
    void setRelease(float ms);
    void setPeakHoldTime(float ms);

    void process(float input);
    float getEnvelope() const { return envelope_; }
    float getPeak() const { return peakHold_; }
    float getSmoothed() const { return smoothed_; }

    void reset();

private:
    void updateCoeffs();

    double sampleRate_ = 44100.0;
    float attackMs_ = 5.0f;
    float releaseMs_ = 100.0f;
    float peakHoldMs_ = 200.0f;

    float attackCoeff_ = 0.0f;
    float releaseCoeff_ = 0.0f;
    float envelope_ = 0.0f;
    float smoothed_ = 0.0f;
    float peakHold_ = 0.0f;
    float peakDecay_ = 0.0f;
    int peakHoldSamples_ = 0;
    int peakHoldCounter_ = 0;
};

class MultiBandEnvelope {
public:
    static constexpr int kNumBands = 6;

    MultiBandEnvelope();

    void setSampleRate(double sampleRate);
    void setAttack(float ms);
    void setRelease(float ms);

    void processBand(int band, float energy);
    float getEnvelope(int band) const;
    float getPeak(int band) const;
    const std::array<float, kNumBands>& getAllEnvelopes() const { return envelopes_; }

    void reset();

private:
    std::array<EnvelopeFollower, kNumBands> followers_;
    std::array<float, kNumBands> envelopes_{};
};

} // namespace dvds
