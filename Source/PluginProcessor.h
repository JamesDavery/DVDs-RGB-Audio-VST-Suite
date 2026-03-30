#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>
#include "Core/ColorTheoryEngine.h"
#include "Core/PaletteState.h"
#include "Audio/FFTAnalyzer.h"
#include "Audio/BandSplitter.h"
#include "Audio/EnvelopeFollower.h"
#include "Audio/BeatDetector.h"
#include "Audio/AudioColorMapper.h"
#include "Synth/SynthVoice.h"
#include "Synth/VoiceManager.h"
#include "Synth/EffectsChain.h"
#include "Synth/ColorSynthMapper.h"

class DVDsRGBAudioProcessor : public juce::AudioProcessor {
public:
    DVDsRGBAudioProcessor();
    ~DVDsRGBAudioProcessor() override;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return JucePlugin_Name; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return {}; }
    void changeProgramName(int, const juce::String&) override {}

    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    // Accessors
    dvds::ColorTheoryEngine& getColorEngine() { return colorEngine_; }
    dvds::PaletteState& getPaletteState() { return paletteState_; }
    dvds::FFTAnalyzer& getFFTAnalyzer() { return fftAnalyzer_; }
    dvds::BandSplitter& getBandSplitter() { return bandSplitter_; }
    dvds::MultiBandEnvelope& getEnvelopes() { return envelopes_; }
    dvds::BeatDetector& getBeatDetector() { return beatDetector_; }
    dvds::AudioColorMapper& getAudioColorMapper() { return audioColorMapper_; }
    dvds::VoiceManager& getVoiceManager() { return voiceManager_; }
    dvds::ColorSynthMapper& getColorSynthMapper() { return colorSynthMapper_; }
    const dvds::SynthParams& getSynthParams() const { return synthParams_; }
    const dvds::EffectsParams& getEffectsParams() const { return fxParams_; }

    juce::AudioProcessorValueTreeState& getParameters() { return parameters_; }

private:
    juce::AudioProcessorValueTreeState parameters_;

    // Color / Visual
    dvds::ColorTheoryEngine colorEngine_;
    dvds::PaletteState paletteState_;

    // Audio analysis
    dvds::FFTAnalyzer fftAnalyzer_;
    dvds::BandSplitter bandSplitter_;
    dvds::MultiBandEnvelope envelopes_;
    dvds::BeatDetector beatDetector_;
    dvds::AudioColorMapper audioColorMapper_;

    // Synth
    dvds::VoiceManager voiceManager_;
    dvds::SynthParams synthParams_;
    dvds::EffectsChain effectsChain_;
    dvds::EffectsParams fxParams_;
    dvds::ColorSynthMapper colorSynthMapper_;

    int sampleCounter_ = 0;
    double currentSampleRate_ = 44100.0;

    void updateSynthParamsFromTree();
    void updateFXParamsFromTree();
    void handleMidi(juce::MidiBuffer& midi, int numSamples);

    juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DVDsRGBAudioProcessor)
};
