#pragma once
#include "Audio/FFTAnalyzer.h"
#include "Audio/BandSplitter.h"
#include "Audio/BeatDetector.h"
#include "Audio/AudioColorMapper.h"
#include <array>
#include <functional>

namespace dvds {

class AudioReactPanel {
public:
    AudioReactPanel();
    ~AudioReactPanel();

    void setFFTData(const std::array<float, FFTAnalyzer::kBinCount>& spectrum);
    void setBandEnergies(const std::array<float, kNumBands>& energies);
    void setRMS(float rms);
    void setBPM(float bpm);
    void setBeatActive(bool active);

    void setAudioColorConfig(const AudioColorConfig& config);
    const AudioColorConfig& getAudioColorConfig() const { return colorConfig_; }

    void paint(/* Graphics& g */);
    void resized(int width, int height);
    void mouseDown(float x, float y);
    void mouseDrag(float x, float y);

    std::function<void(const AudioColorConfig&)> onConfigChanged;

private:
    std::array<float, FFTAnalyzer::kBinCount> spectrum_{};
    std::array<float, kNumBands> bandEnergies_{};
    float rms_ = 0.0f;
    float bpm_ = 120.0f;
    bool beatActive_ = false;
    AudioColorConfig colorConfig_;
    int width_ = 400, height_ = 600;

    struct KnobState {
        float value;
        float* target;
        std::string label;
        float x, y, size;
    };
    std::vector<KnobState> knobs_;
    int activeKnob_ = -1;

    void layoutKnobs();
    void drawFFTSpectrum(/* Graphics& g */) const;
    void drawBandMeters(/* Graphics& g */) const;
    void drawBeatIndicator(/* Graphics& g */) const;
};

} // namespace dvds
