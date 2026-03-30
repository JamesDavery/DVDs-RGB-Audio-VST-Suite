#pragma once
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif
#include "SynthVoice.h"
#include "EffectsChain.h"
#include "../Core/ColorTheoryEngine.h"
#include "../Core/PaletteState.h"
#include "../Audio/BandSplitter.h"
#include <array>

namespace dvds {

struct ColorSynthConfig {
    float cutoffFromHue = 0.3f;       // palette hue -> filter cutoff mod
    float resoFromSat = 0.3f;         // palette saturation -> resonance mod
    float fmIndexFromLight = 0.3f;    // palette lightness -> FM mod index
    float wtPosFromHue = 0.3f;        // palette hue -> wavetable position
    float detuneFromSpread = 0.2f;    // palette spread -> osc detune
    float reverbFromAir = 0.3f;       // air band energy -> reverb mix
    float delayFromPresence = 0.2f;   // presence band -> delay feedback
    float chorusFromMid = 0.2f;       // mid band -> chorus depth
    float enabled = 1.0f;
};

class ColorSynthMapper {
public:
    void setConfig(const ColorSynthConfig& cfg) { config_ = cfg; }
    const ColorSynthConfig& getConfig() const { return config_; }

    void update(const std::array<OKHsl, 5>& palette,
                const std::array<float, 6>& bandEnergies,
                float rms)
    {
        if (config_.enabled < 0.5f) {
            synthCutoffMod_ = 0.0f;
            synthResMod_ = 0.0f;
            synthFMIndexMod_ = 0.0f;
            synthWTPosMod_ = 0.0f;
            synthDetuneMod_ = 0.0f;
            fxReverbMod_ = 0.0f;
            fxDelayMod_ = 0.0f;
            fxChorusMod_ = 0.0f;
            return;
        }

        // Use primary palette color for synth modulation
        float hueNorm = palette[0].h / 360.0f;
        float sat = palette[0].s;
        float light = palette[0].l;

        // Hue spread (angular distance between palette colors)
        float hueSpread = 0.0f;
        for (int i = 1; i < 5; ++i) {
            float diff = std::abs(palette[i].h - palette[0].h);
            if (diff > 180.0f) diff = 360.0f - diff;
            hueSpread += diff;
        }
        hueSpread /= (4.0f * 180.0f); // normalize to 0-1

        // Synth parameter modulations
        // Hue maps to cutoff: warm colors (red/orange 0-60) -> lower, cool (blue 180-270) -> higher
        float warmCool = std::sin(hueNorm * 2.0f * (float)M_PI); // -1 to 1
        synthCutoffMod_ = warmCool * config_.cutoffFromHue * 3000.0f;

        // Saturation maps to resonance: more saturated -> more resonance
        synthResMod_ = sat * config_.resoFromSat * 0.5f;

        // Lightness maps to FM index: brighter -> more harmonics
        synthFMIndexMod_ = light * config_.fmIndexFromLight * 3.0f;

        // Hue maps to wavetable position
        synthWTPosMod_ = hueNorm * config_.wtPosFromHue;

        // Spread maps to detune
        synthDetuneMod_ = hueSpread * config_.detuneFromSpread * 20.0f;

        // Band energies modulate effects
        fxReverbMod_ = bandEnergies[5] * config_.reverbFromAir * 0.5f; // Air
        fxDelayMod_ = bandEnergies[4] * config_.delayFromPresence * 0.4f; // Presence
        fxChorusMod_ = bandEnergies[2] * config_.chorusFromMid * 5.0f; // Mid
    }

    void applySynthParams(SynthParams& params) const {
        params.colorCutoffMod = synthCutoffMod_;
        params.colorResMod = synthResMod_;
        params.colorFMIndexMod = synthFMIndexMod_;
        params.colorWTPosMod = synthWTPosMod_;
        params.colorDetuneMod = synthDetuneMod_;
    }

    void applyEffectsParams(EffectsParams& params) const {
        params.reverbMix = std::clamp(params.reverbMix + fxReverbMod_, 0.0f, 1.0f);
        params.delayFeedback = std::clamp(params.delayFeedback + fxDelayMod_, 0.0f, 0.95f);
        params.chorusDepth = std::clamp(params.chorusDepth + fxChorusMod_, 0.0f, 15.0f);
    }

private:
    ColorSynthConfig config_;
    float synthCutoffMod_ = 0.0f;
    float synthResMod_ = 0.0f;
    float synthFMIndexMod_ = 0.0f;
    float synthWTPosMod_ = 0.0f;
    float synthDetuneMod_ = 0.0f;
    float fxReverbMod_ = 0.0f;
    float fxDelayMod_ = 0.0f;
    float fxChorusMod_ = 0.0f;
};

} // namespace dvds
