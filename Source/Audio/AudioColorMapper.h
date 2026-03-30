#pragma once
#include "Core/OKLab.h"
#include "Core/ColorTheoryEngine.h"
#include "Core/PaletteState.h"
#include "Audio/BandSplitter.h"
#include "Audio/EnvelopeFollower.h"
#include "Audio/BeatDetector.h"
#include <array>

namespace dvds {

struct AudioColorConfig {
    float reactDepth = 0.75f;         // 0-1 overall reactivity
    float hueRotationSpeed = 0.1f;    // speed of RMS-driven hue rotation
    float subLightMod = 0.5f;         // sub bass -> lightness
    float lowSatMod = 0.4f;           // low -> saturation
    float midHueMod = 0.3f;           // mid -> hue micro-shift
    float hiMidAccentMod = 0.5f;      // hi-mid -> accent intensity
    float presenceShimmerMod = 0.3f;  // presence -> brightness shimmer
    float airAlphaMod = 0.2f;         // air -> alpha/opacity
    float beatHueJump = 0.0f;         // degrees to jump on beat (0 = disabled)
    float beatFlashIntensity = 0.5f;  // flash on beat
    bool beatSyncEnabled = true;
};

class AudioColorMapper {
public:
    AudioColorMapper();

    void setConfig(const AudioColorConfig& config);
    const AudioColorConfig& getConfig() const { return config_; }

    void process(
        const ColorTheoryEngine& engine,
        const BandSplitter& bands,
        const MultiBandEnvelope& envelopes,
        const BeatDetector& beat,
        float rms,
        float deltaTime
    );

    const std::array<OKHsl, 5>& getModulatedPalette() const { return modulatedPalette_; }
    float getModulatedBaseHue() const { return modulatedBaseHue_; }
    float getBeatFlash() const { return beatFlash_; }

private:
    AudioColorConfig config_;
    std::array<OKHsl, 5> modulatedPalette_;
    float modulatedBaseHue_ = 0.0f;
    float hueAccumulator_ = 0.0f;
    float beatFlash_ = 0.0f;
};

} // namespace dvds
