#include "Audio/EnvelopeFollower.h"
#include <algorithm>
#include <cmath>

namespace dvds {

EnvelopeFollower::EnvelopeFollower() {
    updateCoeffs();
}

void EnvelopeFollower::setSampleRate(double sr) {
    sampleRate_ = sr;
    updateCoeffs();
}

void EnvelopeFollower::setAttack(float ms) {
    attackMs_ = std::max(0.1f, ms);
    updateCoeffs();
}

void EnvelopeFollower::setRelease(float ms) {
    releaseMs_ = std::max(0.1f, ms);
    updateCoeffs();
}

void EnvelopeFollower::setPeakHoldTime(float ms) {
    peakHoldMs_ = std::max(0.0f, ms);
    peakHoldSamples_ = static_cast<int>(peakHoldMs_ * sampleRate_ / 1000.0);
}

void EnvelopeFollower::updateCoeffs() {
    attackCoeff_ = std::exp(-1.0f / (attackMs_ * 0.001f * static_cast<float>(sampleRate_)));
    releaseCoeff_ = std::exp(-1.0f / (releaseMs_ * 0.001f * static_cast<float>(sampleRate_)));
    peakHoldSamples_ = static_cast<int>(peakHoldMs_ * sampleRate_ / 1000.0);
    peakDecay_ = std::exp(-1.0f / (500.0f * 0.001f * static_cast<float>(sampleRate_)));
}

void EnvelopeFollower::process(float input) {
    float absInput = std::abs(input);

    if (absInput > envelope_)
        envelope_ = attackCoeff_ * envelope_ + (1.0f - attackCoeff_) * absInput;
    else
        envelope_ = releaseCoeff_ * envelope_ + (1.0f - releaseCoeff_) * absInput;

    smoothed_ = 0.95f * smoothed_ + 0.05f * envelope_;

    if (absInput >= peakHold_) {
        peakHold_ = absInput;
        peakHoldCounter_ = peakHoldSamples_;
    } else if (peakHoldCounter_ > 0) {
        --peakHoldCounter_;
    } else {
        peakHold_ *= peakDecay_;
    }
}

void EnvelopeFollower::reset() {
    envelope_ = 0.0f;
    smoothed_ = 0.0f;
    peakHold_ = 0.0f;
    peakHoldCounter_ = 0;
}

// -- MultiBandEnvelope --

MultiBandEnvelope::MultiBandEnvelope() {
    envelopes_.fill(0.0f);
}

void MultiBandEnvelope::setSampleRate(double sr) {
    for (auto& f : followers_) f.setSampleRate(sr);
}

void MultiBandEnvelope::setAttack(float ms) {
    for (auto& f : followers_) f.setAttack(ms);
}

void MultiBandEnvelope::setRelease(float ms) {
    for (auto& f : followers_) f.setRelease(ms);
}

void MultiBandEnvelope::processBand(int band, float energy) {
    if (band < 0 || band >= kNumBands) return;
    followers_[band].process(energy);
    envelopes_[band] = followers_[band].getEnvelope();
}

float MultiBandEnvelope::getEnvelope(int band) const {
    if (band < 0 || band >= kNumBands) return 0.0f;
    return envelopes_[band];
}

float MultiBandEnvelope::getPeak(int band) const {
    if (band < 0 || band >= kNumBands) return 0.0f;
    return followers_[band].getPeak();
}

void MultiBandEnvelope::reset() {
    for (auto& f : followers_) f.reset();
    envelopes_.fill(0.0f);
}

} // namespace dvds
