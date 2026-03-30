#pragma once
#include <cmath>
#include <algorithm>
#include <array>

namespace dvds {

struct RGB {
    float r = 0.0f, g = 0.0f, b = 0.0f, a = 1.0f;

    RGB() = default;
    RGB(float r, float g, float b, float a = 1.0f) : r(r), g(g), b(b), a(a) {}

    static RGB fromHex(uint32_t hex);
    uint32_t toHex() const;

    bool operator==(const RGB& o) const {
        return r == o.r && g == o.g && b == o.b && a == o.a;
    }
};

struct LinearRGB {
    float r = 0.0f, g = 0.0f, b = 0.0f;
};

struct OKLab {
    float L = 0.0f, a = 0.0f, b = 0.0f;
};

struct OKHsl {
    float h = 0.0f; // 0-360
    float s = 0.0f; // 0-1
    float l = 0.0f; // 0-1
};

struct OKHsv {
    float h = 0.0f; // 0-360
    float s = 0.0f; // 0-1
    float v = 0.0f; // 0-1
};

// sRGB <-> Linear RGB
LinearRGB srgbToLinear(const RGB& c);
RGB linearToSrgb(const LinearRGB& c);

// Linear RGB <-> OKLab
OKLab linearToOklab(const LinearRGB& c);
LinearRGB oklabToLinear(const OKLab& c);

// OKLab <-> OKHsl
OKHsl oklabToOkhsl(const OKLab& c);
OKLab okhslToOklab(const OKHsl& c);

// OKLab <-> OKHsv
OKHsv oklabToOkhsv(const OKLab& c);
OKLab okhsvToOklab(const OKHsv& c);

// Convenience: sRGB <-> OKHsl direct
OKHsl rgbToOkhsl(const RGB& c);
RGB okhslToRgb(const OKHsl& c);

// Convenience: sRGB <-> OKLab direct
OKLab rgbToOklab(const RGB& c);
RGB oklabToRgb(const OKLab& c);

// Perceptual interpolation in OKLab space
OKLab lerpOklab(const OKLab& a, const OKLab& b, float t);
OKHsl lerpOkhsl(const OKHsl& a, const OKHsl& b, float t);

// Hue wrapping utility
float wrapHue(float h);
float hueDistance(float h1, float h2);
float lerpHue(float h1, float h2, float t);

// Clamp to valid sRGB gamut
RGB clampRgb(const RGB& c);
OKLab gamutClipOklab(const OKLab& c);

// Convert OKHsl palette to float array for shader uniforms (RGBA floats)
std::array<float, 20> paletteToShaderUniform(const std::array<OKHsl, 5>& palette);

} // namespace dvds
