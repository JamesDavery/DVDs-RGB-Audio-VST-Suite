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

private:
    DVDsRGBAudioProcessor& processorRef_;

    juce::ComboBox harmonyModeBox_;
    juce::ComboBox shaderSelectBox_;
    juce::Slider baseHueSlider_;
    juce::Slider spreadSlider_;
    juce::Slider satSlider_;
    juce::Slider lightSlider_;
    juce::Slider reactDepthSlider_;
    juce::Label titleLabel_;

    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> harmonyAttach_;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> hueAttach_;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> spreadAttach_;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> satAttach_;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> lightAttach_;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> reactAttach_;

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

    // Drawing helpers
    void drawColorWheel(juce::Graphics& g, juce::Rectangle<float> area);
    void drawPaletteStrip(juce::Graphics& g, juce::Rectangle<float> area);
    void drawShaderPreview(juce::Graphics& g, juce::Rectangle<float> area);
    void drawFFTPanel(juce::Graphics& g, juce::Rectangle<float> area);
    void drawBandMeters(juce::Graphics& g, juce::Rectangle<float> area);

    float wheelBaseHue_ = 0.0f;
    juce::Rectangle<float> wheelBounds_;
    bool isWheelDrag_ = false;

    float pointToHue(float x, float y, juce::Rectangle<float> area) const;
    juce::Colour toJuceColour(const dvds::RGB& c) const;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DVDsRGBAudioEditor)
};
