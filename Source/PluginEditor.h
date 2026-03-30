#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_opengl/juce_opengl.h>
#include <juce_gui_extra/juce_gui_extra.h>
#include "PluginProcessor.h"
#include "Core/ColorTheoryEngine.h"
#include "Core/PaletteState.h"

class ColorWheelWidget : public juce::Component {
public:
    ColorWheelWidget();
    void paint(juce::Graphics& g) override;
    void mouseDown(const juce::MouseEvent& e) override;
    void mouseDrag(const juce::MouseEvent& e) override;

    void setBaseHue(float hue);
    float getBaseHue() const { return baseHue_; }
    void setPalette(const std::array<dvds::OKHsl, 5>& palette);

    std::function<void(float)> onHueChanged;

private:
    float baseHue_ = 0.0f;
    std::array<dvds::OKHsl, 5> palette_;
    float pointToHue(float x, float y) const;
};

class PaletteStripWidget : public juce::Component {
public:
    void paint(juce::Graphics& g) override;
    void setPalette(const std::array<dvds::RGB, 5>& palette);
private:
    std::array<dvds::RGB, 5> palette_;
};

class FFTDisplayWidget : public juce::Component {
public:
    void paint(juce::Graphics& g) override;
    void setSpectrum(const float* data, int size);
    void setBandEnergies(const std::array<float, 6>& bands);
    void setRMS(float rms);
    void setBPM(float bpm);
    void setBeat(bool beat);
    void setPaletteColors(const std::array<dvds::RGB, 5>& colors);
private:
    std::vector<float> spectrum_;
    std::array<float, 6> bandEnergies_{};
    std::array<dvds::RGB, 5> paletteColors_;
    float rms_ = 0.0f, bpm_ = 120.0f;
    bool beat_ = false;
};

class ShaderViewport : public juce::Component, public juce::OpenGLRenderer {
public:
    ShaderViewport();
    ~ShaderViewport() override;

    void newOpenGLContextCreated() override;
    void renderOpenGL() override;
    void openGLContextClosing() override;

    void setShaderSource(const std::string& fragSrc);
    void setPalette(const std::array<float, 20>& palette);
    void setTime(float t);
    void setRMS(float rms);
    void setBPM(float bpm);
    void setBeat(float beat);
    void setBeatPhase(float phase);

    void paint(juce::Graphics& g) override;
    void resized() override;

private:
    juce::OpenGLContext glContext_;
    juce::OpenGLShaderProgram* shader_ = nullptr;
    std::string pendingFragSrc_;
    bool shaderNeedsRecompile_ = true;
    std::array<float, 20> palette_{};
    float time_ = 0.0f, rms_ = 0.0f, bpm_ = 120.0f, beat_ = 0.0f, beatPhase_ = 0.0f;
    juce::CriticalSection shaderLock_;

    std::string currentVertSrc_;
    std::string currentFragSrc_;
    bool glInitialized_ = false;
};

class DVDsRGBAudioEditor : public juce::AudioProcessorEditor,
                            private juce::Timer {
public:
    DVDsRGBAudioEditor(DVDsRGBAudioProcessor&);
    ~DVDsRGBAudioEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    DVDsRGBAudioProcessor& processorRef_;

    ColorWheelWidget colorWheel_;
    PaletteStripWidget paletteStrip_;
    FFTDisplayWidget fftDisplay_;
    ShaderViewport shaderViewport_;

    juce::ComboBox harmonyModeBox_;
    juce::Slider baseHueSlider_;
    juce::Slider spreadSlider_;
    juce::Slider satSlider_;
    juce::Slider lightSlider_;
    juce::Slider reactDepthSlider_;
    juce::ComboBox shaderSelectBox_;
    juce::Label titleLabel_;
    juce::Label harmonyLabel_;
    juce::Label shaderLabel_;
    juce::Label infoLabel_;

    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> harmonyAttach_;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> hueAttach_;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> spreadAttach_;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> satAttach_;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> lightAttach_;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> reactAttach_;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> shaderAttach_;

    float elapsedTime_ = 0.0f;
    int shaderCount_ = 0;

    void timerCallback() override;
    void updateFromProcessor();
    void populateShaderList();
    void onShaderSelected();
    void onHarmonyChanged();

    static std::vector<std::pair<std::string, std::string>> getBuiltInShaders();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DVDsRGBAudioEditor)
};
