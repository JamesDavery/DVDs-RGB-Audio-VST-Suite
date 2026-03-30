#include "Audio/AudioColorMapper.h"
#include <cmath>
#include <algorithm>

namespace dvds {

AudioColorMapper::AudioColorMapper() {
    OKHsl def = { 0.0f, 0.75f, 0.65f };
    modulatedPalette_.fill(def);
}

void AudioColorMapper::setConfig(const AudioColorConfig& config) {
    config_ = config;
}

void AudioColorMapper::process(
    const ColorTheoryEngine& engine,
    const BandSplitter& bands,
    const MultiBandEnvelope& envelopes,
    const BeatDetector& beat,
    float rms,
    float deltaTime
) {
    float depth = config_.reactDepth;

    // 1. RMS-driven base hue rotation
    hueAccumulator_ += rms * config_.hueRotationSpeed * depth * deltaTime * 360.0f;
    hueAccumulator_ = std::fmod(hueAccumulator_, 360.0f);

    float baseHue = wrapHue(engine.getConfig().baseHue + hueAccumulator_);
    modulatedBaseHue_ = baseHue;

    // 2. Beat-driven hue jump
    if (beat.isBeat() && config_.beatSyncEnabled && config_.beatHueJump > 0.0f) {
        hueAccumulator_ += config_.beatHueJump;
        hueAccumulator_ = std::fmod(hueAccumulator_, 360.0f);
        baseHue = wrapHue(engine.getConfig().baseHue + hueAccumulator_);
        modulatedBaseHue_ = baseHue;
    }

    // 3. Beat flash
    if (beat.isBeat() && config_.beatSyncEnabled) {
        beatFlash_ = config_.beatFlashIntensity * depth;
    }
    beatFlash_ *= std::exp(-10.0f * deltaTime); // fast decay

    // 4. Generate base palette with modulated hue
    HarmonyConfig modConfig = engine.getConfig();
    modConfig.baseHue = baseHue;
    ColorTheoryEngine modEngine;
    modEngine.setConfig(modConfig);
    auto basePalette = modEngine.generatePalette();

    // 5. Per-band modulation of palette colors
    const auto& normEnergy = bands.getAllNormalizedEnergies();

    float subE   = normEnergy[static_cast<int>(FrequencyBand::Sub)]      * depth;
    float lowE   = normEnergy[static_cast<int>(FrequencyBand::Low)]      * depth;
    float midE   = normEnergy[static_cast<int>(FrequencyBand::Mid)]      * depth;
    float hiMidE = normEnergy[static_cast<int>(FrequencyBand::HighMid)]  * depth;
    float presE  = normEnergy[static_cast<int>(FrequencyBand::Presence)] * depth;
    float airE   = normEnergy[static_cast<int>(FrequencyBand::Air)]      * depth;

    for (int i = 0; i < 5; ++i) {
        OKHsl color = basePalette[i];

        // Sub -> Lightness pulsing (darker on low energy, brighter on high)
        float lightMod = (subE - 0.5f) * config_.subLightMod * 0.3f;
        color.l = std::clamp(color.l + lightMod, 0.05f, 0.95f);

        // Low -> Saturation modulation
        float satMod = lowE * config_.lowSatMod * 0.3f;
        color.s = std::clamp(color.s + satMod, 0.0f, 1.0f);

        // Mid -> Hue micro-shift within harmony spread
        float hueMod = (midE - 0.5f) * config_.midHueMod * modConfig.spread * 0.5f;
        color.h = wrapHue(color.h + hueMod);

        // Hi-Mid -> Accent intensity (boost sat+light on accent colors [1,3])
        if (i == 1 || i == 3) {
            float accentBoost = hiMidE * config_.hiMidAccentMod * 0.2f;
            color.s = std::clamp(color.s + accentBoost, 0.0f, 1.0f);
            color.l = std::clamp(color.l + accentBoost * 0.5f, 0.05f, 0.95f);
        }

        // Presence -> Shimmer (lightness oscillation)
        float shimmer = presE * config_.presenceShimmerMod * 0.15f;
        color.l = std::clamp(color.l + shimmer, 0.05f, 0.95f);

        // Beat flash -> momentary lightness boost
        color.l = std::clamp(color.l + beatFlash_ * 0.2f, 0.05f, 0.98f);

        modulatedPalette_[i] = color;
    }
}

} // namespace dvds
