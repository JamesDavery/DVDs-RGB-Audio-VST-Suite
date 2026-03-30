#include <gtest/gtest.h>
#include "Core/OKLab.h"
#include "Core/ColorTheoryEngine.h"
#include "Core/PaletteState.h"

using namespace dvds;

TEST(OKLabTest, SRGBRoundTrip) {
    RGB input(0.5f, 0.3f, 0.8f);
    OKLab lab = rgbToOklab(input);
    RGB output = oklabToRgb(lab);
    EXPECT_NEAR(input.r, output.r, 0.01f);
    EXPECT_NEAR(input.g, output.g, 0.01f);
    EXPECT_NEAR(input.b, output.b, 0.01f);
}

TEST(OKLabTest, BlackAndWhite) {
    OKLab black = rgbToOklab(RGB(0, 0, 0));
    EXPECT_NEAR(black.L, 0.0f, 0.01f);

    OKLab white = rgbToOklab(RGB(1, 1, 1));
    EXPECT_NEAR(white.L, 1.0f, 0.01f);
}

TEST(OKLabTest, HexConversion) {
    RGB c = RGB::fromHex(0xFF8800);
    EXPECT_NEAR(c.r, 1.0f, 0.01f);
    EXPECT_NEAR(c.g, 0.533f, 0.01f);
    EXPECT_NEAR(c.b, 0.0f, 0.01f);

    uint32_t hex = c.toHex();
    EXPECT_EQ(hex, 0xFF8800);
}

TEST(OKLabTest, WrapHue) {
    EXPECT_FLOAT_EQ(wrapHue(0.0f), 0.0f);
    EXPECT_FLOAT_EQ(wrapHue(360.0f), 0.0f);
    EXPECT_FLOAT_EQ(wrapHue(-30.0f), 330.0f);
    EXPECT_FLOAT_EQ(wrapHue(720.0f), 0.0f);
    EXPECT_FLOAT_EQ(wrapHue(450.0f), 90.0f);
}

TEST(OKLabTest, HueDistance) {
    EXPECT_NEAR(hueDistance(10.0f, 350.0f), -20.0f, 0.01f);
    EXPECT_NEAR(hueDistance(350.0f, 10.0f), 20.0f, 0.01f);
    EXPECT_NEAR(hueDistance(0.0f, 180.0f), 180.0f, 0.01f);
}

TEST(OKLabTest, LerpHue) {
    float h = lerpHue(350.0f, 10.0f, 0.5f);
    EXPECT_NEAR(h, 0.0f, 0.01f);
}

TEST(OKLabTest, ClampRgb) {
    RGB out = clampRgb(RGB(1.5f, -0.3f, 0.5f));
    EXPECT_FLOAT_EQ(out.r, 1.0f);
    EXPECT_FLOAT_EQ(out.g, 0.0f);
    EXPECT_FLOAT_EQ(out.b, 0.5f);
}

TEST(OKLabTest, PaletteToShaderUniform) {
    std::array<OKHsl, 5> palette;
    for (int i = 0; i < 5; ++i)
        palette[i] = { i * 72.0f, 0.8f, 0.6f };
    auto uniforms = paletteToShaderUniform(palette);
    EXPECT_EQ(uniforms.size(), 20u);
    for (int i = 0; i < 5; ++i) {
        EXPECT_GE(uniforms[i*4], 0.0f);
        EXPECT_LE(uniforms[i*4], 1.0f);
        EXPECT_GE(uniforms[i*4+1], 0.0f);
        EXPECT_LE(uniforms[i*4+1], 1.0f);
        EXPECT_GE(uniforms[i*4+2], 0.0f);
        EXPECT_LE(uniforms[i*4+2], 1.0f);
    }
}

// -- ColorTheoryEngine tests --

TEST(ColorTheoryTest, AllModesGenerate5Colors) {
    ColorTheoryEngine engine;
    for (int m = 0; m < static_cast<int>(HarmonyMode::Count); ++m) {
        engine.setHarmonyMode(static_cast<HarmonyMode>(m));
        auto palette = engine.generatePalette();
        EXPECT_EQ(palette.size(), 5u) << "Mode " << m;
    }
}

TEST(ColorTheoryTest, AnalogousHuesAreClose) {
    ColorTheoryEngine engine;
    engine.setHarmonyMode(HarmonyMode::Analogous);
    engine.setBaseHue(180.0f);
    engine.setSpread(30.0f);
    auto palette = engine.generatePalette();

    for (const auto& c : palette) {
        float dist = std::abs(hueDistance(180.0f, c.h));
        EXPECT_LT(dist, 70.0f) << "Hue " << c.h << " too far from base 180";
    }
}

TEST(ColorTheoryTest, ComplementaryHasOppositeHue) {
    ColorTheoryEngine engine;
    engine.setHarmonyMode(HarmonyMode::Complementary);
    engine.setBaseHue(60.0f);
    auto palette = engine.generatePalette();

    bool hasComplement = false;
    for (const auto& c : palette) {
        float dist = std::abs(hueDistance(60.0f, c.h));
        if (dist > 170.0f) hasComplement = true;
    }
    EXPECT_TRUE(hasComplement);
}

TEST(ColorTheoryTest, TriadHas120DegreeSpacing) {
    ColorTheoryEngine engine;
    engine.setHarmonyMode(HarmonyMode::Triad);
    engine.setBaseHue(0.0f);
    auto palette = engine.generatePalette();

    bool has120 = false, has240 = false;
    for (const auto& c : palette) {
        float h = wrapHue(c.h);
        if (std::abs(h - 120.0f) < 5.0f) has120 = true;
        if (std::abs(h - 240.0f) < 5.0f) has240 = true;
    }
    EXPECT_TRUE(has120);
    EXPECT_TRUE(has240);
}

TEST(ColorTheoryTest, MonochromaticSameHue) {
    ColorTheoryEngine engine;
    engine.setHarmonyMode(HarmonyMode::Monochromatic);
    engine.setBaseHue(200.0f);
    auto palette = engine.generatePalette();

    for (const auto& c : palette) {
        EXPECT_NEAR(c.h, 200.0f, 0.1f);
    }
}

TEST(ColorTheoryTest, ShadesVaryingLightness) {
    ColorTheoryEngine engine;
    engine.setHarmonyMode(HarmonyMode::Shades);
    engine.setBaseHue(100.0f);
    auto palette = engine.generatePalette();

    for (int i = 1; i < 5; ++i) {
        EXPECT_LT(palette[i].l, palette[i-1].l)
            << "Shades should decrease in lightness";
    }
}

TEST(ColorTheoryTest, SquareHas4EquidistantHues) {
    ColorTheoryEngine engine;
    engine.setHarmonyMode(HarmonyMode::Square);
    engine.setBaseHue(45.0f);
    auto palette = engine.generatePalette();

    std::vector<float> expectedHues = { 45.0f, 135.0f, 225.0f, 315.0f };
    for (float expected : expectedHues) {
        bool found = false;
        for (const auto& c : palette) {
            if (std::abs(hueDistance(expected, c.h)) < 5.0f) { found = true; break; }
        }
        EXPECT_TRUE(found) << "Expected hue near " << expected;
    }
}

TEST(ColorTheoryTest, GlobalModifiersApply) {
    ColorTheoryEngine engine;
    engine.setBaseHue(0.0f);
    engine.setBaseSaturation(0.8f);
    engine.setGlobalSaturationMultiplier(0.5f);
    auto palette = engine.generatePalette();
    for (const auto& c : palette) {
        EXPECT_LE(c.s, 0.5f);
    }
}

TEST(ColorTheoryTest, HarmonyModeStringRoundTrip) {
    for (int m = 0; m < static_cast<int>(HarmonyMode::Count); ++m) {
        auto mode = static_cast<HarmonyMode>(m);
        std::string s = harmonyModeToString(mode);
        EXPECT_EQ(stringToHarmonyMode(s), mode) << "Mode: " << s;
    }
}

TEST(ColorTheoryTest, ComplementaryHueHelper) {
    EXPECT_NEAR(ColorTheoryEngine::getComplementaryHue(0.0f), 180.0f, 0.01f);
    EXPECT_NEAR(ColorTheoryEngine::getComplementaryHue(90.0f), 270.0f, 0.01f);
    EXPECT_NEAR(ColorTheoryEngine::getComplementaryHue(270.0f), 90.0f, 0.01f);
}

TEST(ColorTheoryTest, EvenlySpacedHues) {
    auto hues = ColorTheoryEngine::getEvenlySpacedHues(0.0f, 4);
    EXPECT_EQ(hues.size(), 4u);
    EXPECT_NEAR(hues[0], 0.0f, 0.01f);
    EXPECT_NEAR(hues[1], 90.0f, 0.01f);
    EXPECT_NEAR(hues[2], 180.0f, 0.01f);
    EXPECT_NEAR(hues[3], 270.0f, 0.01f);
}

// -- PaletteState tests --

TEST(PaletteStateTest, SnapToTarget) {
    PaletteState state;
    std::array<OKHsl, 5> target;
    for (int i = 0; i < 5; ++i) target[i] = { i * 72.0f, 0.9f, 0.7f };
    state.setTarget(target);
    state.snapToTarget();
    auto current = state.getCurrent();
    for (int i = 0; i < 5; ++i) {
        EXPECT_FLOAT_EQ(current[i].h, target[i].h);
        EXPECT_FLOAT_EQ(current[i].s, target[i].s);
        EXPECT_FLOAT_EQ(current[i].l, target[i].l);
    }
}

TEST(PaletteStateTest, TransitionProgresses) {
    PaletteState state;
    std::array<OKHsl, 5> target;
    for (int i = 0; i < 5; ++i) target[i] = { 180.0f, 0.9f, 0.7f };
    state.setTarget(target);
    EXPECT_TRUE(state.isTransitioning());
    state.update(0.15f); // half of default 0.3s transition
    EXPECT_TRUE(state.isTransitioning());
    state.update(0.2f);
    EXPECT_FALSE(state.isTransitioning());
}

TEST(PaletteStateTest, ShaderUniformFormat) {
    PaletteState state;
    auto uniforms = state.getShaderUniform();
    EXPECT_EQ(uniforms.size(), 20u);
}
