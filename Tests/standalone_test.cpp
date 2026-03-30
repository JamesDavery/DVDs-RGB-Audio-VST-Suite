#include <iostream>
#include <cassert>
#include <cmath>
#include <string>
#include <vector>

#include "Core/OKLab.h"
#include "Core/ColorTheoryEngine.h"
#include "Core/PaletteState.h"
#include "Audio/FFTAnalyzer.h"
#include "Audio/BandSplitter.h"
#include "Audio/EnvelopeFollower.h"
#include "Audio/BeatDetector.h"
#include "Audio/AudioColorMapper.h"

using namespace dvds;

static int passed = 0, failed = 0;

#define TEST_ASSERT(cond, msg) do { \
    if (!(cond)) { std::cout << "  FAIL: " << msg << "\n"; ++failed; } \
    else { ++passed; } \
} while(0)

#define NEAR(a, b, tol) (std::abs((a) - (b)) < (tol))

void printRGB(const char* label, const RGB& c) {
    printf("  %s: #%06X  rgb(%d, %d, %d)\n", label, c.toHex(),
        int(c.r*255), int(c.g*255), int(c.b*255));
}

void testOKLab() {
    std::cout << "\n=== OKLab Color Math ===\n";

    RGB red(1, 0, 0);
    OKLab lab = rgbToOklab(red);
    RGB back = oklabToRgb(lab);
    TEST_ASSERT(NEAR(back.r, 1.0f, 0.02f) && NEAR(back.g, 0.0f, 0.02f), "Red round-trip");

    TEST_ASSERT(NEAR(rgbToOklab(RGB(0,0,0)).L, 0.0f, 0.01f), "Black L=0");
    TEST_ASSERT(NEAR(rgbToOklab(RGB(1,1,1)).L, 1.0f, 0.01f), "White L=1");

    TEST_ASSERT(wrapHue(-30.0f) == 330.0f, "wrapHue(-30) = 330");
    TEST_ASSERT(wrapHue(450.0f) == 90.0f, "wrapHue(450) = 90");
    TEST_ASSERT(NEAR(hueDistance(350.0f, 10.0f), 20.0f, 0.01f), "hueDistance across 0");
    TEST_ASSERT(NEAR(lerpHue(350.0f, 10.0f, 0.5f), 0.0f, 0.5f), "lerpHue across 0");

    RGB hex = RGB::fromHex(0xFF8800);
    TEST_ASSERT(hex.toHex() == 0xFF8800, "Hex round-trip");

    auto uniforms = paletteToShaderUniform({
        OKHsl{0,0.8f,0.6f}, OKHsl{72,0.8f,0.6f}, OKHsl{144,0.8f,0.6f},
        OKHsl{216,0.8f,0.6f}, OKHsl{288,0.8f,0.6f}
    });
    bool allValid = true;
    for (int i = 0; i < 20; ++i)
        if (uniforms[i] < 0.0f || uniforms[i] > 1.01f) allValid = false;
    TEST_ASSERT(allValid, "Shader uniforms in [0,1]");
}

void testColorTheory() {
    std::cout << "\n=== Color Theory Engine (10 Modes) ===\n";

    ColorTheoryEngine engine;
    const char* modeNames[] = {
        "Analogous", "Monochromatic", "Triad", "Complementary",
        "Split-Comp", "Double-Split", "Square", "Compound", "Shades", "Custom"
    };

    for (int m = 0; m < static_cast<int>(HarmonyMode::Count); ++m) {
        engine.setHarmonyMode(static_cast<HarmonyMode>(m));
        engine.setBaseHue(200.0f);
        engine.setBaseSaturation(0.8f);
        engine.setBaseLightness(0.6f);
        auto palette = engine.generatePalette();
        auto rgb = engine.generatePaletteRGB();

        TEST_ASSERT(palette.size() == 5, std::string(modeNames[m]) + " generates 5 colors");

        bool allValid = true;
        for (int i = 0; i < 5; ++i) {
            if (palette[i].h < 0 || palette[i].h >= 360 ||
                palette[i].s < 0 || palette[i].s > 1.01f ||
                palette[i].l < 0 || palette[i].l > 1.01f)
                allValid = false;
        }
        TEST_ASSERT(allValid, std::string(modeNames[m]) + " colors in valid range");

        std::cout << "  " << modeNames[m] << " (base hue 200): ";
        for (int i = 0; i < 5; ++i) printf("#%06X ", rgb[i].toHex());
        std::cout << "\n";
    }

    // Specific mode tests
    engine.setHarmonyMode(HarmonyMode::Monochromatic);
    engine.setBaseHue(120.0f);
    auto mono = engine.generatePalette();
    bool sameHue = true;
    for (auto& c : mono) if (!NEAR(c.h, 120.0f, 0.1f)) sameHue = false;
    TEST_ASSERT(sameHue, "Monochromatic: all same hue");

    engine.setHarmonyMode(HarmonyMode::Shades);
    auto shades = engine.generatePalette();
    bool descending = true;
    for (int i = 1; i < 5; ++i) if (shades[i].l >= shades[i-1].l) descending = false;
    TEST_ASSERT(descending, "Shades: lightness descends");

    engine.setHarmonyMode(HarmonyMode::Complementary);
    engine.setBaseHue(60.0f);
    auto comp = engine.generatePalette();
    bool hasComp = false;
    for (auto& c : comp) if (std::abs(hueDistance(60.0f, c.h)) > 170.0f) hasComp = true;
    TEST_ASSERT(hasComp, "Complementary: has opposite hue");

    engine.setHarmonyMode(HarmonyMode::Triad);
    engine.setBaseHue(0.0f);
    auto triad = engine.generatePalette();
    bool has120 = false, has240 = false;
    for (auto& c : triad) {
        if (NEAR(wrapHue(c.h), 120.0f, 5.0f)) has120 = true;
        if (NEAR(wrapHue(c.h), 240.0f, 5.0f)) has240 = true;
    }
    TEST_ASSERT(has120 && has240, "Triad: has 120 and 240 degree hues");

    // String round-trip
    for (int m = 0; m < static_cast<int>(HarmonyMode::Count); ++m) {
        auto mode = static_cast<HarmonyMode>(m);
        TEST_ASSERT(stringToHarmonyMode(harmonyModeToString(mode)) == mode,
            "String round-trip: " + harmonyModeToString(mode));
    }
}

void testFFT() {
    std::cout << "\n=== FFT Analyzer ===\n";

    FFTAnalyzer fft;
    fft.setSampleRate(44100.0);

    // Silence
    std::vector<float> silence(FFTAnalyzer::kFFTSize, 0.0f);
    fft.pushSamples(silence.data(), FFTAnalyzer::kFFTSize);
    fft.processFFT();
    TEST_ASSERT(NEAR(fft.getRMS(), 0.0f, 0.001f), "Silence RMS ~0");
    TEST_ASSERT(fft.isNewDataAvailable(), "New data flag set after process");

    // 1kHz sine
    float freq = 1000.0f;
    std::vector<float> sine(FFTAnalyzer::kFFTSize);
    for (int i = 0; i < FFTAnalyzer::kFFTSize; ++i)
        sine[i] = std::sin(2.0f * 3.14159f * freq * i / 44100.0f);
    fft.pushSamples(sine.data(), FFTAnalyzer::kFFTSize);
    fft.processFFT();

    TEST_ASSERT(fft.getRMS() > 0.3f, "Sine RMS > 0.3");

    const auto& mag = fft.getMagnitudeSpectrum();
    int expectedBin = fft.frequencyToBin(freq);
    float peakMag = 0; int peakBin = 0;
    for (int i = 0; i < FFTAnalyzer::kBinCount; ++i)
        if (mag[i] > peakMag) { peakMag = mag[i]; peakBin = i; }
    TEST_ASSERT(std::abs(peakBin - expectedBin) <= 3,
        "1kHz peak at bin " + std::to_string(peakBin) + " (expected ~" + std::to_string(expectedBin) + ")");

    std::cout << "  1kHz sine: peak at bin " << peakBin << " = "
              << fft.getBinFrequency(peakBin) << " Hz, RMS=" << fft.getRMS() << "\n";
}

void testBandSplitter() {
    std::cout << "\n=== Band Splitter ===\n";

    FFTAnalyzer fft;
    fft.setSampleRate(44100.0);
    BandSplitter bands;
    bands.setSampleRate(44100.0);

    // 40 Hz sine (sub band)
    std::vector<float> sine(FFTAnalyzer::kFFTSize);
    for (int i = 0; i < FFTAnalyzer::kFFTSize; ++i)
        sine[i] = 0.8f * std::sin(2.0f * 3.14159f * 40.0f * i / 44100.0f);
    fft.pushSamples(sine.data(), FFTAnalyzer::kFFTSize);
    fft.processFFT();
    bands.analyze(fft);

    TEST_ASSERT(bands.getBandEnergy(FrequencyBand::Sub) > 0.0f, "Sub band has energy at 40Hz");
    TEST_ASSERT(bands.getBandEnergy(FrequencyBand::Sub) > bands.getBandEnergy(FrequencyBand::Presence),
        "Sub > Presence at 40Hz");

    std::cout << "  40Hz sine band energies: ";
    for (int b = 0; b < kNumBands; ++b)
        printf("%s=%.4f ", bandToString(static_cast<FrequencyBand>(b)).c_str(), bands.getBandEnergy(b));
    std::cout << "\n";
}

void testEnvelopeFollower() {
    std::cout << "\n=== Envelope Follower ===\n";

    EnvelopeFollower env;
    env.setSampleRate(44100.0);
    env.setAttack(1.0f);
    env.setRelease(50.0f);

    for (int i = 0; i < 200; ++i) env.process(0.8f);
    TEST_ASSERT(env.getEnvelope() > 0.5f, "Follows rising input");
    float peak = env.getEnvelope();

    for (int i = 0; i < 2000; ++i) env.process(0.0f);
    TEST_ASSERT(env.getEnvelope() < peak * 0.5f, "Decays on silence");
}

void testAudioColorMapper() {
    std::cout << "\n=== Audio-Color Adaptive Mapper ===\n";

    ColorTheoryEngine engine;
    engine.setBaseHue(120.0f);
    engine.setHarmonyMode(HarmonyMode::Analogous);
    engine.setBaseSaturation(0.8f);
    engine.setBaseLightness(0.6f);

    FFTAnalyzer fft;
    fft.setSampleRate(44100.0);
    BandSplitter bands;
    bands.setSampleRate(44100.0);
    MultiBandEnvelope envelopes;
    envelopes.setSampleRate(44100.0);
    BeatDetector beat;
    beat.setSampleRate(44100.0);
    AudioColorMapper mapper;

    // Silence: palette should stay close to base
    std::vector<float> silence(FFTAnalyzer::kFFTSize, 0.0f);
    fft.pushSamples(silence.data(), FFTAnalyzer::kFFTSize);
    fft.processFFT();
    bands.analyze(fft);
    beat.process(fft);
    mapper.process(engine, bands, envelopes, beat, 0.0f, 1.0f/60.0f);

    auto base = engine.generatePalette();
    auto mod = mapper.getModulatedPalette();
    bool closeToBase = true;
    for (int i = 0; i < 5; ++i)
        if (!NEAR(mod[i].h, base[i].h, 10.0f)) closeToBase = false;
    TEST_ASSERT(closeToBase, "Silence: palette close to base");

    // Loud sine: palette should differ
    AudioColorConfig config;
    config.reactDepth = 1.0f;
    config.hueRotationSpeed = 5.0f;
    config.subLightMod = 1.0f;
    config.lowSatMod = 1.0f;
    mapper.setConfig(config);

    std::vector<float> loud(FFTAnalyzer::kFFTSize);
    for (int i = 0; i < FFTAnalyzer::kFFTSize; ++i)
        loud[i] = 0.9f * std::sin(2.0f * 3.14159f * 80.0f * i / 44100.0f);
    fft.pushSamples(loud.data(), FFTAnalyzer::kFFTSize);
    fft.processFFT();
    bands.analyze(fft);
    for (int b = 0; b < kNumBands; ++b)
        envelopes.processBand(b, bands.getBandEnergy(b));
    beat.process(fft);

    for (int i = 0; i < 10; ++i)
        mapper.process(engine, bands, envelopes, beat, fft.getRMS(), 0.1f);

    mod = mapper.getModulatedPalette();
    bool changed = false;
    for (int i = 0; i < 5; ++i)
        if (!NEAR(mod[i].l, base[i].l, 0.001f) || !NEAR(mod[i].s, base[i].s, 0.001f))
            changed = true;
    TEST_ASSERT(changed, "Loud audio: palette modulated");

    // Range check
    bool inRange = true;
    for (int i = 0; i < 5; ++i) {
        if (mod[i].h < 0 || mod[i].h >= 360 || mod[i].s < 0 || mod[i].s > 1 ||
            mod[i].l < 0 || mod[i].l > 1) inRange = false;
    }
    TEST_ASSERT(inRange, "Modulated palette stays in valid range");

    // All harmony modes work with mapper
    for (int m = 0; m < static_cast<int>(HarmonyMode::Count); ++m) {
        engine.setHarmonyMode(static_cast<HarmonyMode>(m));
        mapper.process(engine, bands, envelopes, beat, fft.getRMS(), 0.02f);
        auto p = mapper.getModulatedPalette();
        bool valid = true;
        for (int i = 0; i < 5; ++i)
            if (std::isnan(p[i].h) || std::isnan(p[i].s) || std::isnan(p[i].l)) valid = false;
        TEST_ASSERT(valid, "No NaN in mode " + harmonyModeToString(static_cast<HarmonyMode>(m)));
    }

    // Print demo output
    engine.setHarmonyMode(HarmonyMode::Complementary);
    mapper.process(engine, bands, envelopes, beat, fft.getRMS(), 0.05f);
    mod = mapper.getModulatedPalette();
    std::cout << "  Complementary + 80Hz bass modulation:\n";
    for (int i = 0; i < 5; ++i) {
        RGB rgb = clampRgb(okhslToRgb(mod[i]));
        printf("    Swatch %d: #%06X  (H=%.1f S=%.2f L=%.2f)\n",
            i, rgb.toHex(), mod[i].h, mod[i].s, mod[i].l);
    }
}

void testPaletteState() {
    std::cout << "\n=== Palette State ===\n";

    PaletteState state;
    std::array<OKHsl, 5> target;
    for (int i = 0; i < 5; ++i) target[i] = { i * 72.0f, 0.9f, 0.7f };

    state.setTarget(target);
    TEST_ASSERT(state.isTransitioning(), "Transition starts");

    state.snapToTarget();
    TEST_ASSERT(!state.isTransitioning(), "Snap stops transition");
    auto current = state.getCurrent();
    TEST_ASSERT(NEAR(current[0].h, 0.0f, 0.01f), "Snap matches target");

    auto uniforms = state.getShaderUniform();
    TEST_ASSERT(uniforms.size() == 20, "Shader uniform has 20 floats");
}

void demoPaletteGeneration() {
    std::cout << "\n=== DEMO: All 10 Harmony Modes at Base Hue 30 (Orange) ===\n";
    ColorTheoryEngine engine;
    engine.setBaseHue(30.0f);
    engine.setBaseSaturation(0.85f);
    engine.setBaseLightness(0.65f);

    for (int m = 0; m < static_cast<int>(HarmonyMode::Count); ++m) {
        engine.setHarmonyMode(static_cast<HarmonyMode>(m));
        auto rgb = engine.generatePaletteRGB();
        printf("  %-20s ", harmonyModeToString(static_cast<HarmonyMode>(m)).c_str());
        for (int i = 0; i < 5; ++i) printf("#%06X ", rgb[i].toHex());
        printf("\n");
    }
}

int main() {
    std::cout << "DVDs-RGB-Audio-VST-Suite -- Standalone Test Runner\n";
    std::cout << "==================================================\n";

    testOKLab();
    testColorTheory();
    testFFT();
    testBandSplitter();
    testEnvelopeFollower();
    testAudioColorMapper();
    testPaletteState();
    demoPaletteGeneration();

    std::cout << "\n==================================================\n";
    std::cout << "Results: " << passed << " passed, " << failed << " failed\n";

    if (failed > 0) {
        std::cout << "SOME TESTS FAILED!\n";
        return 1;
    }
    std::cout << "ALL TESTS PASSED!\n";
    return 0;
}
