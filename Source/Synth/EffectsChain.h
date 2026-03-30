#pragma once
#include <cmath>
#include <array>
#include <algorithm>
#include <cstring>
#include <vector>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace dvds {

// Simple delay line
class DelayLine {
public:
    void init(int maxSamples) {
        buffer_.resize((size_t)maxSamples, 0.0f);
        maxSize_ = maxSamples;
        writePos_ = 0;
    }

    void clear() {
        std::fill(buffer_.begin(), buffer_.end(), 0.0f);
    }

    void write(float sample) {
        buffer_[(size_t)writePos_] = sample;
        writePos_ = (writePos_ + 1) % maxSize_;
    }

    float read(float delaySamples) const {
        float readPos = (float)writePos_ - delaySamples;
        while (readPos < 0) readPos += (float)maxSize_;
        int i0 = (int)readPos;
        int i1 = (i0 + 1) % maxSize_;
        float frac = readPos - (float)i0;
        return buffer_[(size_t)(i0 % maxSize_)] * (1.0f - frac) +
               buffer_[(size_t)i1] * frac;
    }

private:
    std::vector<float> buffer_;
    int writePos_ = 0;
    int maxSize_ = 1;
};

// Stereo delay
class StereoDelay {
public:
    void setSampleRate(double sr) {
        sampleRate_ = sr;
        int maxSamples = (int)(sr * 2.0);
        delayL_.init(maxSamples);
        delayR_.init(maxSamples);
    }

    void setTime(float seconds) { delaySamples_ = seconds * (float)sampleRate_; }
    void setFeedback(float fb) { feedback_ = std::clamp(fb, 0.0f, 0.95f); }
    void setMix(float m) { mix_ = std::clamp(m, 0.0f, 1.0f); }
    void setPingPong(bool pp) { pingPong_ = pp; }

    void clear() { delayL_.clear(); delayR_.clear(); }

    void process(float& left, float& right) {
        float dL = delayL_.read(delaySamples_);
        float dR = delayR_.read(delaySamples_ * (pingPong_ ? 0.75f : 1.0f));

        float fbL = pingPong_ ? dR : dL;
        float fbR = pingPong_ ? dL : dR;

        delayL_.write(left + fbL * feedback_);
        delayR_.write(right + fbR * feedback_);

        left  = left  * (1.0f - mix_) + dL * mix_;
        right = right * (1.0f - mix_) + dR * mix_;
    }

private:
    double sampleRate_ = 44100.0;
    DelayLine delayL_, delayR_;
    float delaySamples_ = 22050.0f;
    float feedback_ = 0.3f;
    float mix_ = 0.2f;
    bool pingPong_ = false;
};

// Schroeder reverb (4 comb + 2 allpass)
class Reverb {
public:
    void setSampleRate(double sr) {
        sampleRate_ = sr;
        float scale = (float)(sr / 44100.0);
        for (int i = 0; i < 4; ++i)
            combs_[i].init((int)(combTimes_[i] * scale));
        for (int i = 0; i < 2; ++i)
            allpasses_[i].init((int)(apTimes_[i] * scale));
        clear();
    }

    void setRoomSize(float r) { roomSize_ = std::clamp(r, 0.0f, 1.0f); updateFeedback(); }
    void setDamping(float d) { damping_ = std::clamp(d, 0.0f, 1.0f); }
    void setMix(float m) { mix_ = std::clamp(m, 0.0f, 1.0f); }

    void clear() {
        for (auto& c : combs_) c.clear();
        for (auto& a : allpasses_) a.clear();
        for (auto& d : combDamp_) d = 0.0f;
    }

    void process(float& left, float& right) {
        float input = (left + right) * 0.5f;
        float out = 0.0f;

        for (int i = 0; i < 4; ++i) {
            float d = combs_[i].read((float)combs_[i].maxSize());
            combDamp_[i] = d * (1.0f - damping_) + combDamp_[i] * damping_;
            combs_[i].write(input + combDamp_[i] * combFeedback_);
            out += d;
        }
        out *= 0.25f;

        for (int i = 0; i < 2; ++i) {
            float d = allpasses_[i].read((float)allpasses_[i].maxSize());
            allpasses_[i].write(out + d * 0.5f);
            out = d - out * 0.5f;
        }

        float wet = out;
        left  = left  * (1.0f - mix_) + wet * mix_;
        right = right * (1.0f - mix_) + wet * mix_;
    }

private:
    class SimpleDelay {
    public:
        void init(int size) { buf_.resize((size_t)size, 0.0f); size_ = size; wp_ = 0; }
        void clear() { std::fill(buf_.begin(), buf_.end(), 0.0f); }
        void write(float s) { buf_[(size_t)wp_] = s; wp_ = (wp_ + 1) % size_; }
        float read(float delay) const {
            int rp = wp_ - (int)delay;
            while (rp < 0) rp += size_;
            return buf_[(size_t)(rp % size_)];
        }
        int maxSize() const { return size_; }
    private:
        std::vector<float> buf_;
        int size_ = 1, wp_ = 0;
    };

    double sampleRate_ = 44100.0;
    float roomSize_ = 0.5f, damping_ = 0.3f, mix_ = 0.2f;
    float combFeedback_ = 0.7f;
    SimpleDelay combs_[4], allpasses_[2];
    float combDamp_[4]{};

    static constexpr int combTimes_[4] = { 1116, 1188, 1277, 1356 };
    static constexpr int apTimes_[2] = { 556, 441 };

    void updateFeedback() {
        combFeedback_ = 0.5f + roomSize_ * 0.45f;
    }
};

// Chorus/Flanger (modulated delay)
class Chorus {
public:
    void setSampleRate(double sr) {
        sampleRate_ = sr;
        delayL_.init((int)(sr * 0.1));
        delayR_.init((int)(sr * 0.1));
    }

    void setRate(float hz) { rate_ = hz; }
    void setDepth(float ms) { depth_ = ms; }
    void setMix(float m) { mix_ = std::clamp(m, 0.0f, 1.0f); }
    void clear() { delayL_.clear(); delayR_.clear(); phase_ = 0.0; }

    void process(float& left, float& right) {
        double inc = rate_ / sampleRate_;
        float lfo = (float)std::sin(phase_ * 2.0 * M_PI);
        float lfo2 = (float)std::sin((phase_ + 0.25) * 2.0 * M_PI);

        float delayMs = 7.0f + depth_ * lfo;
        float delayMs2 = 7.0f + depth_ * lfo2;
        float delaySampL = delayMs * 0.001f * (float)sampleRate_;
        float delaySampR = delayMs2 * 0.001f * (float)sampleRate_;

        delayL_.write(left);
        delayR_.write(right);

        float wetL = delayL_.read(delaySampL);
        float wetR = delayR_.read(delaySampR);

        left  = left  * (1.0f - mix_) + wetL * mix_;
        right = right * (1.0f - mix_) + wetR * mix_;

        phase_ += inc;
        if (phase_ >= 1.0) phase_ -= 1.0;
    }

private:
    double sampleRate_ = 44100.0;
    DelayLine delayL_, delayR_;
    double phase_ = 0.0;
    float rate_ = 1.0f, depth_ = 3.0f, mix_ = 0.2f;
};

// Phaser (6-stage allpass)
class Phaser {
public:
    void setSampleRate(double sr) { sampleRate_ = sr; }
    void setRate(float hz) { rate_ = hz; }
    void setDepth(float d) { depth_ = std::clamp(d, 0.0f, 1.0f); }
    void setMix(float m) { mix_ = std::clamp(m, 0.0f, 1.0f); }

    void process(float& left, float& right) {
        double inc = rate_ / sampleRate_;
        float lfo = (float)std::sin(phase_ * 2.0 * M_PI) * 0.5f + 0.5f;
        float freq = 200.0f + lfo * depth_ * 3000.0f;
        float w = 2.0f * (float)std::sin(M_PI * freq / sampleRate_);

        float inL = left, inR = right;
        for (int s = 0; s < 6; ++s) {
            float a = (1.0f - w) / (1.0f + w);
            float outL = a * inL + stateL_[s];
            stateL_[s] = inL - a * outL;
            inL = outL;

            float outR = a * inR + stateR_[s];
            stateR_[s] = inR - a * outR;
            inR = outR;
        }

        left  = left  * (1.0f - mix_) + inL * mix_;
        right = right * (1.0f - mix_) + inR * mix_;

        phase_ += inc;
        if (phase_ >= 1.0) phase_ -= 1.0;
    }

private:
    double sampleRate_ = 44100.0;
    double phase_ = 0.0;
    float rate_ = 0.5f, depth_ = 0.5f, mix_ = 0.3f;
    float stateL_[6]{}, stateR_[6]{};
};

// Waveshaper distortion
class Distortion {
public:
    void setDrive(float d) { drive_ = std::clamp(d, 0.0f, 1.0f); }
    void setMix(float m) { mix_ = std::clamp(m, 0.0f, 1.0f); }

    void process(float& left, float& right) {
        float gain = 1.0f + drive_ * 20.0f;
        float dryL = left, dryR = right;

        left  = std::tanh(left * gain);
        right = std::tanh(right * gain);

        left  = dryL * (1.0f - mix_) + left * mix_;
        right = dryR * (1.0f - mix_) + right * mix_;
    }

private:
    float drive_ = 0.0f, mix_ = 0.0f;
};

// 3-band EQ (low shelf, mid peak, high shelf)
class ThreeBandEQ {
public:
    void setSampleRate(double sr) { sampleRate_ = sr; }
    void setLowGain(float db) { lowGain_ = std::pow(10.0f, db / 20.0f); }
    void setMidGain(float db) { midGain_ = std::pow(10.0f, db / 20.0f); }
    void setHighGain(float db) { highGain_ = std::pow(10.0f, db / 20.0f); }
    void setLowFreq(float hz) { lowFreq_ = hz; }
    void setHighFreq(float hz) { highFreq_ = hz; }

    void process(float& left, float& right) {
        // Simple 1-pole crossover approach
        float lpCoeff = 1.0f - std::exp(-2.0f * (float)M_PI * lowFreq_ / (float)sampleRate_);
        float hpCoeff = 1.0f - std::exp(-2.0f * (float)M_PI * highFreq_ / (float)sampleRate_);

        // Left
        lpStateL_ += lpCoeff * (left - lpStateL_);
        hpStateL_ += hpCoeff * (left - hpStateL_);
        float lowL = lpStateL_;
        float highL = left - hpStateL_;
        float midL = left - lowL - highL;
        left = lowL * lowGain_ + midL * midGain_ + highL * highGain_;

        // Right
        lpStateR_ += lpCoeff * (right - lpStateR_);
        hpStateR_ += hpCoeff * (right - hpStateR_);
        float lowR = lpStateR_;
        float highR = right - hpStateR_;
        float midR = right - lowR - highR;
        right = lowR * lowGain_ + midR * midGain_ + highR * highGain_;
    }

private:
    double sampleRate_ = 44100.0;
    float lowGain_ = 1.0f, midGain_ = 1.0f, highGain_ = 1.0f;
    float lowFreq_ = 200.0f, highFreq_ = 4000.0f;
    float lpStateL_ = 0.0f, lpStateR_ = 0.0f;
    float hpStateL_ = 0.0f, hpStateR_ = 0.0f;
};

// Combined effects chain
struct EffectsParams {
    // Reverb
    float reverbSize = 0.5f, reverbDamp = 0.3f, reverbMix = 0.15f;
    // Delay
    float delayTime = 0.3f, delayFeedback = 0.3f, delayMix = 0.15f;
    bool delayPingPong = false;
    // Chorus
    float chorusRate = 1.0f, chorusDepth = 3.0f, chorusMix = 0.0f;
    // Phaser
    float phaserRate = 0.5f, phaserDepth = 0.5f, phaserMix = 0.0f;
    // Distortion
    float distDrive = 0.0f, distMix = 0.0f;
    // EQ
    float eqLowGain = 0.0f, eqMidGain = 0.0f, eqHighGain = 0.0f;
    float eqLowFreq = 200.0f, eqHighFreq = 4000.0f;
};

class EffectsChain {
public:
    void setSampleRate(double sr) {
        delay_.setSampleRate(sr);
        reverb_.setSampleRate(sr);
        chorus_.setSampleRate(sr);
        phaser_.setSampleRate(sr);
        eq_.setSampleRate(sr);
    }

    void updateParams(const EffectsParams& p) {
        delay_.setTime(p.delayTime);
        delay_.setFeedback(p.delayFeedback);
        delay_.setMix(p.delayMix);
        delay_.setPingPong(p.delayPingPong);

        reverb_.setRoomSize(p.reverbSize);
        reverb_.setDamping(p.reverbDamp);
        reverb_.setMix(p.reverbMix);

        chorus_.setRate(p.chorusRate);
        chorus_.setDepth(p.chorusDepth);
        chorus_.setMix(p.chorusMix);

        phaser_.setRate(p.phaserRate);
        phaser_.setDepth(p.phaserDepth);
        phaser_.setMix(p.phaserMix);

        distortion_.setDrive(p.distDrive);
        distortion_.setMix(p.distMix);

        eq_.setLowGain(p.eqLowGain);
        eq_.setMidGain(p.eqMidGain);
        eq_.setHighGain(p.eqHighGain);
        eq_.setLowFreq(p.eqLowFreq);
        eq_.setHighFreq(p.eqHighFreq);
    }

    void process(float& left, float& right) {
        // Order: Distortion -> EQ -> Chorus -> Phaser -> Delay -> Reverb
        distortion_.process(left, right);
        eq_.process(left, right);
        chorus_.process(left, right);
        phaser_.process(left, right);
        delay_.process(left, right);
        reverb_.process(left, right);
    }

    void clear() {
        delay_.clear();
        reverb_.clear();
        chorus_.clear();
    }

private:
    StereoDelay delay_;
    Reverb reverb_;
    Chorus chorus_;
    Phaser phaser_;
    Distortion distortion_;
    ThreeBandEQ eq_;
};

} // namespace dvds
