#include "PluginEditor.h"

DVDsRGBAudioEditor::DVDsRGBAudioEditor(DVDsRGBAudioProcessor& p)
    : AudioProcessorEditor(&p), processorRef_(p)
{
    setSize(1200, 800);
    setResizable(true, true);
    setResizeLimits(800, 600, 3840, 2160);

    openGLContext_.setRenderer(this);
    openGLContext_.attachTo(*this);
    openGLContext_.setContinuousRepainting(true);

    mainLayout_.initialize(getWidth(), getHeight());
    setupCallbacks();

    startTimerHz(60);
}

DVDsRGBAudioEditor::~DVDsRGBAudioEditor() {
    stopTimer();
    openGLContext_.detach();
}

void DVDsRGBAudioEditor::setupCallbacks() {
    mainLayout_.getColorWheel().onConfigChanged = [this](const dvds::HarmonyConfig& config) {
        processorRef_.getColorEngine().setConfig(config);
        auto palette = processorRef_.getColorEngine().generatePalette();
        mainLayout_.getPaletteStrip().setPalette(palette);
    };

    mainLayout_.getColorWheel().onBaseHueChanged = [this](float hue) {
        if (auto* param = processorRef_.getParameters().getParameter("baseHue"))
            param->setValueNotifyingHost(hue / 360.0f);
    };

    mainLayout_.getShaderBrowser().onShaderSelected = [this](int index, const std::string& path) {
        if (path.empty()) {
            shaderPipeline_.setActiveShader(index);
        } else {
            auto isf = isfLoader_.loadFromFile(path);
            if (isf.metadata.valid) isfRenderer_.load(isf);
        }
    };

    mainLayout_.getAudioReactPanel().onConfigChanged = [this](const dvds::AudioColorConfig& config) {
        processorRef_.getAudioColorMapper().setConfig(config);
    };
}

void DVDsRGBAudioEditor::newOpenGLContextCreated() {
    shaderPipeline_.initialize(getWidth(), getHeight());
    generativeScene_.initialize(getWidth(), getHeight());
    videoMixer_.initialize(getWidth(), getHeight());
    scene3D_.initialize(getWidth(), getHeight());
    particleSystem_.initialize(10000);

    dvds::OutputConfig outConfig;
#ifdef _WIN32
    outConfig.spoutEnabled = true;
#elif __APPLE__
    outConfig.syphonEnabled = true;
#endif
    outputManager_.initialize(outConfig);
}

void DVDsRGBAudioEditor::renderOpenGL() {
    juce::OpenGLHelpers::clear(juce::Colours::black);

    auto now = static_cast<float>(juce::Time::getMillisecondCounterHiRes() * 0.001);
    float dt = now - lastFrameTime_;
    lastFrameTime_ = now;
    elapsedTime_ += dt;

    updateVisualsFromProcessor();

    auto palette = processorRef_.getPaletteState().getShaderUniform();
    auto& fft = processorRef_.getFFTAnalyzer();
    float rms = fft.getRMS();
    float bpm = processorRef_.getBeatDetector().getBPM();
    float beat = processorRef_.getBeatDetector().isBeat() ? 1.0f : 0.0f;
    float beatPhase = processorRef_.getBeatDetector().getBeatPhase();

    shaderPipeline_.setPalette(palette);
    shaderPipeline_.setTime(elapsedTime_);
    shaderPipeline_.setResolution(static_cast<float>(getWidth()), static_cast<float>(getHeight()));
    shaderPipeline_.setRMS(rms);
    shaderPipeline_.setBPM(bpm);
    shaderPipeline_.setBeat(beat);
    shaderPipeline_.setBeatPhase(beatPhase);
    shaderPipeline_.setFFTData(fft.getMagnitudeSpectrum().data(), dvds::FFTAnalyzer::kBinCount);

    shaderPipeline_.render();

    // Send to outputs
    unsigned int outputTex = shaderPipeline_.getOutputTexture();
    outputManager_.sendFrame(outputTex, getWidth(), getHeight());

    ++frameCount_;
}

void DVDsRGBAudioEditor::openGLContextClosing() {
    outputManager_.shutdown();
}

void DVDsRGBAudioEditor::updateVisualsFromProcessor() {
    // Update GUI panels with latest audio analysis data
    auto& fft = processorRef_.getFFTAnalyzer();
    auto& bands = processorRef_.getBandSplitter();
    auto& beat = processorRef_.getBeatDetector();

    mainLayout_.getAudioReactPanel().setFFTData(fft.getMagnitudeSpectrum());
    mainLayout_.getAudioReactPanel().setBandEnergies(bands.getAllNormalizedEnergies());
    mainLayout_.getAudioReactPanel().setRMS(fft.getRMS());
    mainLayout_.getAudioReactPanel().setBPM(beat.getBPM());
    mainLayout_.getAudioReactPanel().setBeatActive(beat.isBeat());

    auto palette = processorRef_.getPaletteState().getCurrent();
    mainLayout_.getPaletteStrip().setPalette(palette);

    // Sync harmony config from processor to wheel
    mainLayout_.getColorWheel().setHarmonyConfig(processorRef_.getColorEngine().getConfig());
}

void DVDsRGBAudioEditor::timerCallback() {
    repaint();
}

void DVDsRGBAudioEditor::paint(juce::Graphics& g) {
    g.fillAll(juce::Colour(0xFF1A1A2E));
    // mainLayout_.paint() would draw all GUI panels
    // The OpenGL viewport handles the visual output in the center
}

void DVDsRGBAudioEditor::resized() {
    mainLayout_.resized(getWidth(), getHeight());
    shaderPipeline_.resize(getWidth(), getHeight());
}

void DVDsRGBAudioEditor::mouseDown(const juce::MouseEvent& e) {
    mainLayout_.getColorWheel().mouseDown(
        static_cast<float>(e.x), static_cast<float>(e.y));
}

void DVDsRGBAudioEditor::mouseDrag(const juce::MouseEvent& e) {
    mainLayout_.getColorWheel().mouseDrag(
        static_cast<float>(e.x), static_cast<float>(e.y));
}

void DVDsRGBAudioEditor::mouseUp(const juce::MouseEvent& e) {
    mainLayout_.getColorWheel().mouseUp(
        static_cast<float>(e.x), static_cast<float>(e.y));
}

void DVDsRGBAudioEditor::mouseWheelMove(const juce::MouseEvent& e,
                                          const juce::MouseWheelDetails& wheel) {
    mainLayout_.getScene3DPanel().mouseWheel(wheel.deltaY);
}
