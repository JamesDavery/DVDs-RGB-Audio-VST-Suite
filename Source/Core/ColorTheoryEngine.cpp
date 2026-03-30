#include "Core/ColorTheoryEngine.h"
#include <cmath>
#include <algorithm>

namespace dvds {

std::string harmonyModeToString(HarmonyMode mode) {
    switch (mode) {
        case HarmonyMode::Analogous:          return "Analogous";
        case HarmonyMode::Monochromatic:      return "Monochromatic";
        case HarmonyMode::Triad:              return "Triad";
        case HarmonyMode::Complementary:      return "Complementary";
        case HarmonyMode::SplitComplementary: return "Split-Complementary";
        case HarmonyMode::DoubleSplit:        return "Double-Split";
        case HarmonyMode::Square:             return "Square";
        case HarmonyMode::Compound:           return "Compound";
        case HarmonyMode::Shades:             return "Shades";
        case HarmonyMode::Custom:             return "Custom";
        default:                              return "Unknown";
    }
}

HarmonyMode stringToHarmonyMode(const std::string& s) {
    if (s == "Analogous")          return HarmonyMode::Analogous;
    if (s == "Monochromatic")      return HarmonyMode::Monochromatic;
    if (s == "Triad")              return HarmonyMode::Triad;
    if (s == "Complementary")      return HarmonyMode::Complementary;
    if (s == "Split-Complementary") return HarmonyMode::SplitComplementary;
    if (s == "Double-Split")       return HarmonyMode::DoubleSplit;
    if (s == "Square")             return HarmonyMode::Square;
    if (s == "Compound")           return HarmonyMode::Compound;
    if (s == "Shades")             return HarmonyMode::Shades;
    if (s == "Custom")             return HarmonyMode::Custom;
    return HarmonyMode::Analogous;
}

ColorTheoryEngine::ColorTheoryEngine() {
    config_.baseHue = 0.0f;
    config_.baseSaturation = 0.75f;
    config_.baseLightness = 0.65f;
    config_.spread = 30.0f;
    config_.mode = HarmonyMode::Analogous;
    for (int i = 0; i < 5; ++i)
        config_.customColors[i] = { wrapHue(i * 72.0f), 0.75f, 0.65f };
}

void ColorTheoryEngine::setConfig(const HarmonyConfig& config) { config_ = config; }
void ColorTheoryEngine::setBaseHue(float hue) { config_.baseHue = wrapHue(hue); }
void ColorTheoryEngine::setHarmonyMode(HarmonyMode mode) { config_.mode = mode; }
void ColorTheoryEngine::setSpread(float spread) { config_.spread = std::clamp(spread, 1.0f, 180.0f); }
void ColorTheoryEngine::setBaseSaturation(float sat) { config_.baseSaturation = std::clamp(sat, 0.0f, 1.0f); }
void ColorTheoryEngine::setBaseLightness(float light) { config_.baseLightness = std::clamp(light, 0.0f, 1.0f); }
void ColorTheoryEngine::setGlobalSaturationMultiplier(float mul) { config_.globalSatMul = std::clamp(mul, 0.0f, 2.0f); }
void ColorTheoryEngine::setGlobalLightnessMultiplier(float mul) { config_.globalLightMul = std::clamp(mul, 0.0f, 2.0f); }

void ColorTheoryEngine::setCustomColor(int index, const OKHsl& color) {
    if (index >= 0 && index < 5) config_.customColors[index] = color;
}

OKHsl ColorTheoryEngine::applyGlobalModifiers(const OKHsl& color) const {
    return {
        color.h,
        std::clamp(color.s * config_.globalSatMul, 0.0f, 1.0f),
        std::clamp(color.l * config_.globalLightMul, 0.0f, 1.0f)
    };
}

std::array<OKHsl, 5> ColorTheoryEngine::generatePalette() const {
    std::array<OKHsl, 5> raw;
    switch (config_.mode) {
        case HarmonyMode::Analogous:          raw = generateAnalogous(); break;
        case HarmonyMode::Monochromatic:      raw = generateMonochromatic(); break;
        case HarmonyMode::Triad:              raw = generateTriad(); break;
        case HarmonyMode::Complementary:      raw = generateComplementary(); break;
        case HarmonyMode::SplitComplementary: raw = generateSplitComplementary(); break;
        case HarmonyMode::DoubleSplit:        raw = generateDoubleSplit(); break;
        case HarmonyMode::Square:             raw = generateSquare(); break;
        case HarmonyMode::Compound:           raw = generateCompound(); break;
        case HarmonyMode::Shades:             raw = generateShades(); break;
        case HarmonyMode::Custom:             raw = generateCustom(); break;
        default:                              raw = generateAnalogous(); break;
    }
    for (auto& c : raw) c = applyGlobalModifiers(c);
    return raw;
}

std::array<RGB, 5> ColorTheoryEngine::generatePaletteRGB() const {
    auto palette = generatePalette();
    std::array<RGB, 5> out;
    for (int i = 0; i < 5; ++i) out[i] = clampRgb(okhslToRgb(palette[i]));
    return out;
}

std::array<float, 20> ColorTheoryEngine::generateShaderPalette() const {
    return paletteToShaderUniform(generatePalette());
}

float ColorTheoryEngine::getComplementaryHue(float hue) {
    return wrapHue(hue + 180.0f);
}

std::vector<float> ColorTheoryEngine::getEvenlySpacedHues(float baseHue, int n) {
    std::vector<float> hues;
    hues.reserve(n);
    float step = 360.0f / static_cast<float>(n);
    for (int i = 0; i < n; ++i)
        hues.push_back(wrapHue(baseHue + step * i));
    return hues;
}

// -- Analogous: base hue with 4 neighboring hues spread evenly --
std::array<OKHsl, 5> ColorTheoryEngine::generateAnalogous() const {
    float h = config_.baseHue;
    float sp = config_.spread;
    float s = config_.baseSaturation;
    float l = config_.baseLightness;
    return {{
        { wrapHue(h - sp * 2.0f), s * 0.85f, l * 0.9f  },
        { wrapHue(h - sp),        s * 0.95f, l * 0.95f },
        { h,                       s,         l          },
        { wrapHue(h + sp),        s * 0.95f, l * 1.05f },
        { wrapHue(h + sp * 2.0f), s * 0.85f, l * 1.1f  }
    }};
}

// -- Monochromatic: same hue, 5 lightness/saturation steps --
std::array<OKHsl, 5> ColorTheoryEngine::generateMonochromatic() const {
    float h = config_.baseHue;
    float s = config_.baseSaturation;
    float l = config_.baseLightness;
    return {{
        { h, s * 0.3f,  l * 0.4f  },
        { h, s * 0.6f,  l * 0.65f },
        { h, s,         l          },
        { h, s * 0.8f,  l * 1.2f  },
        { h, s * 0.5f,  l * 1.45f }
    }};
}

// -- Triad: 3 equidistant hues + 2 tint/shade variants --
std::array<OKHsl, 5> ColorTheoryEngine::generateTriad() const {
    float h = config_.baseHue;
    float s = config_.baseSaturation;
    float l = config_.baseLightness;
    return {{
        { h,                  s,         l          },
        { wrapHue(h + 120.0f), s * 0.9f, l * 0.9f  },
        { wrapHue(h + 240.0f), s * 0.9f, l * 0.9f  },
        { wrapHue(h + 120.0f), s * 0.7f, l * 1.15f },
        { wrapHue(h + 240.0f), s * 0.7f, l * 1.15f }
    }};
}

// -- Complementary: base + opposite + 3 variants --
std::array<OKHsl, 5> ColorTheoryEngine::generateComplementary() const {
    float h = config_.baseHue;
    float comp = wrapHue(h + 180.0f);
    float s = config_.baseSaturation;
    float l = config_.baseLightness;
    return {{
        { h,    s * 0.6f,  l * 0.5f  },
        { h,    s,         l          },
        { h,    s * 0.7f,  l * 1.3f  },
        { comp, s,         l          },
        { comp, s * 0.7f,  l * 1.3f  }
    }};
}

// -- Split-Complementary: base + two flanks of complement --
std::array<OKHsl, 5> ColorTheoryEngine::generateSplitComplementary() const {
    float h = config_.baseHue;
    float s = config_.baseSaturation;
    float l = config_.baseLightness;
    float flankAngle = 30.0f;
    return {{
        { h,                                 s,         l          },
        { wrapHue(h + 180.0f - flankAngle),  s * 0.9f,  l * 0.9f  },
        { wrapHue(h + 180.0f + flankAngle),  s * 0.9f,  l * 0.9f  },
        { wrapHue(h + 180.0f - flankAngle),  s * 0.65f, l * 1.2f  },
        { wrapHue(h + 180.0f + flankAngle),  s * 0.65f, l * 1.2f  }
    }};
}

// -- Double-Split: two complementary pairs (base+comp + shifted pair) --
std::array<OKHsl, 5> ColorTheoryEngine::generateDoubleSplit() const {
    float h = config_.baseHue;
    float s = config_.baseSaturation;
    float l = config_.baseLightness;
    float shift = config_.spread;
    return {{
        { h,                              s,         l          },
        { wrapHue(h + shift),            s * 0.9f,  l * 0.95f  },
        { wrapHue(h + 180.0f),           s * 0.9f,  l * 0.95f  },
        { wrapHue(h + 180.0f + shift),   s * 0.85f, l * 1.05f  },
        { wrapHue(h + shift * 0.5f),     s * 0.7f,  l * 1.2f   }
    }};
}

// -- Square: 4 equidistant hues at 90-degree intervals --
std::array<OKHsl, 5> ColorTheoryEngine::generateSquare() const {
    float h = config_.baseHue;
    float s = config_.baseSaturation;
    float l = config_.baseLightness;
    return {{
        { h,                  s,         l          },
        { wrapHue(h + 90.0f),  s * 0.9f, l * 0.9f  },
        { wrapHue(h + 180.0f), s * 0.9f, l * 0.95f },
        { wrapHue(h + 270.0f), s * 0.9f, l * 1.05f },
        { h,                   s * 0.6f, l * 1.3f   }
    }};
}

// -- Compound: mix of analogous + complementary accent (Adobe's approach) --
std::array<OKHsl, 5> ColorTheoryEngine::generateCompound() const {
    float h = config_.baseHue;
    float s = config_.baseSaturation;
    float l = config_.baseLightness;
    float sp = config_.spread;
    float comp = wrapHue(h + 180.0f);
    return {{
        { wrapHue(h - sp),   s * 0.85f, l * 0.9f  },
        { h,                  s,         l          },
        { wrapHue(h + sp),   s * 0.85f, l * 1.1f   },
        { comp,               s * 0.75f, l * 0.85f  },
        { wrapHue(comp + sp * 0.5f), s * 0.6f, l * 1.2f }
    }};
}

// -- Shades: single hue, 5 darkness levels from light to dark --
std::array<OKHsl, 5> ColorTheoryEngine::generateShades() const {
    float h = config_.baseHue;
    float s = config_.baseSaturation;
    return {{
        { h, s, 0.9f  },
        { h, s, 0.72f },
        { h, s, 0.55f },
        { h, s, 0.38f },
        { h, s, 0.2f  }
    }};
}

// -- Custom: user-defined 5 handles --
std::array<OKHsl, 5> ColorTheoryEngine::generateCustom() const {
    return config_.customColors;
}

} // namespace dvds
