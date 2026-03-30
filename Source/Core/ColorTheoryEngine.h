#pragma once
#include "Core/OKLab.h"
#include <array>
#include <string>
#include <vector>
#include <functional>

namespace dvds {

enum class HarmonyMode {
    Analogous = 0,
    Monochromatic,
    Triad,
    Complementary,
    SplitComplementary,
    DoubleSplit,
    Square,
    Compound,
    Shades,
    Custom,
    Count
};

std::string harmonyModeToString(HarmonyMode mode);
HarmonyMode stringToHarmonyMode(const std::string& s);

struct HarmonyConfig {
    HarmonyMode mode = HarmonyMode::Analogous;
    float baseHue = 0.0f;          // 0-360 OKHsl hue
    float baseSaturation = 0.75f;  // 0-1
    float baseLightness = 0.65f;   // 0-1
    float spread = 30.0f;          // mode-dependent angular spread
    float globalSatMul = 1.0f;     // 0-2 multiplier
    float globalLightMul = 1.0f;   // 0-2 multiplier

    // Custom mode: 5 free handles (hue, sat, light)
    std::array<OKHsl, 5> customColors{};
};

class ColorTheoryEngine {
public:
    ColorTheoryEngine();

    void setConfig(const HarmonyConfig& config);
    const HarmonyConfig& getConfig() const { return config_; }

    void setBaseHue(float hue);
    void setHarmonyMode(HarmonyMode mode);
    void setSpread(float spread);
    void setBaseSaturation(float sat);
    void setBaseLightness(float light);
    void setGlobalSaturationMultiplier(float mul);
    void setGlobalLightnessMultiplier(float mul);
    void setCustomColor(int index, const OKHsl& color);

    // Generate the 5-color palette based on current config
    std::array<OKHsl, 5> generatePalette() const;

    // Generate palette as sRGB for display
    std::array<RGB, 5> generatePaletteRGB() const;

    // Generate palette as shader-ready float array (5x RGBA)
    std::array<float, 20> generateShaderPalette() const;

    // Get complementary color for a given hue
    static float getComplementaryHue(float hue);

    // Get N evenly spaced hues
    static std::vector<float> getEvenlySpacedHues(float baseHue, int n);

private:
    HarmonyConfig config_;

    std::array<OKHsl, 5> generateAnalogous() const;
    std::array<OKHsl, 5> generateMonochromatic() const;
    std::array<OKHsl, 5> generateTriad() const;
    std::array<OKHsl, 5> generateComplementary() const;
    std::array<OKHsl, 5> generateSplitComplementary() const;
    std::array<OKHsl, 5> generateDoubleSplit() const;
    std::array<OKHsl, 5> generateSquare() const;
    std::array<OKHsl, 5> generateCompound() const;
    std::array<OKHsl, 5> generateShades() const;
    std::array<OKHsl, 5> generateCustom() const;

    OKHsl applyGlobalModifiers(const OKHsl& color) const;
};

} // namespace dvds
