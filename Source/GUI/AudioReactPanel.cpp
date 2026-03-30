#include "GUI/AudioReactPanel.h"
#include <cmath>

namespace dvds {

AudioReactPanel::AudioReactPanel() {
    spectrum_.fill(0.0f);
    bandEnergies_.fill(0.0f);
    layoutKnobs();
}

AudioReactPanel::~AudioReactPanel() = default;

void AudioReactPanel::setFFTData(const std::array<float, FFTAnalyzer::kBinCount>& s) { spectrum_ = s; }
void AudioReactPanel::setBandEnergies(const std::array<float, kNumBands>& e) { bandEnergies_ = e; }
void AudioReactPanel::setRMS(float rms) { rms_ = rms; }
void AudioReactPanel::setBPM(float bpm) { bpm_ = bpm; }
void AudioReactPanel::setBeatActive(bool a) { beatActive_ = a; }

void AudioReactPanel::setAudioColorConfig(const AudioColorConfig& config) {
    colorConfig_ = config;
    layoutKnobs();
}

void AudioReactPanel::paint(/* Graphics& g */) {
    drawFFTSpectrum(/* g */);
    drawBandMeters(/* g */);
    drawBeatIndicator(/* g */);

    // Draw modulation depth knobs
    for (const auto& knob : knobs_) {
        // Draw rotary knob at (knob.x, knob.y) with size and value
        // Label below
    }
}

void AudioReactPanel::resized(int w, int h) {
    width_ = w; height_ = h;
    layoutKnobs();
}

void AudioReactPanel::mouseDown(float x, float y) {
    for (int i = 0; i < static_cast<int>(knobs_.size()); ++i) {
        float dx = x - knobs_[i].x, dy = y - knobs_[i].y;
        if (dx * dx + dy * dy < knobs_[i].size * knobs_[i].size) {
            activeKnob_ = i;
            return;
        }
    }
    activeKnob_ = -1;
}

void AudioReactPanel::mouseDrag(float x, float y) {
    if (activeKnob_ < 0) return;
    // Map vertical drag to value change
    auto& knob = knobs_[activeKnob_];
    float delta = (knob.y - y) * 0.005f;
    knob.value = std::clamp(knob.value + delta, 0.0f, 1.0f);
    if (knob.target) *knob.target = knob.value;
    if (onConfigChanged) onConfigChanged(colorConfig_);
}

void AudioReactPanel::layoutKnobs() {
    knobs_.clear();
    float startY = height_ * 0.55f;
    float knobSize = 25.0f;
    float spacing = width_ / 7.0f;

    auto addKnob = [&](const std::string& label, float& target, float x) {
        knobs_.push_back({ target, &target, label, x, startY, knobSize });
    };

    addKnob("React",    colorConfig_.reactDepth,        spacing * 1);
    addKnob("Hue Rot",  colorConfig_.hueRotationSpeed,  spacing * 2);
    addKnob("Sub->L",   colorConfig_.subLightMod,       spacing * 3);
    addKnob("Low->S",   colorConfig_.lowSatMod,         spacing * 4);
    addKnob("Mid->H",   colorConfig_.midHueMod,         spacing * 5);
    addKnob("Beat",     colorConfig_.beatFlashIntensity, spacing * 6);
}

void AudioReactPanel::drawFFTSpectrum(/* Graphics& g */) const {
    // Draw spectrum bars across top portion of panel
    float barWidth = static_cast<float>(width_) / 128.0f;
    for (int i = 0; i < 128; ++i) {
        float mag = spectrum_[i * (FFTAnalyzer::kBinCount / 128)];
        float barHeight = mag * height_ * 0.3f;
        // Draw bar at (i * barWidth, height_ * 0.3f - barHeight, barWidth, barHeight)
    }
}

void AudioReactPanel::drawBandMeters(/* Graphics& g */) const {
    float meterHeight = height_ * 0.08f;
    float meterY = height_ * 0.35f;
    float meterWidth = (width_ - 30.0f) / kNumBands;
    for (int i = 0; i < kNumBands; ++i) {
        float energy = bandEnergies_[i];
        // Draw meter bar at (5 + i * meterWidth, meterY) with fill = energy
        // Label with band name
    }
}

void AudioReactPanel::drawBeatIndicator(/* Graphics& g */) const {
    // Draw BPM text and beat flash circle
    // If beatActive_, flash a bright circle
}

} // namespace dvds
