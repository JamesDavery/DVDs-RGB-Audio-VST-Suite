#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_opengl/juce_opengl.h>
#include <juce_gui_extra/juce_gui_extra.h>
#include "PluginProcessor.h"
#include "Core/ColorTheoryEngine.h"
#include "Core/PaletteState.h"

class DVDsRGBAudioEditor : public juce::AudioProcessorEditor,
                            private juce::Timer {
public:
    DVDsRGBAudioEditor(DVDsRGBAudioProcessor&);
    ~DVDsRGBAudioEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;
    void mouseDown(const juce::MouseEvent&) override;
    void mouseDrag(const juce::MouseEvent&) override;
    void mouseUp(const juce::MouseEvent&) override;

private:
    DVDsRGBAudioProcessor& processorRef_;

    // Tabs
    int activeTab_ = 0; // 0=Synth, 1=Visual, 2=Effects

    // --- Visual tab controls ---
    juce::ComboBox harmonyModeBox_;
    juce::ComboBox shaderSelectBox_;
    juce::Slider baseHueSlider_, spreadSlider_, satSlider_, lightSlider_, reactDepthSlider_;

    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> harmonyAttach_;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> hueAttach_;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> spreadAttach_;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> satAttach_;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> lightAttach_;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> reactAttach_;

    // --- Synth tab controls ---
    juce::ComboBox engineSelectBox_, osc1ShapeBox_, osc2ShapeBox_, filterTypeBox_;
    juce::Slider osc1LevelSlider_, osc2LevelSlider_, osc2DetuneSlider_, osc2SemiSlider_;
    juce::Slider subOscSlider_, noiseSlider_, pulseWidthSlider_;
    juce::Slider filterCutoffSlider_, filterResoSlider_, filterEnvAmtSlider_;
    juce::Slider ampASlider_, ampDSlider_, ampSSlider_, ampRSlider_;
    juce::Slider filtASlider_, filtDSlider_, filtSSlider_, filtRSlider_;
    juce::Slider masterGainSlider_, glideSlider_, colorSynthAmtSlider_;
    // FM
    juce::Slider fmRatio2Slider_, fmRatio3Slider_, fmIndex1Slider_, fmFeedbackSlider_;
    // WT
    juce::Slider wtPosSlider_;

    // Synth attachments
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> engineAttach_, osc1ShapeAttach_, osc2ShapeAttach_, filterTypeAttach_;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>
        osc1LvlAtt_, osc2LvlAtt_, osc2DetAtt_, osc2SemiAtt_,
        subOscAtt_, noiseAtt_, pwAtt_,
        cutoffAtt_, resoAtt_, filtEnvAtt_,
        ampAAtt_, ampDAtt_, ampSAtt_, ampRAtt_,
        filtAAtt_, filtDAtt_, filtSAtt_, filtRAtt_,
        masterAtt_, glideAtt_, colorSynthAtt_,
        fmR2Att_, fmR3Att_, fmI1Att_, fmFbAtt_,
        wtPosAtt_;

    // --- Effects tab controls ---
    juce::Slider reverbSizeSlider_, reverbMixSlider_;
    juce::Slider delayTimeSlider_, delayFbSlider_, delayMixSlider_;
    juce::Slider chorusRateSlider_, chorusMixSlider_;
    juce::Slider phaserRateSlider_, phaserMixSlider_;
    juce::Slider distDriveSlider_, distMixSlider_;
    juce::Slider eqLowSlider_, eqMidSlider_, eqHighSlider_;

    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>
        revSizeAtt_, revMixAtt_,
        delTimeAtt_, delFbAtt_, delMixAtt_,
        chRateAtt_, chMixAtt_,
        phRateAtt_, phMixAtt_,
        distDrvAtt_, distMixAtt_,
        eqLoAtt_, eqMdAtt_, eqHiAtt_;

    float elapsedTime_ = 0.0f;
    int activeShader_ = 0;

    // Cached display data
    std::array<dvds::OKHsl, 5> currentPalette_;
    std::array<dvds::RGB, 5> currentPaletteRGB_;
    std::array<float, 6> bandEnergies_{};
    float rms_ = 0.0f, bpm_ = 120.0f;
    bool beat_ = false;
    std::vector<float> spectrum_;

    void timerCallback() override;
    void updateFromProcessor();
    void showTab(int tab);

    // Drawing helpers
    void drawColorWheel(juce::Graphics& g, juce::Rectangle<float> area);
    void drawPaletteStrip(juce::Graphics& g, juce::Rectangle<float> area);
    void drawShaderPreview(juce::Graphics& g, juce::Rectangle<float> area);
    void drawFFTPanel(juce::Graphics& g, juce::Rectangle<float> area);
    void drawBandMeters(juce::Graphics& g, juce::Rectangle<float> area);
    void drawSynthTab(juce::Graphics& g, juce::Rectangle<int> area);
    void drawEffectsTab(juce::Graphics& g, juce::Rectangle<int> area);

    float wheelBaseHue_ = 0.0f;
    juce::Rectangle<float> wheelBounds_;
    bool isWheelDrag_ = false;

    // Tab button bounds
    juce::Rectangle<int> tabBounds_[3];

    float pointToHue(float x, float y, juce::Rectangle<float> area) const;
    juce::Colour toJuceColour(const dvds::RGB& c) const;

    void setupKnob(juce::Slider& s, double min, double max, double def, double step = 0.01);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DVDsRGBAudioEditor)
};
