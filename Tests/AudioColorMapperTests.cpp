#include <gtest/gtest.h>
#include "Audio/AudioColorMapper.h"
#include "Audio/FFTAnalyzer.h"
#include <vector>
#include <cmath>

using namespace dvds;

class AudioColorMapperFixture : public ::testing::Test {
protected:
    void SetUp() override {
        engine_.setBaseHue(120.0f);
        engine_.setHarmonyMode(HarmonyMode::Analogous);
        engine_.setBaseSaturation(0.8f);
        engine_.setBaseLightness(0.6f);

        fft_.setSampleRate(44100.0);
        bands_.setSampleRate(44100.0);
        envelopes_.setSampleRate(44100.0);
        beat_.setSampleRate(44100.0);
    }

    void feedSilence() {
        std::vector<float> silence(FFTAnalyzer::kFFTSize, 0.0f);
        fft_.pushSamples(silence.data(), FFTAnalyzer::kFFTSize);
        fft_.processFFT();
        bands_.analyze(fft_);
        for (int b = 0; b < kNumBands; ++b)
            envelopes_.processBand(b, bands_.getBandEnergy(b));
        beat_.process(fft_);
    }

    void feedSine(float freq, float amp) {
        std::vector<float> sine(FFTAnalyzer::kFFTSize);
        for (int i = 0; i < FFTAnalyzer::kFFTSize; ++i)
            sine[i] = amp * std::sin(2.0f * 3.14159f * freq * i / 44100.0f);
        fft_.pushSamples(sine.data(), FFTAnalyzer::kFFTSize);
        fft_.processFFT();
        bands_.analyze(fft_);
        for (int b = 0; b < kNumBands; ++b)
            envelopes_.processBand(b, bands_.getBandEnergy(b));
        beat_.process(fft_);
    }

    ColorTheoryEngine engine_;
    FFTAnalyzer fft_;
    BandSplitter bands_;
    MultiBandEnvelope envelopes_;
    BeatDetector beat_;
    AudioColorMapper mapper_;
};

TEST_F(AudioColorMapperFixture, SilencePreservesBasePalette) {
    feedSilence();
    mapper_.process(engine_, bands_, envelopes_, beat_, 0.0f, 1.0f / 60.0f);

    auto basePalette = engine_.generatePalette();
    auto modPalette = mapper_.getModulatedPalette();

    for (int i = 0; i < 5; ++i) {
        EXPECT_NEAR(modPalette[i].h, basePalette[i].h, 5.0f)
            << "Hue diverged on silence for swatch " << i;
    }
}

TEST_F(AudioColorMapperFixture, AudioModulatesPalette) {
    AudioColorConfig config;
    config.reactDepth = 1.0f;
    config.hueRotationSpeed = 1.0f;
    config.subLightMod = 1.0f;
    config.lowSatMod = 1.0f;
    mapper_.setConfig(config);

    feedSine(40.0f, 0.9f); // strong sub bass
    mapper_.process(engine_, bands_, envelopes_, beat_, fft_.getRMS(), 1.0f / 60.0f);

    auto basePalette = engine_.generatePalette();
    auto modPalette = mapper_.getModulatedPalette();

    bool different = false;
    for (int i = 0; i < 5; ++i) {
        if (std::abs(modPalette[i].l - basePalette[i].l) > 0.001f ||
            std::abs(modPalette[i].s - basePalette[i].s) > 0.001f) {
            different = true;
        }
    }
    EXPECT_TRUE(different) << "Palette should differ from base when audio is playing";
}

TEST_F(AudioColorMapperFixture, HueRotatesWithRMS) {
    AudioColorConfig config;
    config.reactDepth = 1.0f;
    config.hueRotationSpeed = 10.0f;
    mapper_.setConfig(config);

    feedSine(440.0f, 0.8f);
    float rms = fft_.getRMS();

    for (int i = 0; i < 10; ++i)
        mapper_.process(engine_, bands_, envelopes_, beat_, rms, 0.1f);

    float modHue = mapper_.getModulatedBaseHue();
    float baseHue = engine_.getConfig().baseHue;
    EXPECT_NE(modHue, baseHue) << "Hue should rotate with non-zero RMS";
}

TEST_F(AudioColorMapperFixture, ZeroReactDepthNoModulation) {
    AudioColorConfig config;
    config.reactDepth = 0.0f;
    mapper_.setConfig(config);

    feedSine(440.0f, 0.9f);
    mapper_.process(engine_, bands_, envelopes_, beat_, fft_.getRMS(), 0.1f);

    auto basePalette = engine_.generatePalette();
    auto modPalette = mapper_.getModulatedPalette();

    for (int i = 0; i < 5; ++i) {
        EXPECT_NEAR(modPalette[i].h, basePalette[i].h, 1.0f);
        EXPECT_NEAR(modPalette[i].s, basePalette[i].s, 0.01f);
        EXPECT_NEAR(modPalette[i].l, basePalette[i].l, 0.01f);
    }
}

TEST_F(AudioColorMapperFixture, PaletteColorsStayInRange) {
    AudioColorConfig config;
    config.reactDepth = 1.0f;
    config.subLightMod = 1.0f;
    config.lowSatMod = 1.0f;
    config.midHueMod = 1.0f;
    config.beatFlashIntensity = 1.0f;
    mapper_.setConfig(config);

    feedSine(100.0f, 1.0f);
    mapper_.process(engine_, bands_, envelopes_, beat_, fft_.getRMS(), 0.05f);

    auto modPalette = mapper_.getModulatedPalette();
    for (int i = 0; i < 5; ++i) {
        EXPECT_GE(modPalette[i].h, 0.0f);
        EXPECT_LT(modPalette[i].h, 360.0f);
        EXPECT_GE(modPalette[i].s, 0.0f);
        EXPECT_LE(modPalette[i].s, 1.0f);
        EXPECT_GE(modPalette[i].l, 0.0f);
        EXPECT_LE(modPalette[i].l, 1.0f);
    }
}

TEST_F(AudioColorMapperFixture, AllHarmonyModesWork) {
    AudioColorConfig config;
    config.reactDepth = 0.5f;
    mapper_.setConfig(config);

    feedSine(440.0f, 0.5f);

    for (int m = 0; m < static_cast<int>(HarmonyMode::Count); ++m) {
        engine_.setHarmonyMode(static_cast<HarmonyMode>(m));
        mapper_.process(engine_, bands_, envelopes_, beat_, fft_.getRMS(), 0.02f);
        auto modPalette = mapper_.getModulatedPalette();
        for (int i = 0; i < 5; ++i) {
            EXPECT_FALSE(std::isnan(modPalette[i].h)) << "NaN in mode " << m;
            EXPECT_FALSE(std::isnan(modPalette[i].s)) << "NaN in mode " << m;
            EXPECT_FALSE(std::isnan(modPalette[i].l)) << "NaN in mode " << m;
        }
    }
}
