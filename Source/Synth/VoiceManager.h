#pragma once
#include "SynthVoice.h"
#include <array>
#include <cstring>

namespace dvds {

class VoiceManager {
public:
    static constexpr int kMaxVoices = 32;

    void setSampleRate(double sr) {
        sampleRate_ = sr;
        for (auto& v : voices_)
            v.setSampleRate(sr);
    }

    void setParams(const SynthParams* p) {
        params_ = p;
        for (auto& v : voices_)
            v.setParams(p);
    }

    void noteOn(int note, float velocity) {
        // Check if same note is already playing -> steal it
        for (auto& v : voices_) {
            if (v.isActive() && v.getNote() == note) {
                v.noteOn(note, velocity);
                return;
            }
        }

        // Find free voice
        for (auto& v : voices_) {
            if (!v.isActive()) {
                v.noteOn(note, velocity);
                return;
            }
        }

        // Voice stealing: take oldest (first active)
        voices_[stealIndex_].noteOn(note, velocity);
        stealIndex_ = (stealIndex_ + 1) % kMaxVoices;
    }

    void noteOff(int note) {
        for (auto& v : voices_) {
            if (v.isActive() && v.getNote() == note) {
                v.noteOff();
            }
        }
    }

    void allNotesOff() {
        for (auto& v : voices_)
            v.noteOff();
    }

    void process(float* outL, float* outR, int numSamples) {
        std::memset(outL, 0, sizeof(float) * (size_t)numSamples);
        std::memset(outR, 0, sizeof(float) * (size_t)numSamples);

        activeVoiceCount_ = 0;
        for (auto& v : voices_) {
            if (v.isActive()) {
                v.process(outL, outR, numSamples);
                if (v.isActive()) ++activeVoiceCount_;
            }
        }
    }

    int getActiveVoiceCount() const { return activeVoiceCount_; }

    void handlePitchBend(float semitones) {
        // Applied via params
    }

private:
    std::array<SynthVoice, kMaxVoices> voices_;
    const SynthParams* params_ = nullptr;
    double sampleRate_ = 44100.0;
    int stealIndex_ = 0;
    int activeVoiceCount_ = 0;
};

} // namespace dvds
