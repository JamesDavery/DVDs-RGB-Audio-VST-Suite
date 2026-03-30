#include <gtest/gtest.h>
#include "Audio/FFTAnalyzer.h"
#include "Audio/BandSplitter.h"
#include "Audio/EnvelopeFollower.h"
#include "Audio/BeatDetector.h"
#include <cmath>
#include <vector>

using namespace dvds;

TEST(FFTAnalyzerTest, InitialState) {
    FFTAnalyzer fft;
    EXPECT_FLOAT_EQ(fft.getRMS(), 0.0f);
    EXPECT_FLOAT_EQ(fft.getPeakLevel(), 0.0f);
    EXPECT_FALSE(fft.isNewDataAvailable());
}

TEST(FFTAnalyzerTest, SilenceProducesZeroSpectrum) {
    FFTAnalyzer fft;
    fft.setSampleRate(44100.0);

    std::vector<float> silence(FFTAnalyzer::kFFTSize, 0.0f);
    fft.pushSamples(silence.data(), FFTAnalyzer::kFFTSize);
    fft.processFFT();

    EXPECT_TRUE(fft.isNewDataAvailable());
    EXPECT_NEAR(fft.getRMS(), 0.0f, 0.001f);

    const auto& mag = fft.getMagnitudeSpectrum();
    for (int i = 0; i < FFTAnalyzer::kBinCount; ++i)
        EXPECT_NEAR(mag[i], 0.0f, 0.001f);
}

TEST(FFTAnalyzerTest, SineWaveProducesPeak) {
    FFTAnalyzer fft;
    double sr = 44100.0;
    fft.setSampleRate(sr);

    float freq = 1000.0f;
    std::vector<float> sine(FFTAnalyzer::kFFTSize);
    for (int i = 0; i < FFTAnalyzer::kFFTSize; ++i)
        sine[i] = std::sin(2.0f * 3.14159f * freq * i / static_cast<float>(sr));

    fft.pushSamples(sine.data(), FFTAnalyzer::kFFTSize);
    fft.processFFT();

    EXPECT_GT(fft.getRMS(), 0.0f);

    const auto& mag = fft.getMagnitudeSpectrum();
    int expectedBin = fft.frequencyToBin(freq);
    float peakMag = 0.0f;
    int peakBin = 0;
    for (int i = 0; i < FFTAnalyzer::kBinCount; ++i) {
        if (mag[i] > peakMag) { peakMag = mag[i]; peakBin = i; }
    }
    EXPECT_NEAR(peakBin, expectedBin, 3)
        << "Peak at bin " << peakBin << " expected near " << expectedBin;
}

TEST(FFTAnalyzerTest, BinFrequencyCalculation) {
    FFTAnalyzer fft;
    fft.setSampleRate(44100.0);

    EXPECT_NEAR(fft.getBinFrequency(0), 0.0f, 0.01f);
    float nyquist = fft.getBinFrequency(FFTAnalyzer::kBinCount - 1);
    EXPECT_NEAR(nyquist, 44100.0f * (FFTAnalyzer::kBinCount - 1) / FFTAnalyzer::kFFTSize, 1.0f);
}

TEST(FFTAnalyzerTest, WindowFunctions) {
    FFTAnalyzer fft;
    fft.setWindowFunction(WindowFunction::Hann);
    fft.setWindowFunction(WindowFunction::BlackmanHarris);
    fft.setWindowFunction(WindowFunction::Hamming);
    // No crash = pass
}

// -- BandSplitter Tests --

TEST(BandSplitterTest, SilenceGivesZeroBands) {
    FFTAnalyzer fft;
    fft.setSampleRate(44100.0);
    std::vector<float> silence(FFTAnalyzer::kFFTSize, 0.0f);
    fft.pushSamples(silence.data(), FFTAnalyzer::kFFTSize);
    fft.processFFT();

    BandSplitter bands;
    bands.setSampleRate(44100.0);
    bands.analyze(fft);

    for (int b = 0; b < kNumBands; ++b)
        EXPECT_NEAR(bands.getBandEnergy(b), 0.0f, 0.001f);
}

TEST(BandSplitterTest, LowFreqInSubBand) {
    FFTAnalyzer fft;
    double sr = 44100.0;
    fft.setSampleRate(sr);

    float freq = 40.0f; // sub band
    std::vector<float> sine(FFTAnalyzer::kFFTSize);
    for (int i = 0; i < FFTAnalyzer::kFFTSize; ++i)
        sine[i] = 0.5f * std::sin(2.0f * 3.14159f * freq * i / static_cast<float>(sr));
    fft.pushSamples(sine.data(), FFTAnalyzer::kFFTSize);
    fft.processFFT();

    BandSplitter bands;
    bands.setSampleRate(sr);
    bands.analyze(fft);

    EXPECT_GT(bands.getBandEnergy(FrequencyBand::Sub), 0.0f);
    EXPECT_GT(bands.getBandEnergy(FrequencyBand::Sub), bands.getBandEnergy(FrequencyBand::Presence));
}

// -- EnvelopeFollower Tests --

TEST(EnvelopeFollowerTest, FollowsInput) {
    EnvelopeFollower env;
    env.setSampleRate(44100.0);
    env.setAttack(1.0f);
    env.setRelease(50.0f);

    for (int i = 0; i < 100; ++i) env.process(0.8f);
    EXPECT_GT(env.getEnvelope(), 0.5f);

    for (int i = 0; i < 1000; ++i) env.process(0.0f);
    EXPECT_LT(env.getEnvelope(), 0.3f);
}

TEST(EnvelopeFollowerTest, PeakHold) {
    EnvelopeFollower env;
    env.setSampleRate(44100.0);
    env.setPeakHoldTime(100.0f);

    env.process(0.9f);
    EXPECT_NEAR(env.getPeak(), 0.9f, 0.01f);
    for (int i = 0; i < 100; ++i) env.process(0.0f);
    EXPECT_GT(env.getPeak(), 0.5f); // still holding
}

TEST(MultiBandEnvelopeTest, IndependentBands) {
    MultiBandEnvelope mbe;
    mbe.setSampleRate(44100.0);

    mbe.processBand(0, 0.9f);
    mbe.processBand(1, 0.1f);

    EXPECT_GT(mbe.getEnvelope(0), mbe.getEnvelope(1));
}

// -- BeatDetector Tests --

TEST(BeatDetectorTest, InitialState) {
    BeatDetector bd;
    EXPECT_FALSE(bd.isBeat());
    EXPECT_NEAR(bd.getBPM(), 120.0f, 1.0f);
}

TEST(BeatDetectorTest, NoBeatsOnSilence) {
    FFTAnalyzer fft;
    fft.setSampleRate(44100.0);
    std::vector<float> silence(FFTAnalyzer::kFFTSize, 0.0f);

    BeatDetector bd;
    bd.setSampleRate(44100.0);

    for (int i = 0; i < 10; ++i) {
        fft.pushSamples(silence.data(), FFTAnalyzer::kFFTSize);
        fft.processFFT();
        bd.process(fft);
        EXPECT_FALSE(bd.isBeat());
    }
}
