#pragma once
#include <algorithm>
#include <cmath>

namespace dvds {

class ADSREnvelope {
public:
    enum class Stage { Idle, Attack, Decay, Sustain, Release };

    void setSampleRate(double sr) { sampleRate_ = sr; recalc(); }

    void setAttack(float seconds)  { attack_ = std::max(seconds, 0.001f); recalc(); }
    void setDecay(float seconds)   { decay_ = std::max(seconds, 0.001f); recalc(); }
    void setSustain(float level)   { sustain_ = std::clamp(level, 0.0f, 1.0f); }
    void setRelease(float seconds) { release_ = std::max(seconds, 0.001f); recalc(); }

    void noteOn() {
        stage_ = Stage::Attack;
        if (level_ <= 0.001f) level_ = 0.0f;
    }

    void noteOff() {
        if (stage_ != Stage::Idle)
            stage_ = Stage::Release;
    }

    bool isActive() const { return stage_ != Stage::Idle; }
    float getLevel() const { return level_; }
    Stage getStage() const { return stage_; }

    float process() {
        switch (stage_) {
            case Stage::Idle:
                return 0.0f;
            case Stage::Attack:
                level_ += attackRate_;
                if (level_ >= 1.0f) {
                    level_ = 1.0f;
                    stage_ = Stage::Decay;
                }
                break;
            case Stage::Decay:
                level_ += (sustain_ - level_) * decayCoeff_;
                if (std::abs(level_ - sustain_) < 0.001f) {
                    level_ = sustain_;
                    stage_ = Stage::Sustain;
                }
                break;
            case Stage::Sustain:
                level_ = sustain_;
                break;
            case Stage::Release:
                level_ *= releaseCoeff_;
                if (level_ < 0.0001f) {
                    level_ = 0.0f;
                    stage_ = Stage::Idle;
                }
                break;
        }
        return level_;
    }

    void reset() {
        stage_ = Stage::Idle;
        level_ = 0.0f;
    }

private:
    double sampleRate_ = 44100.0;
    float attack_ = 0.01f;
    float decay_ = 0.1f;
    float sustain_ = 0.7f;
    float release_ = 0.3f;

    float attackRate_ = 0.0f;
    float decayCoeff_ = 0.0f;
    float releaseCoeff_ = 0.0f;
    float level_ = 0.0f;
    Stage stage_ = Stage::Idle;

    void recalc() {
        attackRate_ = 1.0f / (float)(attack_ * sampleRate_);
        decayCoeff_ = 1.0f - std::exp(-1.0f / (float)(decay_ * sampleRate_));
        releaseCoeff_ = 1.0f - 1.0f / (float)(release_ * sampleRate_);
        if (releaseCoeff_ > 0.99999f) releaseCoeff_ = 0.99999f;
    }
};

} // namespace dvds
