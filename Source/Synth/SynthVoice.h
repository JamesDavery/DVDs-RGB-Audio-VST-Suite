#pragma once
#include "Oscillator.h"
#include "ADSREnvelope.h"
#include "SVFilter.h"
#include <cmath>

namespace dvds {

struct SynthParams {
    SynthEngine engine = SynthEngine::Subtractive;

    // Subtractive
    WaveShape osc1Shape = WaveShape::Saw;
    WaveShape osc2Shape = WaveShape::Square;
    float osc1Level = 0.8f;
    float osc2Level = 0.0f;
    float osc2Detune = 7.0f;   // cents
    float osc2Semitone = 0.0f;
    float subOscLevel = 0.0f;
    float noiseLevel = 0.0f;
    float pulseWidth = 0.5f;

    // Filter
    FilterType filterType = FilterType::LowPass;
    float filterCutoff = 8000.0f;
    float filterResonance = 0.2f;
    float filterEnvAmount = 2000.0f;
    float filterKeyTracking = 0.5f;

    // ADSR
    float ampAttack = 0.01f, ampDecay = 0.1f, ampSustain = 0.8f, ampRelease = 0.3f;
    float filtAttack = 0.01f, filtDecay = 0.2f, filtSustain = 0.4f, filtRelease = 0.5f;

    // FM
    float fmRatio2 = 2.0f, fmRatio3 = 3.0f, fmRatio4 = 4.0f;
    float fmIndex1 = 1.0f, fmIndex2 = 0.5f, fmIndex3 = 0.3f;
    float fmFeedback = 0.0f;
    int fmAlgorithm = 0; // 0-3

    // Wavetable
    float wtPosition = 0.0f;

    // Color modulation targets (set by ColorSynthMapper)
    float colorCutoffMod = 0.0f;
    float colorResMod = 0.0f;
    float colorFMIndexMod = 0.0f;
    float colorWTPosMod = 0.0f;
    float colorDetuneMod = 0.0f;

    // Master
    float masterGain = 0.7f;
    float pan = 0.0f; // -1 to 1
    float pitchBend = 0.0f; // semitones
    float glideTime = 0.0f;
};

class SynthVoice {
public:
    void setSampleRate(double sr) {
        sampleRate_ = sr;
        osc1_.setSampleRate(sr);
        osc2_.setSampleRate(sr);
        subOsc_.setSampleRate(sr);
        filter_.setSampleRate(sr);
        ampEnv_.setSampleRate(sr);
        filtEnv_.setSampleRate(sr);
        wtOsc_.setSampleRate(sr);
        wtOsc_.initDefaultTables();
        for (auto& op : fmOps_) op.setSampleRate(sr);
    }

    void noteOn(int note, float velocity) {
        note_ = note;
        velocity_ = velocity;
        active_ = true;

        double freq = midiToFreq(note);
        if (params_->glideTime > 0.001f && lastFreq_ > 0.0) {
            glideTarget_ = freq;
            glideRate_ = (freq - lastFreq_) / (params_->glideTime * sampleRate_);
            currentFreq_ = lastFreq_;
        } else {
            currentFreq_ = freq;
            glideTarget_ = freq;
            glideRate_ = 0.0;
        }
        lastFreq_ = freq;

        osc1_.reset();
        osc2_.reset();
        subOsc_.reset();
        wtOsc_.reset();
        for (auto& op : fmOps_) op.reset();
        filter_.reset();
        ampEnv_.noteOn();
        filtEnv_.noteOn();
    }

    void noteOff() {
        ampEnv_.noteOff();
        filtEnv_.noteOff();
    }

    bool isActive() const { return active_; }
    int getNote() const { return note_; }
    void setParams(const SynthParams* p) { params_ = p; }

    void process(float* outL, float* outR, int numSamples) {
        if (!active_ || !params_) return;

        for (int i = 0; i < numSamples; ++i) {
            // Glide
            if (std::abs(currentFreq_ - glideTarget_) > 0.01) {
                currentFreq_ += glideRate_;
                if ((glideRate_ > 0 && currentFreq_ > glideTarget_) ||
                    (glideRate_ < 0 && currentFreq_ < glideTarget_))
                    currentFreq_ = glideTarget_;
            }

            double freq = currentFreq_ * std::pow(2.0, params_->pitchBend / 12.0);

            float sample = 0.0f;

            switch (params_->engine) {
                case SynthEngine::Subtractive:
                    sample = processSubtractive(freq);
                    break;
                case SynthEngine::FM:
                    sample = processFM(freq);
                    break;
                case SynthEngine::Wavetable:
                    sample = processWavetable(freq);
                    break;
                default:
                    break;
            }

            // Filter envelope
            float filtEnvVal = filtEnv_.process();
            float cutoff = params_->filterCutoff + params_->colorCutoffMod;
            cutoff += filtEnvVal * params_->filterEnvAmount;
            cutoff = std::clamp(cutoff, 20.0f, 20000.0f);

            float reso = std::clamp(params_->filterResonance + params_->colorResMod, 0.0f, 1.0f);

            filter_.setType(params_->filterType);
            filter_.setCutoff(cutoff);
            filter_.setResonance(reso);
            filter_.setKeyTracking(params_->filterKeyTracking);
            filter_.setNoteFreq((float)currentFreq_);
            sample = filter_.process(sample);

            // Amp envelope
            float ampEnvVal = ampEnv_.process();
            sample *= ampEnvVal * velocity_ * params_->masterGain;

            // Pan
            float panR = (params_->pan + 1.0f) * 0.5f;
            float panL = 1.0f - panR;
            outL[i] += sample * panL;
            outR[i] += sample * panR;

            if (!ampEnv_.isActive()) {
                active_ = false;
                break;
            }
        }
    }

private:
    const SynthParams* params_ = nullptr;
    double sampleRate_ = 44100.0;
    int note_ = 0;
    float velocity_ = 0.0f;
    bool active_ = false;

    double currentFreq_ = 440.0;
    double glideTarget_ = 440.0;
    double glideRate_ = 0.0;
    double lastFreq_ = 0.0;

    Oscillator osc1_, osc2_, subOsc_;
    WavetableOscillator wtOsc_;
    std::array<FMOperator, 4> fmOps_;
    SVFilter filter_;
    ADSREnvelope ampEnv_, filtEnv_;

    static double midiToFreq(int note) {
        return 440.0 * std::pow(2.0, (note - 69) / 12.0);
    }

    float processSubtractive(double freq) {
        osc1_.setFrequency(freq);
        osc1_.setShape(params_->osc1Shape);
        osc1_.setPulseWidth(params_->pulseWidth);

        float detune = params_->osc2Detune + params_->colorDetuneMod;
        double freq2 = freq * std::pow(2.0, (double)params_->osc2Semitone / 12.0);
        osc2_.setFrequency(freq2);
        osc2_.setDetune(detune);
        osc2_.setShape(params_->osc2Shape);
        osc2_.setPulseWidth(params_->pulseWidth);

        subOsc_.setFrequency(freq * 0.5);
        subOsc_.setShape(WaveShape::Sine);

        float s1 = osc1_.process() * params_->osc1Level;
        float s2 = osc2_.process() * params_->osc2Level;
        float sub = subOsc_.process() * params_->subOscLevel;
        float noise = (params_->noiseLevel > 0.001f)
            ? ((float)rand() / (float)RAND_MAX * 2.0f - 1.0f) * params_->noiseLevel
            : 0.0f;

        return s1 + s2 + sub + noise;
    }

    float processFM(double freq) {
        for (auto& op : fmOps_) op.setFrequency(freq);
        fmOps_[0].setRatio(1.0f);
        fmOps_[1].setRatio(params_->fmRatio2);
        fmOps_[2].setRatio(params_->fmRatio3);
        fmOps_[3].setRatio(params_->fmRatio4);

        float idx1 = params_->fmIndex1 + params_->colorFMIndexMod;
        fmOps_[0].setIndex(0.0f);
        fmOps_[1].setIndex(idx1);
        fmOps_[2].setIndex(params_->fmIndex2);
        fmOps_[3].setIndex(params_->fmIndex3);
        fmOps_[3].setFeedback(params_->fmFeedback);

        // Algorithm routing (4 algorithms)
        float out = 0.0f;
        switch (params_->fmAlgorithm) {
            case 0: { // serial: 4->3->2->1
                float o4 = fmOps_[3].process(0.0f);
                float o3 = fmOps_[2].process(o4);
                float o2 = fmOps_[1].process(o3);
                out = fmOps_[0].process(o2);
                break;
            }
            case 1: { // parallel carriers: (3->1) + (4->2)
                float o3 = fmOps_[2].process(0.0f);
                float o4 = fmOps_[3].process(0.0f);
                float o1 = fmOps_[0].process(o3);
                float o2 = fmOps_[1].process(o4);
                out = (o1 + o2) * 0.5f;
                break;
            }
            case 2: { // 4->3, both mod 1; 2 is carrier
                float o4 = fmOps_[3].process(0.0f);
                float o3 = fmOps_[2].process(o4);
                float o1 = fmOps_[0].process(o3);
                float o2 = fmOps_[1].process(0.0f);
                out = (o1 + o2) * 0.5f;
                break;
            }
            case 3: { // all parallel (additive)
                out = (fmOps_[0].process(0.0f) + fmOps_[1].process(0.0f) +
                       fmOps_[2].process(0.0f) + fmOps_[3].process(0.0f)) * 0.25f;
                break;
            }
            default:
                out = fmOps_[0].process(0.0f);
                break;
        }
        return out;
    }

    float processWavetable(double freq) {
        wtOsc_.setFrequency(freq);
        float pos = std::clamp(params_->wtPosition + params_->colorWTPosMod, 0.0f, 1.0f);
        wtOsc_.setPosition(pos);
        return wtOsc_.process();
    }
};

} // namespace dvds
