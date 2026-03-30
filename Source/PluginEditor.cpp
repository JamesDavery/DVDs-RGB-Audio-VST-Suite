#include "PluginEditor.h"
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

juce::Colour DVDsRGBAudioEditor::toJuceColour(const dvds::RGB& c) const {
    return juce::Colour::fromFloatRGBA(
        juce::jlimit(0.0f, 1.0f, c.r),
        juce::jlimit(0.0f, 1.0f, c.g),
        juce::jlimit(0.0f, 1.0f, c.b), 1.0f);
}

void DVDsRGBAudioEditor::setupKnob(juce::Slider& s, double min, double max, double def, double step) {
    s.setSliderStyle(juce::Slider::RotaryVerticalDrag);
    s.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 52, 14);
    s.setRange(min, max, step);
    s.setValue(def, juce::dontSendNotification);
    s.setColour(juce::Slider::rotarySliderFillColourId, juce::Colour(0xFF8855EE));
    s.setColour(juce::Slider::rotarySliderOutlineColourId, juce::Colour(0xFF333355));
    s.setColour(juce::Slider::thumbColourId, juce::Colours::white);
    s.setColour(juce::Slider::textBoxTextColourId, juce::Colours::white.withAlpha(0.7f));
    s.setColour(juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    addChildComponent(s);
}

DVDsRGBAudioEditor::DVDsRGBAudioEditor(DVDsRGBAudioProcessor& p)
    : AudioProcessorEditor(&p), processorRef_(p)
{
    setSize(1200, 800);
    setResizable(true, true);
    setResizeLimits(900, 600, 3840, 2160);

    dvds::OKHsl def = { 0.0f, 0.8f, 0.65f };
    currentPalette_.fill(def);
    for (auto& c : currentPaletteRGB_) c = dvds::RGB(0.8f, 0.2f, 0.2f);
    spectrum_.resize(128, 0.0f);

    auto& params = processorRef_.getParameters();

    // ============ VISUAL TAB CONTROLS ============
    harmonyModeBox_.addItemList({
        "Analogous", "Monochromatic", "Triad", "Complementary",
        "Split-Complementary", "Double-Split", "Square", "Compound",
        "Shades", "Custom"
    }, 1);
    addChildComponent(harmonyModeBox_);
    harmonyAttach_ = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
        params, "harmonyMode", harmonyModeBox_);

    shaderSelectBox_.addItemList({
        "Plasma", "Voronoi", "Aurora", "Tunnel", "Neon Rings",
        "Fractal Noise", "Kaleidoscope", "Grid Pulse", "Liquid", "Radial Burst"
    }, 1);
    shaderSelectBox_.onChange = [this]() { activeShader_ = shaderSelectBox_.getSelectedId() - 1; };
    addChildComponent(shaderSelectBox_);

    auto setupVKnob = [this](juce::Slider& s, double min, double max, double def2) {
        s.setSliderStyle(juce::Slider::RotaryVerticalDrag);
        s.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
        s.setRange(min, max, 0.01);
        s.setValue(def2, juce::dontSendNotification);
        s.setColour(juce::Slider::rotarySliderFillColourId, juce::Colour(0xFF8855EE));
        s.setColour(juce::Slider::rotarySliderOutlineColourId, juce::Colour(0xFF333355));
        s.setColour(juce::Slider::thumbColourId, juce::Colours::white);
        addChildComponent(s);
    };

    setupVKnob(baseHueSlider_, 0, 360, 0);
    setupVKnob(spreadSlider_, 0, 1, 0.5);
    setupVKnob(satSlider_, 0, 2, 1.0);
    setupVKnob(lightSlider_, 0, 2, 1.0);
    setupVKnob(reactDepthSlider_, 0, 1, 0.75);

    hueAttach_ = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(params, "baseHue", baseHueSlider_);
    spreadAttach_ = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(params, "spread", spreadSlider_);
    satAttach_ = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(params, "globalSat", satSlider_);
    lightAttach_ = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(params, "globalLight", lightSlider_);
    reactAttach_ = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(params, "reactDepth", reactDepthSlider_);

    baseHueSlider_.onValueChange = [this]() { wheelBaseHue_ = (float)baseHueSlider_.getValue(); };

    // ============ SYNTH TAB CONTROLS ============
    engineSelectBox_.addItemList({ "Subtractive", "FM", "Wavetable" }, 1);
    addChildComponent(engineSelectBox_);
    engineAttach_ = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(params, "synthEngine", engineSelectBox_);

    osc1ShapeBox_.addItemList({ "Sine", "Saw", "Square", "Triangle", "Noise" }, 1);
    addChildComponent(osc1ShapeBox_);
    osc1ShapeAttach_ = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(params, "osc1Shape", osc1ShapeBox_);

    osc2ShapeBox_.addItemList({ "Sine", "Saw", "Square", "Triangle", "Noise" }, 1);
    addChildComponent(osc2ShapeBox_);
    osc2ShapeAttach_ = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(params, "osc2Shape", osc2ShapeBox_);

    filterTypeBox_.addItemList({ "Low Pass", "High Pass", "Band Pass", "Notch" }, 1);
    addChildComponent(filterTypeBox_);
    filterTypeAttach_ = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(params, "filterType", filterTypeBox_);

    // Osc knobs
    setupKnob(osc1LevelSlider_, 0, 1, 0.8);
    setupKnob(osc2LevelSlider_, 0, 1, 0);
    setupKnob(osc2DetuneSlider_, -100, 100, 7, 0.1);
    setupKnob(osc2SemiSlider_, -24, 24, 0, 1);
    setupKnob(subOscSlider_, 0, 1, 0);
    setupKnob(noiseSlider_, 0, 1, 0);
    setupKnob(pulseWidthSlider_, 0.05, 0.95, 0.5);

    osc1LvlAtt_ = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(params, "osc1Level", osc1LevelSlider_);
    osc2LvlAtt_ = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(params, "osc2Level", osc2LevelSlider_);
    osc2DetAtt_ = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(params, "osc2Detune", osc2DetuneSlider_);
    osc2SemiAtt_ = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(params, "osc2Semi", osc2SemiSlider_);
    subOscAtt_ = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(params, "subOscLevel", subOscSlider_);
    noiseAtt_ = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(params, "noiseLevel", noiseSlider_);
    pwAtt_ = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(params, "pulseWidth", pulseWidthSlider_);

    // Filter knobs
    setupKnob(filterCutoffSlider_, 20, 20000, 8000, 1);
    setupKnob(filterResoSlider_, 0, 1, 0.2);
    setupKnob(filterEnvAmtSlider_, -10000, 10000, 2000, 10);

    cutoffAtt_ = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(params, "filterCutoff", filterCutoffSlider_);
    resoAtt_ = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(params, "filterReso", filterResoSlider_);
    filtEnvAtt_ = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(params, "filterEnvAmt", filterEnvAmtSlider_);

    // ADSR knobs
    setupKnob(ampASlider_, 0.001, 5, 0.01);
    setupKnob(ampDSlider_, 0.001, 5, 0.1);
    setupKnob(ampSSlider_, 0, 1, 0.8);
    setupKnob(ampRSlider_, 0.001, 10, 0.3);
    setupKnob(filtASlider_, 0.001, 5, 0.01);
    setupKnob(filtDSlider_, 0.001, 5, 0.2);
    setupKnob(filtSSlider_, 0, 1, 0.4);
    setupKnob(filtRSlider_, 0.001, 10, 0.5);

    ampAAtt_ = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(params, "ampAttack", ampASlider_);
    ampDAtt_ = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(params, "ampDecay", ampDSlider_);
    ampSAtt_ = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(params, "ampSustain", ampSSlider_);
    ampRAtt_ = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(params, "ampRelease", ampRSlider_);
    filtAAtt_ = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(params, "filtAttack", filtASlider_);
    filtDAtt_ = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(params, "filtDecay", filtDSlider_);
    filtSAtt_ = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(params, "filtSustain", filtSSlider_);
    filtRAtt_ = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(params, "filtRelease", filtRSlider_);

    // FM knobs
    setupKnob(fmRatio2Slider_, 0.5, 16, 2, 0.01);
    setupKnob(fmRatio3Slider_, 0.5, 16, 3, 0.01);
    setupKnob(fmIndex1Slider_, 0, 10, 1, 0.01);
    setupKnob(fmFeedbackSlider_, 0, 1, 0);
    fmR2Att_ = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(params, "fmRatio2", fmRatio2Slider_);
    fmR3Att_ = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(params, "fmRatio3", fmRatio3Slider_);
    fmI1Att_ = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(params, "fmIndex1", fmIndex1Slider_);
    fmFbAtt_ = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(params, "fmFeedback", fmFeedbackSlider_);

    // WT knob
    setupKnob(wtPosSlider_, 0, 1, 0);
    wtPosAtt_ = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(params, "wtPosition", wtPosSlider_);

    // Master
    setupKnob(masterGainSlider_, 0, 1, 0.7);
    setupKnob(glideSlider_, 0, 1, 0);
    setupKnob(colorSynthAmtSlider_, 0, 1, 0.5);
    masterAtt_ = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(params, "masterGain", masterGainSlider_);
    glideAtt_ = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(params, "glideTime", glideSlider_);
    colorSynthAtt_ = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(params, "colorSynthAmt", colorSynthAmtSlider_);

    // ============ EFFECTS TAB CONTROLS ============
    setupKnob(reverbSizeSlider_, 0, 1, 0.5);
    setupKnob(reverbMixSlider_, 0, 1, 0.15);
    setupKnob(delayTimeSlider_, 0.01, 2, 0.3);
    setupKnob(delayFbSlider_, 0, 0.95, 0.3);
    setupKnob(delayMixSlider_, 0, 1, 0.15);
    setupKnob(chorusRateSlider_, 0.1, 10, 1);
    setupKnob(chorusMixSlider_, 0, 1, 0);
    setupKnob(phaserRateSlider_, 0.1, 5, 0.5);
    setupKnob(phaserMixSlider_, 0, 1, 0);
    setupKnob(distDriveSlider_, 0, 1, 0);
    setupKnob(distMixSlider_, 0, 1, 0);
    setupKnob(eqLowSlider_, -12, 12, 0, 0.1);
    setupKnob(eqMidSlider_, -12, 12, 0, 0.1);
    setupKnob(eqHighSlider_, -12, 12, 0, 0.1);

    revSizeAtt_ = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(params, "reverbSize", reverbSizeSlider_);
    revMixAtt_ = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(params, "reverbMix", reverbMixSlider_);
    delTimeAtt_ = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(params, "delayTime", delayTimeSlider_);
    delFbAtt_ = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(params, "delayFeedback", delayFbSlider_);
    delMixAtt_ = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(params, "delayMix", delayMixSlider_);
    chRateAtt_ = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(params, "chorusRate", chorusRateSlider_);
    chMixAtt_ = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(params, "chorusMix", chorusMixSlider_);
    phRateAtt_ = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(params, "phaserRate", phaserRateSlider_);
    phMixAtt_ = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(params, "phaserMix", phaserMixSlider_);
    distDrvAtt_ = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(params, "distDrive", distDriveSlider_);
    distMixAtt_ = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(params, "distMix", distMixSlider_);
    eqLoAtt_ = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(params, "eqLowGain", eqLowSlider_);
    eqMdAtt_ = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(params, "eqMidGain", eqMidSlider_);
    eqHiAtt_ = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(params, "eqHighGain", eqHighSlider_);

    showTab(0); // Start on Synth tab
    startTimerHz(30);
}

DVDsRGBAudioEditor::~DVDsRGBAudioEditor() { stopTimer(); }

void DVDsRGBAudioEditor::showTab(int tab) {
    activeTab_ = tab;

    // Hide all tab-specific controls
    auto hideAll = [](auto&... items) { (items.setVisible(false), ...); };

    // Visual
    hideAll(harmonyModeBox_, shaderSelectBox_, baseHueSlider_, spreadSlider_, satSlider_, lightSlider_, reactDepthSlider_);
    // Synth
    hideAll(engineSelectBox_, osc1ShapeBox_, osc2ShapeBox_, filterTypeBox_,
            osc1LevelSlider_, osc2LevelSlider_, osc2DetuneSlider_, osc2SemiSlider_,
            subOscSlider_, noiseSlider_, pulseWidthSlider_,
            filterCutoffSlider_, filterResoSlider_, filterEnvAmtSlider_,
            ampASlider_, ampDSlider_, ampSSlider_, ampRSlider_,
            filtASlider_, filtDSlider_, filtSSlider_, filtRSlider_,
            masterGainSlider_, glideSlider_, colorSynthAmtSlider_,
            fmRatio2Slider_, fmRatio3Slider_, fmIndex1Slider_, fmFeedbackSlider_,
            wtPosSlider_);
    // FX
    hideAll(reverbSizeSlider_, reverbMixSlider_, delayTimeSlider_, delayFbSlider_, delayMixSlider_,
            chorusRateSlider_, chorusMixSlider_, phaserRateSlider_, phaserMixSlider_,
            distDriveSlider_, distMixSlider_, eqLowSlider_, eqMidSlider_, eqHighSlider_);

    if (tab == 0) { // Synth
        auto showAll = [](auto&... items) { (items.setVisible(true), ...); };
        showAll(engineSelectBox_, osc1ShapeBox_, osc2ShapeBox_, filterTypeBox_,
                osc1LevelSlider_, osc2LevelSlider_, osc2DetuneSlider_, osc2SemiSlider_,
                subOscSlider_, noiseSlider_, pulseWidthSlider_,
                filterCutoffSlider_, filterResoSlider_, filterEnvAmtSlider_,
                ampASlider_, ampDSlider_, ampSSlider_, ampRSlider_,
                filtASlider_, filtDSlider_, filtSSlider_, filtRSlider_,
                masterGainSlider_, glideSlider_, colorSynthAmtSlider_,
                fmRatio2Slider_, fmRatio3Slider_, fmIndex1Slider_, fmFeedbackSlider_,
                wtPosSlider_);
    } else if (tab == 1) { // Visual
        auto showAll = [](auto&... items) { (items.setVisible(true), ...); };
        showAll(harmonyModeBox_, shaderSelectBox_, baseHueSlider_, spreadSlider_, satSlider_, lightSlider_, reactDepthSlider_);
    } else if (tab == 2) { // Effects
        auto showAll = [](auto&... items) { (items.setVisible(true), ...); };
        showAll(reverbSizeSlider_, reverbMixSlider_, delayTimeSlider_, delayFbSlider_, delayMixSlider_,
                chorusRateSlider_, chorusMixSlider_, phaserRateSlider_, phaserMixSlider_,
                distDriveSlider_, distMixSlider_, eqLowSlider_, eqMidSlider_, eqHighSlider_);
    }

    resized();
    repaint();
}

void DVDsRGBAudioEditor::timerCallback() {
    elapsedTime_ += 1.0f / 30.0f;
    updateFromProcessor();
    repaint();
}

void DVDsRGBAudioEditor::updateFromProcessor() {
    auto& engine = processorRef_.getColorEngine();
    auto& fft = processorRef_.getFFTAnalyzer();
    auto& bands = processorRef_.getBandSplitter();
    auto& beat = processorRef_.getBeatDetector();

    wheelBaseHue_ = (float)baseHueSlider_.getValue();
    engine.setBaseHue(wheelBaseHue_);
    int modeIdx = harmonyModeBox_.getSelectedId() - 1;
    if (modeIdx >= 0 && modeIdx < (int)dvds::HarmonyMode::Count)
        engine.setHarmonyMode(static_cast<dvds::HarmonyMode>(modeIdx));
    engine.setSpread((float)spreadSlider_.getValue() * 180.0f);
    engine.setGlobalSaturationMultiplier((float)satSlider_.getValue());
    engine.setGlobalLightnessMultiplier((float)lightSlider_.getValue());

    currentPalette_ = engine.generatePalette();
    currentPaletteRGB_ = engine.generatePaletteRGB();
    rms_ = fft.getRMS();
    bpm_ = beat.getBPM();
    beat_ = beat.isBeat();
    bandEnergies_ = bands.getAllNormalizedEnergies();

    const auto& mag = fft.getMagnitudeSpectrum();
    spectrum_.resize(128);
    int step = dvds::FFTAnalyzer::kBinCount / 128;
    for (int i = 0; i < 128; ++i)
        spectrum_[i] = mag[i * step];
}

// ==================== PAINT ====================

void DVDsRGBAudioEditor::paint(juce::Graphics& g) {
    g.fillAll(juce::Colour(0xFF10101C));

    int w = getWidth();
    int h = getHeight();
    int tabBarH = 36;
    int rightW = 200;
    int mainW = w - rightW;

    // Tab bar
    g.setColour(juce::Colour(0xFF181828));
    g.fillRect(0, 0, w, tabBarH);

    const char* tabNames[] = { "SYNTH", "VISUAL", "EFFECTS" };
    int tabW = 100;
    for (int i = 0; i < 3; ++i) {
        tabBounds_[i] = juce::Rectangle<int>(i * tabW + 10, 4, tabW - 4, tabBarH - 8);
        if (i == activeTab_) {
            g.setColour(juce::Colour(0xFF8855EE));
            g.fillRoundedRectangle(tabBounds_[i].toFloat(), 4.0f);
            g.setColour(juce::Colours::white);
        } else {
            g.setColour(juce::Colour(0xFF2A2A44));
            g.fillRoundedRectangle(tabBounds_[i].toFloat(), 4.0f);
            g.setColour(juce::Colours::white.withAlpha(0.5f));
        }
        g.setFont(12.0f);
        g.drawText(tabNames[i], tabBounds_[i], juce::Justification::centred);
    }

    // Title
    g.setColour(juce::Colours::white.withAlpha(0.7f));
    g.setFont(juce::Font(15.0f, juce::Font::bold));
    g.drawText("DVDs-RGB Audio VST Suite", 320, 6, 300, 24, juce::Justification::centredLeft);

    // Voice count
    int voiceCount = processorRef_.getVoiceManager().getActiveVoiceCount();
    g.setFont(10.0f);
    g.setColour(juce::Colour(0xFF8855EE));
    g.drawText("Voices: " + juce::String(voiceCount) + "/32",
               w - rightW - 120, 8, 110, 20, juce::Justification::centredRight);

    // Right panel: always show palette + FFT + meters
    int rpX = w - rightW;
    g.setColour(juce::Colour(0xFF151522));
    g.fillRect(rpX, tabBarH, rightW, h - tabBarH);
    g.setColour(juce::Colour(0xFF2A2A44));
    g.drawVerticalLine(rpX, (float)tabBarH, (float)h);

    int rpY = tabBarH + 4;
    // Mini color wheel (small)
    float miniWheelSize = (float)juce::jmin(rightW - 20, 140);
    auto miniWheelArea = juce::Rectangle<float>((float)rpX + ((float)rightW - miniWheelSize) * 0.5f,
                                                  (float)rpY, miniWheelSize, miniWheelSize);
    if (activeTab_ != 1)
        wheelBounds_ = miniWheelArea;
    drawColorWheel(g, miniWheelArea);

    float palY = miniWheelArea.getBottom() + 4;
    drawPaletteStrip(g, juce::Rectangle<float>((float)rpX + 6, palY, (float)rightW - 12, 35));

    float fftY = palY + 42;
    float fftH = (float)(h - (int)fftY) * 0.4f;
    drawFFTPanel(g, juce::Rectangle<float>((float)rpX + 6, fftY, (float)rightW - 12, fftH));

    float bandY = fftY + fftH + 6;
    float bandH = (float)(h - (int)bandY) * 0.5f;
    drawBandMeters(g, juce::Rectangle<float>((float)rpX + 6, bandY, (float)rightW - 12, bandH));

    // BPM / RMS
    float infoY = bandY + bandH + 4;
    g.setColour(juce::Colours::white.withAlpha(0.5f));
    g.setFont(10.0f);
    g.drawText("RMS: " + juce::String(rms_, 3), rpX + 6, (int)infoY, rightW / 2 - 6, 16,
               juce::Justification::centredLeft);
    g.drawText("BPM: " + juce::String((int)bpm_), rpX + rightW / 2, (int)infoY, rightW / 2 - 6, 16,
               juce::Justification::centredRight);

    // Beat dot
    g.setColour(beat_ ? juce::Colours::white : juce::Colour(0xFF333355));
    g.fillEllipse((float)(rpX + rightW / 2 - 5), infoY + 18, 10, 10);

    // Main content area
    auto mainArea = juce::Rectangle<int>(0, tabBarH, mainW, h - tabBarH);

    if (activeTab_ == 0) {
        drawSynthTab(g, mainArea);
    } else if (activeTab_ == 1) {
        // Shader preview fills main area
        auto previewArea = juce::Rectangle<float>((float)mainArea.getX() + 4, (float)mainArea.getY() + 4,
                                                   (float)mainArea.getWidth() - 8, (float)mainArea.getHeight() * 0.55f);
        drawShaderPreview(g, previewArea);

        // Visual controls below
        g.setColour(juce::Colours::white.withAlpha(0.4f));
        g.setFont(9.0f);
        int knobW = 50, knobStartX = 10;
        int knobLabelY = mainArea.getY() + (int)(mainArea.getHeight() * 0.55f) + 4;
        const char* vLabels[] = { "HUE", "SPREAD", "SAT", "LIGHT", "REACT" };
        for (int i = 0; i < 5; ++i)
            g.drawText(vLabels[i], knobStartX + i * knobW, knobLabelY, knobW, 12, juce::Justification::centred);

        int comboY = knobLabelY + 60;
        g.setFont(10.0f);
        g.drawText("Harmony:", 10, comboY, 60, 20, juce::Justification::centredLeft);
        g.drawText("Shader:", 10, comboY + 28, 60, 20, juce::Justification::centredLeft);
    } else if (activeTab_ == 2) {
        drawEffectsTab(g, mainArea);
    }
}

void DVDsRGBAudioEditor::drawSynthTab(juce::Graphics& g, juce::Rectangle<int> area) {
    int x = area.getX() + 10;
    int y = area.getY() + 8;
    int colW = (area.getWidth() - 20) / 3;

    auto drawSection = [&](int sx, int sy, int sw, int sh, const char* title) {
        g.setColour(juce::Colour(0xFF1A1A2E));
        g.fillRoundedRectangle((float)sx, (float)sy, (float)sw, (float)sh, 6.0f);
        g.setColour(juce::Colour(0xFF8855EE).withAlpha(0.5f));
        g.drawRoundedRectangle((float)sx, (float)sy, (float)sw, (float)sh, 6.0f, 1.0f);
        g.setColour(juce::Colours::white.withAlpha(0.6f));
        g.setFont(11.0f);
        g.drawText(title, sx + 8, sy + 2, sw - 16, 18, juce::Justification::centredLeft);
    };

    int secH = (area.getHeight() - 30) / 3;

    // Engine select header
    g.setColour(juce::Colours::white.withAlpha(0.5f));
    g.setFont(10.0f);
    g.drawText("Engine:", x, y, 50, 20, juce::Justification::centredLeft);

    int oscY = y + 28;
    // Oscillator section
    drawSection(x, oscY, colW - 5, secH, "OSCILLATORS");

    // Draw labels for osc knobs
    g.setColour(juce::Colours::white.withAlpha(0.35f));
    g.setFont(8.0f);
    int kx = x + 5, ky = oscY + 22;
    const char* oscLabels[] = { "OSC1", "OSC2", "DETUNE", "SEMI", "SUB", "NOISE", "PW" };
    int kw = (colW - 15) / 7;
    if (kw < 40) kw = 40;
    for (int i = 0; i < 7; ++i)
        g.drawText(oscLabels[i], kx + i * kw, ky, kw, 10, juce::Justification::centred);

    // Osc shape combos labels
    g.drawText("Shape1:", x + 5, oscY + secH - 30, 50, 14, juce::Justification::centredLeft);
    g.drawText("Shape2:", x + colW / 2, oscY + secH - 30, 50, 14, juce::Justification::centredLeft);

    // Filter section
    int filtY = oscY + secH + 5;
    drawSection(x, filtY, colW - 5, secH, "FILTER");
    g.setColour(juce::Colours::white.withAlpha(0.35f));
    g.setFont(8.0f);
    const char* filtLabels[] = { "CUTOFF", "RESO", "ENV AMT" };
    for (int i = 0; i < 3; ++i)
        g.drawText(filtLabels[i], x + 5 + i * 70, filtY + 22, 65, 10, juce::Justification::centred);
    g.drawText("Type:", x + 5, filtY + secH - 30, 40, 14, juce::Justification::centredLeft);

    // ADSR section (col 2)
    int col2x = x + colW;
    drawSection(col2x, oscY, colW - 5, secH, "AMP ENVELOPE");
    g.setColour(juce::Colours::white.withAlpha(0.35f));
    g.setFont(8.0f);
    const char* adsrLabels[] = { "ATK", "DEC", "SUS", "REL" };
    for (int i = 0; i < 4; ++i)
        g.drawText(adsrLabels[i], col2x + 5 + i * 60, oscY + 22, 55, 10, juce::Justification::centred);

    drawSection(col2x, filtY, colW - 5, secH, "FILTER ENVELOPE");
    for (int i = 0; i < 4; ++i)
        g.drawText(adsrLabels[i], col2x + 5 + i * 60, filtY + 22, 55, 10, juce::Justification::centred);

    // FM / WT / Master (col 3)
    int col3x = x + colW * 2;
    drawSection(col3x, oscY, colW - 5, secH, "FM / WAVETABLE");
    g.setColour(juce::Colours::white.withAlpha(0.35f));
    g.setFont(8.0f);
    const char* fmLabels[] = { "RATIO2", "RATIO3", "INDEX", "FB", "WT POS" };
    for (int i = 0; i < 5; ++i)
        g.drawText(fmLabels[i], col3x + 5 + i * 55, oscY + 22, 50, 10, juce::Justification::centred);

    drawSection(col3x, filtY, colW - 5, secH, "MASTER");
    const char* masterLabels[] = { "GAIN", "GLIDE", "CLR>SYN" };
    for (int i = 0; i < 3; ++i)
        g.drawText(masterLabels[i], col3x + 5 + i * 70, filtY + 22, 65, 10, juce::Justification::centred);

    // Keyboard hint
    int kbY = area.getBottom() - 30;
    g.setColour(juce::Colour(0xFF1A1A2E));
    g.fillRoundedRectangle((float)x, (float)kbY, (float)(area.getWidth() - 20), 26.0f, 4.0f);
    g.setColour(juce::Colours::white.withAlpha(0.3f));
    g.setFont(10.0f);
    g.drawText("Use MIDI keyboard or DAW to play notes | 32-voice polyphony | Engine: "
               + engineSelectBox_.getText(),
               x + 8, kbY + 3, area.getWidth() - 36, 20, juce::Justification::centredLeft);
}

void DVDsRGBAudioEditor::drawEffectsTab(juce::Graphics& g, juce::Rectangle<int> area) {
    int x = area.getX() + 10;
    int y = area.getY() + 8;
    int secW = (area.getWidth() - 30) / 2;
    int secH = (area.getHeight() - 20) / 3;

    auto drawSection = [&](int sx, int sy, int sw, int sh, const char* title) {
        g.setColour(juce::Colour(0xFF1A1A2E));
        g.fillRoundedRectangle((float)sx, (float)sy, (float)sw, (float)sh, 6.0f);
        g.setColour(juce::Colour(0xFF55AAEE).withAlpha(0.5f));
        g.drawRoundedRectangle((float)sx, (float)sy, (float)sw, (float)sh, 6.0f, 1.0f);
        g.setColour(juce::Colours::white.withAlpha(0.6f));
        g.setFont(11.0f);
        g.drawText(title, sx + 8, sy + 2, sw - 16, 18, juce::Justification::centredLeft);
    };

    g.setColour(juce::Colours::white.withAlpha(0.35f));
    g.setFont(8.0f);

    // Reverb
    drawSection(x, y, secW, secH - 5, "REVERB");
    const char* revL[] = { "SIZE", "MIX" };
    for (int i = 0; i < 2; ++i)
        g.drawText(revL[i], x + 5 + i * 70, y + 22, 65, 10, juce::Justification::centred);

    // Delay
    drawSection(x + secW + 10, y, secW, secH - 5, "DELAY");
    const char* delL[] = { "TIME", "FEEDBACK", "MIX" };
    for (int i = 0; i < 3; ++i)
        g.drawText(delL[i], x + secW + 15 + i * 70, y + 22, 65, 10, juce::Justification::centred);

    // Chorus
    int row2y = y + secH;
    drawSection(x, row2y, secW, secH - 5, "CHORUS");
    const char* chL[] = { "RATE", "MIX" };
    for (int i = 0; i < 2; ++i)
        g.drawText(chL[i], x + 5 + i * 70, row2y + 22, 65, 10, juce::Justification::centred);

    // Phaser
    drawSection(x + secW + 10, row2y, secW, secH - 5, "PHASER");
    const char* phL[] = { "RATE", "MIX" };
    for (int i = 0; i < 2; ++i)
        g.drawText(phL[i], x + secW + 15 + i * 70, row2y + 22, 65, 10, juce::Justification::centred);

    // Distortion
    int row3y = row2y + secH;
    drawSection(x, row3y, secW, secH - 5, "DISTORTION");
    const char* distL[] = { "DRIVE", "MIX" };
    for (int i = 0; i < 2; ++i)
        g.drawText(distL[i], x + 5 + i * 70, row3y + 22, 65, 10, juce::Justification::centred);

    // EQ
    drawSection(x + secW + 10, row3y, secW, secH - 5, "3-BAND EQ");
    const char* eqL[] = { "LOW", "MID", "HIGH" };
    for (int i = 0; i < 3; ++i)
        g.drawText(eqL[i], x + secW + 15 + i * 70, row3y + 22, 65, 10, juce::Justification::centred);
}

// ==================== Drawing helpers (same as before) ====================

void DVDsRGBAudioEditor::drawColorWheel(juce::Graphics& g, juce::Rectangle<float> area) {
    float cx = area.getCentreX();
    float cy = area.getCentreY();
    float outerR = area.getWidth() * 0.5f;
    float innerR = outerR * 0.62f;
    float midR = (outerR + innerR) * 0.5f;

    for (int deg = 0; deg < 360; deg += 2) {
        float a1 = (deg - 90) * (float)M_PI / 180.0f;
        float a2 = (deg + 2 - 90) * (float)M_PI / 180.0f;
        dvds::RGB rgb = dvds::clampRgb(dvds::okhslToRgb({ (float)deg, 0.85f, 0.65f }));
        g.setColour(toJuceColour(rgb));
        juce::Path seg;
        seg.addCentredArc(cx, cy, outerR, outerR, 0, a1, a2, true);
        seg.addCentredArc(cx, cy, innerR, innerR, 0, a2, a1, false);
        seg.closeSubPath();
        g.fillPath(seg);
    }

    g.setColour(juce::Colour(0xFF10101C));
    g.fillEllipse(cx - innerR + 1, cy - innerR + 1, (innerR - 1) * 2, (innerR - 1) * 2);

    float hRad = (wheelBaseHue_ - 90.0f) * (float)M_PI / 180.0f;
    g.setColour(juce::Colours::white);
    g.drawLine(cx + innerR * std::cos(hRad), cy + innerR * std::sin(hRad),
               cx + outerR * std::cos(hRad), cy + outerR * std::sin(hRad), 2.0f);

    for (int i = 0; i < 5; ++i) {
        float h = currentPalette_[i].h;
        float hr = (h - 90.0f) * (float)M_PI / 180.0f;
        float hx = cx + midR * std::cos(hr);
        float hy = cy + midR * std::sin(hr);
        float sz = (i == 0) ? 7.0f : 4.5f;
        g.setColour(toJuceColour(currentPaletteRGB_[i]));
        g.fillEllipse(hx - sz, hy - sz, sz * 2, sz * 2);
        g.setColour(juce::Colours::white.withAlpha(0.8f));
        g.drawEllipse(hx - sz, hy - sz, sz * 2, sz * 2, 1.0f);
    }

    g.setColour(juce::Colours::white.withAlpha(0.6f));
    g.setFont(12.0f);
    g.drawText(juce::String((int)wheelBaseHue_) + juce::String::charToString(0x00B0),
               (int)(cx - 20), (int)(cy - 8), 40, 16, juce::Justification::centred);
}

void DVDsRGBAudioEditor::drawPaletteStrip(juce::Graphics& g, juce::Rectangle<float> area) {
    float gap = 2.0f;
    float sw = (area.getWidth() - gap * 4.0f) / 5.0f;
    for (int i = 0; i < 5; ++i) {
        float px = area.getX() + i * (sw + gap);
        auto col = toJuceColour(currentPaletteRGB_[i]);
        g.setColour(col);
        g.fillRoundedRectangle(px, area.getY(), sw, area.getHeight() - 12, 3.0f);
        g.setColour(col.getBrightness() > 0.5f ? juce::Colours::black.withAlpha(0.6f)
                                                 : juce::Colours::white.withAlpha(0.6f));
        g.setFont(8.0f);
        juce::String hex = "#" + juce::String::toHexString((int)currentPaletteRGB_[i].toHex())
                                    .paddedLeft('0', 6).toUpperCase();
        g.drawText(hex, (int)px, (int)(area.getBottom() - 11), (int)sw, 10, juce::Justification::centred);
    }
}

void DVDsRGBAudioEditor::drawShaderPreview(juce::Graphics& g, juce::Rectangle<float> area) {
    int pw = juce::jmin((int)area.getWidth(), 500);
    int ph = juce::jmin((int)area.getHeight(), 350);
    if (pw < 10 || ph < 10) return;

    juce::Image img(juce::Image::ARGB, pw, ph, true);
    {
        juce::Image::BitmapData bmp(img, juce::Image::BitmapData::writeOnly);
        float t = elapsedTime_;

        for (int y = 0; y < ph; ++y) {
            for (int x = 0; x < pw; ++x) {
                float u = (float)x / (float)pw;
                float v = (float)y / (float)ph;
                float val = 0.0f;

                switch (activeShader_) {
                    case 0: { // Plasma
                        val = std::sin((u * 10.0f + t) * (1.0f + rms_ * 2.0f));
                        val += std::sin((v * 10.0f + t) * 1.2f);
                        val += std::sin((u * 10.0f + v * 10.0f + t * 0.7f) * 0.8f);
                        val += std::sin(std::sqrt((u-0.5f)*(u-0.5f)+(v-0.5f)*(v-0.5f)) * 20.0f - t * 2.0f);
                        val = val * 0.25f + 0.5f;
                        break;
                    }
                    case 1: { // Voronoi
                        float minD = 10.0f; int ci2 = 0;
                        for (int k = 0; k < 12; ++k) {
                            float ppx = std::fmod(std::sin((float)k * 127.1f) * 43758.5f, 1.0f) * 0.5f + 0.25f;
                            float ppy = std::fmod(std::sin((float)k * 311.7f) * 43758.5f, 1.0f) * 0.5f + 0.25f;
                            ppx += 0.15f * std::sin(t * 0.5f + (float)k);
                            ppy += 0.15f * std::cos(t * 0.3f + (float)k * 1.3f);
                            float d = std::sqrt((u-ppx)*(u-ppx) + (v-ppy)*(v-ppy));
                            if (d < minD) { minD = d; ci2 = k % 5; }
                        }
                        auto c = currentPaletteRGB_[ci2];
                        float edge = juce::jlimit(0.0f, 1.0f, minD * 8.0f);
                        bmp.setPixelColour(x, y, toJuceColour(c).withMultipliedBrightness(edge));
                        continue;
                    }
                    case 2: { // Aurora
                        float wave = std::sin(u * 3.0f + t * 0.5f + rms_ * 3.0f) * 0.15f;
                        wave += std::sin(u * 5.0f - t * 0.3f) * 0.1f;
                        float center = 0.5f + wave;
                        float spread2 = 0.15f + rms_ * 0.1f;
                        float aurora = juce::jlimit(0.0f, 1.0f, 1.0f - std::abs(v - center) / spread2);
                        aurora *= aurora;
                        float idx = std::fmod(u * 2.0f + t * 0.1f, 1.0f) * 4.0f;
                        int i0 = juce::jlimit(0, 4, (int)idx);
                        int i1 = juce::jlimit(0, 4, i0 + 1);
                        float f = idx - (int)idx;
                        auto c0 = currentPaletteRGB_[i0]; auto c1 = currentPaletteRGB_[i1];
                        bmp.setPixelColour(x, y, juce::Colour::fromFloatRGBA(
                            juce::jlimit(0.0f,1.0f,(c0.r+(c1.r-c0.r)*f)*aurora),
                            juce::jlimit(0.0f,1.0f,(c0.g+(c1.g-c0.g)*f)*aurora),
                            juce::jlimit(0.0f,1.0f,(c0.b+(c1.b-c0.b)*f)*aurora), 1.0f));
                        continue;
                    }
                    case 3: { // Tunnel
                        float ux2 = u*2-1, uy2 = v*2-1;
                        float dist = std::sqrt(ux2*ux2 + uy2*uy2);
                        float angle = std::atan2(uy2, ux2);
                        float tunnel = 1.0f / (dist + 0.01f);
                        val = std::fmod(std::abs(std::sin(tunnel + t * 0.5f) * 2.5f + angle), 5.0f) / 5.0f;
                        break;
                    }
                    case 4: { // Neon Rings
                        float ux2 = u*2-1, uy2 = v*2-1;
                        float dist = std::sqrt(ux2*ux2+uy2*uy2);
                        float glow = 0.0f; int bestRing = 0;
                        for (int r = 0; r < 5; ++r) {
                            float radius = 0.15f + r * 0.15f + std::sin(t * 0.5f + (float)r) * 0.03f;
                            float ring = std::abs(dist - radius);
                            float g2 = 0.004f / (ring + 0.002f);
                            if (g2 > glow) bestRing = r;
                            glow += g2;
                        }
                        glow = juce::jlimit(0.0f, 3.0f, glow) * 0.3f;
                        auto c = currentPaletteRGB_[bestRing];
                        bmp.setPixelColour(x, y, juce::Colour::fromFloatRGBA(
                            juce::jlimit(0.0f,1.0f,c.r*glow), juce::jlimit(0.0f,1.0f,c.g*glow),
                            juce::jlimit(0.0f,1.0f,c.b*glow), 1.0f));
                        continue;
                    }
                    default: {
                        val = std::sin(u * 8.0f + t + v * 6.0f) * 0.5f + 0.5f;
                        break;
                    }
                }

                val = juce::jlimit(0.0f, 1.0f, val);
                float idx = val * 4.0f;
                int i0 = juce::jlimit(0, 4, (int)idx);
                int i1 = juce::jlimit(0, 4, i0 + 1);
                float f = idx - (float)i0;
                auto c0 = currentPaletteRGB_[i0]; auto c1 = currentPaletteRGB_[i1];
                bmp.setPixelColour(x, y, juce::Colour::fromFloatRGBA(
                    juce::jlimit(0.0f,1.0f,c0.r+(c1.r-c0.r)*f),
                    juce::jlimit(0.0f,1.0f,c0.g+(c1.g-c0.g)*f),
                    juce::jlimit(0.0f,1.0f,c0.b+(c1.b-c0.b)*f), 1.0f));
            }
        }
    }
    g.drawImage(img, area, juce::RectanglePlacement::stretchToFit);

    g.setColour(juce::Colours::white.withAlpha(0.4f));
    g.setFont(10.0f);
    g.drawText(shaderSelectBox_.getText(), (int)(area.getX()+6), (int)(area.getY()+4), 200, 14, juce::Justification::centredLeft);
}

void DVDsRGBAudioEditor::drawFFTPanel(juce::Graphics& g, juce::Rectangle<float> area) {
    g.setColour(juce::Colour(0xFF0A0A18));
    g.fillRoundedRectangle(area, 3.0f);
    if (spectrum_.empty()) return;
    int numBars = juce::jmin((int)spectrum_.size(), (int)(area.getWidth() / 2));
    float barW = area.getWidth() / (float)numBars;
    for (int i = 0; i < numBars; ++i) {
        float mag = juce::jlimit(0.0f, 1.0f, spectrum_[i] * 6.0f);
        float barH = mag * area.getHeight() * 0.9f;
        int ci = juce::jlimit(0, 4, (int)((float)i / (float)numBars * 4.99f));
        g.setColour(toJuceColour(currentPaletteRGB_[ci]).withAlpha(0.7f));
        g.fillRect(area.getX() + i * barW, area.getBottom() - barH, barW - 1, barH);
    }
}

void DVDsRGBAudioEditor::drawBandMeters(juce::Graphics& g, juce::Rectangle<float> area) {
    g.setColour(juce::Colour(0xFF0A0A18));
    g.fillRoundedRectangle(area, 3.0f);
    const char* names[] = { "SUB", "LOW", "MID", "HI-M", "PRES", "AIR" };
    float meterW = (area.getWidth() - 8) / 6.0f;
    for (int i = 0; i < 6; ++i) {
        float mx = area.getX() + 4 + i * meterW;
        float energy = juce::jlimit(0.0f, 1.0f, bandEnergies_[i]);
        float fillH = energy * (area.getHeight() - 16);
        g.setColour(juce::Colour(0xFF1A1A33));
        g.fillRoundedRectangle(mx, area.getY() + 2, meterW - 2, area.getHeight() - 16, 2.0f);
        int ci = juce::jlimit(0, 4, i);
        auto col = toJuceColour(currentPaletteRGB_[ci]);
        if (col.getBrightness() < 0.1f) col = juce::Colour(0xFF4466AA);
        g.setColour(col);
        g.fillRoundedRectangle(mx, area.getY() + 2 + (area.getHeight() - 16 - fillH), meterW - 2, fillH, 2.0f);
        g.setColour(juce::Colours::white.withAlpha(0.4f));
        g.setFont(7.0f);
        g.drawText(names[i], (int)mx, (int)(area.getBottom() - 12), (int)(meterW - 2), 10, juce::Justification::centred);
    }
}

// ==================== Mouse handling ====================

float DVDsRGBAudioEditor::pointToHue(float x, float y, juce::Rectangle<float> area) const {
    float cx = area.getCentreX();
    float cy = area.getCentreY();
    float angle = std::atan2(y - cy, x - cx);
    float deg = angle * 180.0f / (float)M_PI + 90.0f;
    return dvds::wrapHue(deg);
}

void DVDsRGBAudioEditor::mouseDown(const juce::MouseEvent& e) {
    // Tab clicks
    for (int i = 0; i < 3; ++i) {
        if (tabBounds_[i].contains(e.x, e.y)) {
            showTab(i);
            return;
        }
    }
    // Color wheel click
    if (wheelBounds_.contains((float)e.x, (float)e.y)) {
        isWheelDrag_ = true;
        baseHueSlider_.setValue(pointToHue((float)e.x, (float)e.y, wheelBounds_), juce::sendNotificationSync);
    }
}

void DVDsRGBAudioEditor::mouseDrag(const juce::MouseEvent& e) {
    if (isWheelDrag_)
        baseHueSlider_.setValue(pointToHue((float)e.x, (float)e.y, wheelBounds_), juce::sendNotificationSync);
}

void DVDsRGBAudioEditor::mouseUp(const juce::MouseEvent&) {
    isWheelDrag_ = false;
}

// ==================== Layout ====================

void DVDsRGBAudioEditor::resized() {
    int w = getWidth();
    int h = getHeight();
    int tabBarH = 36;
    int rightW = 200;
    int mainW = w - rightW;

    if (activeTab_ == 0) { // SYNTH
        int x = 10, y = tabBarH + 8;
        int colW = (mainW - 20) / 3;
        int secH = (h - tabBarH - 30) / 3;

        // Engine select
        engineSelectBox_.setBounds(x + 55, y, 150, 22);

        int oscY = y + 28;
        int kw = juce::jmax((colW - 15) / 7, 40);
        int ky = oscY + 34;
        osc1LevelSlider_.setBounds(x + 5, ky, kw, kw);
        osc2LevelSlider_.setBounds(x + 5 + kw, ky, kw, kw);
        osc2DetuneSlider_.setBounds(x + 5 + kw * 2, ky, kw, kw);
        osc2SemiSlider_.setBounds(x + 5 + kw * 3, ky, kw, kw);
        subOscSlider_.setBounds(x + 5 + kw * 4, ky, kw, kw);
        noiseSlider_.setBounds(x + 5 + kw * 5, ky, kw, kw);
        pulseWidthSlider_.setBounds(x + 5 + kw * 6, ky, kw, kw);

        osc1ShapeBox_.setBounds(x + 55, oscY + secH - 28, colW / 2 - 60, 20);
        osc2ShapeBox_.setBounds(x + colW / 2 + 50, oscY + secH - 28, colW / 2 - 60, 20);

        // Filter
        int filtY = oscY + secH + 5;
        filterCutoffSlider_.setBounds(x + 5, filtY + 34, 65, 65);
        filterResoSlider_.setBounds(x + 75, filtY + 34, 65, 65);
        filterEnvAmtSlider_.setBounds(x + 145, filtY + 34, 65, 65);
        filterTypeBox_.setBounds(x + 45, filtY + secH - 28, colW - 55, 20);

        // Amp ADSR
        int col2x = x + colW;
        ampASlider_.setBounds(col2x + 5, oscY + 34, 55, 55);
        ampDSlider_.setBounds(col2x + 65, oscY + 34, 55, 55);
        ampSSlider_.setBounds(col2x + 125, oscY + 34, 55, 55);
        ampRSlider_.setBounds(col2x + 185, oscY + 34, 55, 55);

        // Filter ADSR
        filtASlider_.setBounds(col2x + 5, filtY + 34, 55, 55);
        filtDSlider_.setBounds(col2x + 65, filtY + 34, 55, 55);
        filtSSlider_.setBounds(col2x + 125, filtY + 34, 55, 55);
        filtRSlider_.setBounds(col2x + 185, filtY + 34, 55, 55);

        // FM / WT
        int col3x = x + colW * 2;
        fmRatio2Slider_.setBounds(col3x + 5, oscY + 34, 50, 50);
        fmRatio3Slider_.setBounds(col3x + 60, oscY + 34, 50, 50);
        fmIndex1Slider_.setBounds(col3x + 115, oscY + 34, 50, 50);
        fmFeedbackSlider_.setBounds(col3x + 170, oscY + 34, 50, 50);
        wtPosSlider_.setBounds(col3x + 225, oscY + 34, 50, 50);

        // Master
        masterGainSlider_.setBounds(col3x + 5, filtY + 34, 65, 65);
        glideSlider_.setBounds(col3x + 75, filtY + 34, 65, 65);
        colorSynthAmtSlider_.setBounds(col3x + 145, filtY + 34, 65, 65);

    } else if (activeTab_ == 1) { // VISUAL
        int y = tabBarH + (int)(h * 0.55f) + 4;
        int knobW = 50;
        baseHueSlider_.setBounds(10, y + 14, knobW, knobW);
        spreadSlider_.setBounds(10 + knobW, y + 14, knobW, knobW);
        satSlider_.setBounds(10 + knobW * 2, y + 14, knobW, knobW);
        lightSlider_.setBounds(10 + knobW * 3, y + 14, knobW, knobW);
        reactDepthSlider_.setBounds(10 + knobW * 4, y + 14, knobW, knobW);

        int comboY = y + 68;
        harmonyModeBox_.setBounds(75, comboY, mainW - 90, 22);
        shaderSelectBox_.setBounds(75, comboY + 28, mainW - 90, 22);

    } else if (activeTab_ == 2) { // EFFECTS
        int x = 10, y = tabBarH + 8;
        int secW = (mainW - 30) / 2;
        int secH = (h - tabBarH - 20) / 3;
        int kSize = 60;

        reverbSizeSlider_.setBounds(x + 5, y + 34, kSize, kSize);
        reverbMixSlider_.setBounds(x + 75, y + 34, kSize, kSize);

        int dx = x + secW + 10;
        delayTimeSlider_.setBounds(dx + 5, y + 34, kSize, kSize);
        delayFbSlider_.setBounds(dx + 75, y + 34, kSize, kSize);
        delayMixSlider_.setBounds(dx + 145, y + 34, kSize, kSize);

        int r2y = y + secH;
        chorusRateSlider_.setBounds(x + 5, r2y + 34, kSize, kSize);
        chorusMixSlider_.setBounds(x + 75, r2y + 34, kSize, kSize);

        phaserRateSlider_.setBounds(dx + 5, r2y + 34, kSize, kSize);
        phaserMixSlider_.setBounds(dx + 75, r2y + 34, kSize, kSize);

        int r3y = r2y + secH;
        distDriveSlider_.setBounds(x + 5, r3y + 34, kSize, kSize);
        distMixSlider_.setBounds(x + 75, r3y + 34, kSize, kSize);

        eqLowSlider_.setBounds(dx + 5, r3y + 34, kSize, kSize);
        eqMidSlider_.setBounds(dx + 75, r3y + 34, kSize, kSize);
        eqHighSlider_.setBounds(dx + 145, r3y + 34, kSize, kSize);
    }
}
