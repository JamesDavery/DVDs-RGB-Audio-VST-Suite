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

    // === Color / Visual ===
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

    // === Synth Engine ===
    params.push_back(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID("synthEngine", 1), "Synth Engine",
        juce::StringArray{ "Subtractive", "FM", "Wavetable" }, 0));

    // Oscillator
    params.push_back(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID("osc1Shape", 1), "Osc 1 Shape",
        juce::StringArray{ "Sine", "Saw", "Square", "Triangle", "Noise" }, 1));
    params.push_back(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID("osc2Shape", 1), "Osc 2 Shape",
        juce::StringArray{ "Sine", "Saw", "Square", "Triangle", "Noise" }, 2));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("osc1Level", 1), "Osc 1 Level", 0.0f, 1.0f, 0.8f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("osc2Level", 1), "Osc 2 Level", 0.0f, 1.0f, 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("osc2Detune", 1), "Osc 2 Detune", -100.0f, 100.0f, 7.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("osc2Semi", 1), "Osc 2 Semitone", -24.0f, 24.0f, 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("subOscLevel", 1), "Sub Osc Level", 0.0f, 1.0f, 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("noiseLevel", 1), "Noise Level", 0.0f, 1.0f, 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("pulseWidth", 1), "Pulse Width", 0.05f, 0.95f, 0.5f));

    // Filter
    params.push_back(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID("filterType", 1), "Filter Type",
        juce::StringArray{ "Low Pass", "High Pass", "Band Pass", "Notch" }, 0));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("filterCutoff", 1), "Filter Cutoff",
        juce::NormalisableRange<float>(20.0f, 20000.0f, 0.1f, 0.3f), 8000.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("filterReso", 1), "Filter Resonance", 0.0f, 1.0f, 0.2f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("filterEnvAmt", 1), "Filter Env Amount", -10000.0f, 10000.0f, 2000.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("filterKeyTrack", 1), "Filter Key Tracking", 0.0f, 1.0f, 0.5f));

    // Amp ADSR
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("ampAttack", 1), "Amp Attack",
        juce::NormalisableRange<float>(0.001f, 5.0f, 0.001f, 0.3f), 0.01f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("ampDecay", 1), "Amp Decay",
        juce::NormalisableRange<float>(0.001f, 5.0f, 0.001f, 0.3f), 0.1f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("ampSustain", 1), "Amp Sustain", 0.0f, 1.0f, 0.8f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("ampRelease", 1), "Amp Release",
        juce::NormalisableRange<float>(0.001f, 10.0f, 0.001f, 0.3f), 0.3f));

    // Filter ADSR
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("filtAttack", 1), "Filter Attack",
        juce::NormalisableRange<float>(0.001f, 5.0f, 0.001f, 0.3f), 0.01f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("filtDecay", 1), "Filter Decay",
        juce::NormalisableRange<float>(0.001f, 5.0f, 0.001f, 0.3f), 0.2f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("filtSustain", 1), "Filter Sustain", 0.0f, 1.0f, 0.4f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("filtRelease", 1), "Filter Release",
        juce::NormalisableRange<float>(0.001f, 10.0f, 0.001f, 0.3f), 0.5f));

    // FM params
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("fmRatio2", 1), "FM Ratio 2", 0.5f, 16.0f, 2.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("fmRatio3", 1), "FM Ratio 3", 0.5f, 16.0f, 3.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("fmRatio4", 1), "FM Ratio 4", 0.5f, 16.0f, 4.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("fmIndex1", 1), "FM Index 1", 0.0f, 10.0f, 1.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("fmIndex2", 1), "FM Index 2", 0.0f, 10.0f, 0.5f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("fmIndex3", 1), "FM Index 3", 0.0f, 10.0f, 0.3f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("fmFeedback", 1), "FM Feedback", 0.0f, 1.0f, 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterInt>(
        juce::ParameterID("fmAlgorithm", 1), "FM Algorithm", 0, 3, 0));

    // Wavetable
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("wtPosition", 1), "WT Position", 0.0f, 1.0f, 0.0f));

    // Effects
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("reverbSize", 1), "Reverb Size", 0.0f, 1.0f, 0.5f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("reverbDamp", 1), "Reverb Damping", 0.0f, 1.0f, 0.3f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("reverbMix", 1), "Reverb Mix", 0.0f, 1.0f, 0.15f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("delayTime", 1), "Delay Time",
        juce::NormalisableRange<float>(0.01f, 2.0f, 0.001f, 0.4f), 0.3f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("delayFeedback", 1), "Delay Feedback", 0.0f, 0.95f, 0.3f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("delayMix", 1), "Delay Mix", 0.0f, 1.0f, 0.15f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("chorusRate", 1), "Chorus Rate", 0.1f, 10.0f, 1.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("chorusDepth", 1), "Chorus Depth", 0.0f, 10.0f, 3.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("chorusMix", 1), "Chorus Mix", 0.0f, 1.0f, 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("phaserRate", 1), "Phaser Rate", 0.1f, 5.0f, 0.5f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("phaserDepth", 1), "Phaser Depth", 0.0f, 1.0f, 0.5f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("phaserMix", 1), "Phaser Mix", 0.0f, 1.0f, 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("distDrive", 1), "Distortion Drive", 0.0f, 1.0f, 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("distMix", 1), "Distortion Mix", 0.0f, 1.0f, 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("eqLowGain", 1), "EQ Low Gain", -12.0f, 12.0f, 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("eqMidGain", 1), "EQ Mid Gain", -12.0f, 12.0f, 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("eqHighGain", 1), "EQ High Gain", -12.0f, 12.0f, 0.0f));

    // Master
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("masterGain", 1), "Master Gain", 0.0f, 1.0f, 0.7f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("glideTime", 1), "Glide Time", 0.0f, 1.0f, 0.0f));

    // Color-Synth mapping
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("colorSynthAmt", 1), "Color->Synth Amount", 0.0f, 1.0f, 0.5f));

    return { params.begin(), params.end() };
}

void DVDsRGBAudioProcessor::prepareToPlay(double sampleRate, int /*samplesPerBlock*/) {
    currentSampleRate_ = sampleRate;
    fftAnalyzer_.setSampleRate(sampleRate);
    bandSplitter_.setSampleRate(sampleRate);
    envelopes_.setSampleRate(sampleRate);
    beatDetector_.setSampleRate(sampleRate);
    voiceManager_.setSampleRate(sampleRate);
    effectsChain_.setSampleRate(sampleRate);
    sampleCounter_ = 0;
}

void DVDsRGBAudioProcessor::releaseResources() {}

bool DVDsRGBAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const {
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
        && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;
    return true;
}

void DVDsRGBAudioProcessor::updateSynthParamsFromTree() {
    auto getF = [this](const char* id) -> float {
        if (auto* p = parameters_.getRawParameterValue(id)) return *p;
        return 0.0f;
    };
    auto getI = [this](const char* id) -> int {
        if (auto* p = parameters_.getRawParameterValue(id)) return (int)*p;
        return 0;
    };

    synthParams_.engine = static_cast<dvds::SynthEngine>(getI("synthEngine"));
    synthParams_.osc1Shape = static_cast<dvds::WaveShape>(getI("osc1Shape"));
    synthParams_.osc2Shape = static_cast<dvds::WaveShape>(getI("osc2Shape"));
    synthParams_.osc1Level = getF("osc1Level");
    synthParams_.osc2Level = getF("osc2Level");
    synthParams_.osc2Detune = getF("osc2Detune");
    synthParams_.osc2Semitone = getF("osc2Semi");
    synthParams_.subOscLevel = getF("subOscLevel");
    synthParams_.noiseLevel = getF("noiseLevel");
    synthParams_.pulseWidth = getF("pulseWidth");

    synthParams_.filterType = static_cast<dvds::FilterType>(getI("filterType"));
    synthParams_.filterCutoff = getF("filterCutoff");
    synthParams_.filterResonance = getF("filterReso");
    synthParams_.filterEnvAmount = getF("filterEnvAmt");
    synthParams_.filterKeyTracking = getF("filterKeyTrack");

    synthParams_.ampAttack = getF("ampAttack");
    synthParams_.ampDecay = getF("ampDecay");
    synthParams_.ampSustain = getF("ampSustain");
    synthParams_.ampRelease = getF("ampRelease");
    synthParams_.filtAttack = getF("filtAttack");
    synthParams_.filtDecay = getF("filtDecay");
    synthParams_.filtSustain = getF("filtSustain");
    synthParams_.filtRelease = getF("filtRelease");

    synthParams_.fmRatio2 = getF("fmRatio2");
    synthParams_.fmRatio3 = getF("fmRatio3");
    synthParams_.fmRatio4 = getF("fmRatio4");
    synthParams_.fmIndex1 = getF("fmIndex1");
    synthParams_.fmIndex2 = getF("fmIndex2");
    synthParams_.fmIndex3 = getF("fmIndex3");
    synthParams_.fmFeedback = getF("fmFeedback");
    synthParams_.fmAlgorithm = getI("fmAlgorithm");

    synthParams_.wtPosition = getF("wtPosition");
    synthParams_.masterGain = getF("masterGain");
    synthParams_.glideTime = getF("glideTime");
}

void DVDsRGBAudioProcessor::updateFXParamsFromTree() {
    auto getF = [this](const char* id) -> float {
        if (auto* p = parameters_.getRawParameterValue(id)) return *p;
        return 0.0f;
    };

    fxParams_.reverbSize = getF("reverbSize");
    fxParams_.reverbDamp = getF("reverbDamp");
    fxParams_.reverbMix = getF("reverbMix");
    fxParams_.delayTime = getF("delayTime");
    fxParams_.delayFeedback = getF("delayFeedback");
    fxParams_.delayMix = getF("delayMix");
    fxParams_.chorusRate = getF("chorusRate");
    fxParams_.chorusDepth = getF("chorusDepth");
    fxParams_.chorusMix = getF("chorusMix");
    fxParams_.phaserRate = getF("phaserRate");
    fxParams_.phaserDepth = getF("phaserDepth");
    fxParams_.phaserMix = getF("phaserMix");
    fxParams_.distDrive = getF("distDrive");
    fxParams_.distMix = getF("distMix");
    fxParams_.eqLowGain = getF("eqLowGain");
    fxParams_.eqMidGain = getF("eqMidGain");
    fxParams_.eqHighGain = getF("eqHighGain");
}

void DVDsRGBAudioProcessor::handleMidi(juce::MidiBuffer& midi, int /*numSamples*/) {
    for (const auto metadata : midi) {
        auto msg = metadata.getMessage();
        if (msg.isNoteOn())
            voiceManager_.noteOn(msg.getNoteNumber(), msg.getFloatVelocity());
        else if (msg.isNoteOff())
            voiceManager_.noteOff(msg.getNoteNumber());
        else if (msg.isAllNotesOff() || msg.isAllSoundOff())
            voiceManager_.allNotesOff();
        else if (msg.isPitchWheel()) {
            float bend = (msg.getPitchWheelValue() - 8192.0f) / 8192.0f * 2.0f;
            synthParams_.pitchBend = bend;
        }
    }
}

void DVDsRGBAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages) {
    juce::ScopedNoDenormals noDenormals;

    int numSamples = buffer.getNumSamples();
    int numChannels = buffer.getNumChannels();

    // Update synth params from parameter tree
    updateSynthParamsFromTree();
    updateFXParamsFromTree();

    // Color -> synth modulation
    float colorSynthAmt = 0.5f;
    if (auto* p = parameters_.getRawParameterValue("colorSynthAmt"))
        colorSynthAmt = *p;

    dvds::ColorSynthConfig csc = colorSynthMapper_.getConfig();
    csc.enabled = colorSynthAmt;
    csc.cutoffFromHue = colorSynthAmt * 0.6f;
    csc.resoFromSat = colorSynthAmt * 0.4f;
    csc.fmIndexFromLight = colorSynthAmt * 0.5f;
    csc.wtPosFromHue = colorSynthAmt * 0.5f;
    csc.detuneFromSpread = colorSynthAmt * 0.3f;
    colorSynthMapper_.setConfig(csc);

    auto palette = colorEngine_.generatePalette();
    auto bandE = bandSplitter_.getAllNormalizedEnergies();
    colorSynthMapper_.update(palette, bandE, fftAnalyzer_.getRMS());
    colorSynthMapper_.applySynthParams(synthParams_);

    // Apply color modulation to effects too
    dvds::EffectsParams modulatedFX = fxParams_;
    colorSynthMapper_.applyEffectsParams(modulatedFX);
    effectsChain_.updateParams(modulatedFX);

    voiceManager_.setParams(&synthParams_);

    // Handle MIDI
    handleMidi(midiMessages, numSamples);

    // Clear output and generate synth audio
    buffer.clear();

    if (numChannels >= 2) {
        float* outL = buffer.getWritePointer(0);
        float* outR = buffer.getWritePointer(1);
        voiceManager_.process(outL, outR, numSamples);

        // Apply effects per-sample
        for (int i = 0; i < numSamples; ++i)
            effectsChain_.process(outL[i], outR[i]);
    } else if (numChannels == 1) {
        std::vector<float> tempR((size_t)numSamples, 0.0f);
        float* outL = buffer.getWritePointer(0);
        voiceManager_.process(outL, tempR.data(), numSamples);
        for (int i = 0; i < numSamples; ++i) {
            effectsChain_.process(outL[i], tempR[i]);
            outL[i] = (outL[i] + tempR[i]) * 0.5f;
        }
    }

    // Feed output to audio analysis
    if (numChannels > 0) {
        const float* analysisData = buffer.getReadPointer(0);
        fftAnalyzer_.pushSamples(analysisData, numSamples);
    }

    sampleCounter_ += numSamples;
    if (sampleCounter_ >= dvds::FFTAnalyzer::kFFTSize) {
        sampleCounter_ -= dvds::FFTAnalyzer::kFFTSize;
        fftAnalyzer_.processFFT();
        bandSplitter_.analyze(fftAnalyzer_);
        beatDetector_.process(fftAnalyzer_);

        for (int b = 0; b < dvds::kNumBands; ++b)
            envelopes_.processBand(b, bandSplitter_.getBandEnergy(b));

        // Update color engine
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

        float dt = (float)dvds::FFTAnalyzer::kFFTSize / (float)currentSampleRate_;
        audioColorMapper_.process(
            colorEngine_, bandSplitter_, envelopes_, beatDetector_,
            fftAnalyzer_.getRMS(), dt);

        paletteState_.setTarget(audioColorMapper_.getModulatedPalette());
        paletteState_.update(dt);
    }
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
