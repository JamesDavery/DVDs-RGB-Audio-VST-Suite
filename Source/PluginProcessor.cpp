#include "PluginProcessor.h"
#include "PluginEditor.h"

DVDsRGBAudioProcessor::DVDsRGBAudioProcessor()
    : AudioProcessor(BusesProperties()
          .withInput("Input", juce::AudioChannelSet::stereo(), true)
          .withOutput("Output", juce::AudioChannelSet::stereo(), true))
    , parameters_(*this, nullptr, "DVDsRGB", createParameterLayout())
{
}

DVDsRGBAudioProcessor::~DVDsRGBAudioProcessor() = default;

juce::AudioProcessorValueTreeState::ParameterLayout DVDsRGBAudioProcessor::createParameterLayout() {
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("baseHue", 1), "Base Hue", 0.0f, 360.0f, 0.0f));

    params.push_back(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID("harmonyMode", 1), "Harmony Mode",
        juce::StringArray{
            "Analogous", "Monochromatic", "Triad", "Complementary",
            "Split-Complementary", "Double-Split", "Square", "Compound",
            "Shades", "Custom"
        }, 0));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("spread", 1), "Spread", 0.0f, 1.0f, 0.5f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("globalSat", 1), "Global Saturation", 0.0f, 2.0f, 1.0f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("globalLight", 1), "Global Lightness", 0.0f, 2.0f, 1.0f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("reactDepth", 1), "React Depth", 0.0f, 1.0f, 0.75f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("hueRotSpeed", 1), "Hue Rotation Speed", 0.0f, 1.0f, 0.1f));

    params.push_back(std::make_unique<juce::AudioParameterInt>(
        juce::ParameterID("shaderSelect", 1), "Shader", 0, 99, 0));

    params.push_back(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID("blendMode", 1), "Blend Mode",
        juce::StringArray{
            "Normal", "Add", "Multiply", "Screen", "Overlay",
            "Difference", "Exclusion", "Soft Light", "Hard Light"
        }, 0));

    params.push_back(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID("beatSync", 1), "Beat Sync", true));

    for (int i = 0; i < 6; ++i) {
        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID("fxAmount" + juce::String(i), 1),
            "FX Amount " + juce::String(i + 1), 0.0f, 1.0f, 0.5f));
    }

    return { params.begin(), params.end() };
}

void DVDsRGBAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock) {
    currentSampleRate_ = sampleRate;
    fftAnalyzer_.setSampleRate(sampleRate);
    bandSplitter_.setSampleRate(sampleRate);
    envelopes_.setSampleRate(sampleRate);
    beatDetector_.setSampleRate(sampleRate);
    sampleCounter_ = 0;
}

void DVDsRGBAudioProcessor::releaseResources() {}

bool DVDsRGBAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const {
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
        && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;
    return true;
}

void DVDsRGBAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages) {
    juce::ScopedNoDenormals noDenormals;

    int numSamples = buffer.getNumSamples();
    int numChannels = buffer.getNumChannels();

    // Mix to mono for analysis
    if (numChannels > 0) {
        const float* channelData = buffer.getReadPointer(0);
        fftAnalyzer_.pushSamples(channelData, numSamples);
    }

    sampleCounter_ += numSamples;

    // Process FFT at appropriate intervals
    if (sampleCounter_ >= dvds::FFTAnalyzer::kFFTSize) {
        sampleCounter_ -= dvds::FFTAnalyzer::kFFTSize;
        fftAnalyzer_.processFFT();
        bandSplitter_.analyze(fftAnalyzer_);
        beatDetector_.process(fftAnalyzer_);

        for (int b = 0; b < dvds::kNumBands; ++b) {
            envelopes_.processBand(b, bandSplitter_.getBandEnergy(b));
        }

        // Update color engine from parameters
        auto* hueParam = parameters_.getRawParameterValue("baseHue");
        auto* modeParam = parameters_.getRawParameterValue("harmonyMode");
        auto* spreadParam = parameters_.getRawParameterValue("spread");
        auto* satParam = parameters_.getRawParameterValue("globalSat");
        auto* lightParam = parameters_.getRawParameterValue("globalLight");
        auto* reactParam = parameters_.getRawParameterValue("reactDepth");
        auto* hueRotParam = parameters_.getRawParameterValue("hueRotSpeed");

        if (hueParam) colorEngine_.setBaseHue(*hueParam);
        if (modeParam) colorEngine_.setHarmonyMode(
            static_cast<dvds::HarmonyMode>(static_cast<int>(*modeParam)));
        if (spreadParam) colorEngine_.setSpread(*spreadParam * 180.0f);
        if (satParam) colorEngine_.setGlobalSaturationMultiplier(*satParam);
        if (lightParam) colorEngine_.setGlobalLightnessMultiplier(*lightParam);

        dvds::AudioColorConfig acConfig = audioColorMapper_.getConfig();
        if (reactParam) acConfig.reactDepth = *reactParam;
        if (hueRotParam) acConfig.hueRotationSpeed = *hueRotParam;
        audioColorMapper_.setConfig(acConfig);

        float dt = static_cast<float>(dvds::FFTAnalyzer::kFFTSize) / static_cast<float>(currentSampleRate_);
        audioColorMapper_.process(
            colorEngine_, bandSplitter_, envelopes_, beatDetector_,
            fftAnalyzer_.getRMS(), dt
        );

        paletteState_.setTarget(audioColorMapper_.getModulatedPalette());
        paletteState_.update(dt);
    }

    // Pass audio through unchanged (this is a visual plugin)
}

juce::AudioProcessorEditor* DVDsRGBAudioProcessor::createEditor() {
    return new DVDsRGBAudioEditor(*this);
}

void DVDsRGBAudioProcessor::getStateInformation(juce::MemoryBlock& destData) {
    auto state = parameters_.copyState();
    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, destData);
}

void DVDsRGBAudioProcessor::setStateInformation(const void* data, int sizeInBytes) {
    std::unique_ptr<juce::XmlElement> xml(getXmlFromBinary(data, sizeInBytes));
    if (xml && xml->hasTagName(parameters_.state.getType()))
        parameters_.replaceState(juce::ValueTree::fromXml(*xml));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() {
    return new DVDsRGBAudioProcessor();
}
