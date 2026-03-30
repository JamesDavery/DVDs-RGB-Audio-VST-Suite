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

    dvds::ColorTheoryEngine& getColorEngine() { return colorEngine_; }
    dvds::PaletteState& getPaletteState() { return paletteState_; }
    dvds::FFTAnalyzer& getFFTAnalyzer() { return fftAnalyzer_; }
    dvds::BandSplitter& getBandSplitter() { return bandSplitter_; }
    dvds::MultiBandEnvelope& getEnvelopes() { return envelopes_; }
    dvds::BeatDetector& getBeatDetector() { return beatDetector_; }
    dvds::AudioColorMapper& getAudioColorMapper() { return audioColorMapper_; }

    juce::AudioProcessorValueTreeState& getParameters() { return parameters_; }

private:
    juce::AudioProcessorValueTreeState parameters_;

    dvds::ColorTheoryEngine colorEngine_;
    dvds::PaletteState paletteState_;
    dvds::FFTAnalyzer fftAnalyzer_;
    dvds::BandSplitter bandSplitter_;
    dvds::MultiBandEnvelope envelopes_;
    dvds::BeatDetector beatDetector_;
    dvds::AudioColorMapper audioColorMapper_;

    int sampleCounter_ = 0;
    double currentSampleRate_ = 44100.0;

    juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DVDsRGBAudioProcessor)
};
