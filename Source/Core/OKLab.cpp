#include "Core/OKLab.h"
#include <cmath>
#include <algorithm>

namespace dvds {

// -- sRGB component transfer functions --

static float srgbTransfer(float a) {
    return a >= 0.0031308f
        ? 1.055f * std::pow(a, 1.0f / 2.4f) - 0.055f
        : 12.92f * a;
}

static float srgbTransferInv(float a) {
    return a >= 0.04045f
        ? std::pow((a + 0.055f) / 1.055f, 2.4f)
        : a / 12.92f;
}

// -- RGB helpers --

RGB RGB::fromHex(uint32_t hex) {
    return RGB(
        ((hex >> 16) & 0xFF) / 255.0f,
        ((hex >> 8) & 0xFF) / 255.0f,
        (hex & 0xFF) / 255.0f,
        1.0f
    );
}

uint32_t RGB::toHex() const {
    auto clamp01 = [](float v) { return std::clamp(v, 0.0f, 1.0f); };
    uint32_t ri = static_cast<uint32_t>(clamp01(r) * 255.0f + 0.5f);
    uint32_t gi = static_cast<uint32_t>(clamp01(g) * 255.0f + 0.5f);
    uint32_t bi = static_cast<uint32_t>(clamp01(b) * 255.0f + 0.5f);
    return (ri << 16) | (gi << 8) | bi;
}

// -- sRGB <-> Linear RGB --

LinearRGB srgbToLinear(const RGB& c) {
    return { srgbTransferInv(c.r), srgbTransferInv(c.g), srgbTransferInv(c.b) };
}

RGB linearToSrgb(const LinearRGB& c) {
    return { srgbTransfer(c.r), srgbTransfer(c.g), srgbTransfer(c.b), 1.0f };
}

// -- Linear RGB <-> OKLab (Bjorn Ottosson's reference implementation) --

OKLab linearToOklab(const LinearRGB& c) {
    float l = 0.4122214708f * c.r + 0.5363325363f * c.g + 0.0514459929f * c.b;
    float m = 0.2119034982f * c.r + 0.6806995451f * c.g + 0.1073969566f * c.b;
    float s = 0.0883024619f * c.r + 0.2817188376f * c.g + 0.6299787005f * c.b;

    float l_ = std::cbrt(l);
    float m_ = std::cbrt(m);
    float s_ = std::cbrt(s);

    return {
        0.2104542553f * l_ + 0.7936177850f * m_ - 0.0040720468f * s_,
        1.9779984951f * l_ - 2.4285922050f * m_ + 0.4505937099f * s_,
        0.0259040371f * l_ + 0.7827717662f * m_ - 0.8086757660f * s_
    };
}

LinearRGB oklabToLinear(const OKLab& c) {
    float l_ = c.L + 0.3963377774f * c.a + 0.2158037573f * c.b;
    float m_ = c.L - 0.1055613458f * c.a - 0.0638541728f * c.b;
    float s_ = c.L - 0.0894841775f * c.a - 1.2914855480f * c.b;

    float l = l_ * l_ * l_;
    float m = m_ * m_ * m_;
    float s = s_ * s_ * s_;

    return {
         4.0767416621f * l - 3.3077115913f * m + 0.2309699292f * s,
        -1.2684380046f * l + 2.6097574011f * m - 0.3413193965f * s,
        -0.0041960863f * l - 0.7034186147f * m + 1.7076147010f * s
    };
}

// -- OKLab <-> OKHsl/OKHsv helpers --

static float computeMaxSaturation(float a, float b) {
    float S, k0, k1, k2, k3, k4, wl, wm, ws;

    if (-1.88170328f * a - 0.80936493f * b > 1.0f) {
        k0 = 1.19086277f; k1 = 1.76576728f; k2 = 0.59662641f;
        k3 = 0.75515197f; k4 = 0.56771245f;
        wl = 4.0767416621f; wm = -3.3077115913f; ws = 0.2309699292f;
    } else if (1.81444104f * a - 1.19445276f * b > 1.0f) {
        k0 = 0.73956515f; k1 = -0.45954404f; k2 = 0.08285427f;
        k3 = 0.12541070f; k4 = -0.14503204f;
        wl = -1.2684380046f; wm = 2.6097574011f; ws = -0.3413193965f;
    } else {
        k0 = 1.35733652f; k1 = -0.00915799f; k2 = -1.15130210f;
        k3 = -0.50559606f; k4 = 0.00692167f;
        wl = -0.0041960863f; wm = -0.7034186147f; ws = 1.7076147010f;
    }

    S = k0 + k1 * a + k2 * b + k3 * a * a + k4 * a * b;

    float k_l = 0.3963377774f * a + 0.2158037573f * b;
    float k_m = -0.1055613458f * a - 0.0638541728f * b;
    float k_s = -0.0894841775f * a - 1.2914855480f * b;

    {
        float l_ = 1.0f + S * k_l;
        float m_ = 1.0f + S * k_m;
        float s_ = 1.0f + S * k_s;

        float l = l_ * l_ * l_;
        float m = m_ * m_ * m_;
        float s = s_ * s_ * s_;

        float l_dS = 3.0f * k_l * l_ * l_;
        float m_dS = 3.0f * k_m * m_ * m_;
        float s_dS = 3.0f * k_s * s_ * s_;

        float l_dS2 = 6.0f * k_l * k_l * l_;
        float m_dS2 = 6.0f * k_m * k_m * m_;
        float s_dS2 = 6.0f * k_s * k_s * s_;

        float f = wl * l + wm * m + ws * s;
        float f1 = wl * l_dS + wm * m_dS + ws * s_dS;
        float f2 = wl * l_dS2 + wm * m_dS2 + ws * s_dS2;

        S = S - f * f1 / (f1 * f1 - 0.5f * f * f2);
        S = S - f * f1 / (f1 * f1 - 0.5f * f * f2);
    }

    return S;
}

struct LC { float L; float C; };

static LC findCusp(float a, float b) {
    float S_cusp = computeMaxSaturation(a, b);
    LinearRGB rgb = oklabToLinear({ 1.0f, S_cusp * a, S_cusp * b });
    float L_cusp = std::cbrt(1.0f / std::max({ rgb.r, rgb.g, rgb.b }));
    float C_cusp = L_cusp * S_cusp;
    return { L_cusp, C_cusp };
}

static float toeInv(float x) {
    const float k1 = 0.206f, k2 = 0.03f, k3 = (1.0f + k1) / (1.0f + k2);
    return (x * x + k1 * x) / (k3 * (x + k2));
}

static float toe(float x) {
    const float k1 = 0.206f, k2 = 0.03f, k3 = (1.0f + k1) / (1.0f + k2);
    return 0.5f * (k3 * x - k1 + std::sqrt((k3 * x - k1) * (k3 * x - k1) + 4.0f * k2 * k3 * x));
}

static std::pair<float, float> getStMid(float a_, float b_) {
    float S = 0.11516375f + 1.0f / (
        7.44778970f + 4.15901240f * b_
        + a_ * (-2.19557347f + 1.75198401f * b_
        + a_ * (-2.13704948f - 10.02301043f * b_
        + a_ * (-4.24894561f + 5.38770819f * b_ + 4.69891013f * a_)))
    );
    float T = 0.11239642f + 1.0f / (
        1.61320320f - 0.68124379f * b_
        + a_ * (0.40370612f + 0.90148123f * b_
        + a_ * (-0.27087943f + 0.61223990f * b_
        + a_ * (0.00299215f - 0.45399568f * b_ - 0.14661872f * a_)))
    );
    return { S, T };
}

static std::pair<float, float> getCs(float L, float a_, float b_) {
    LC cusp = findCusp(a_, b_);

    float C_max = findCusp(a_, b_).C;
    auto [S_mid, T_mid] = getStMid(a_, b_);

    float k = C_max / std::min(L * cusp.C / cusp.L, (1.0f - L) * cusp.C / (1.0f - cusp.L));

    float C_mid;
    {
        float C_a = L * S_mid;
        float C_b = (1.0f - L) * T_mid;
        C_mid = 0.9f * k * std::sqrt(std::sqrt(1.0f / (1.0f / (C_a * C_a * C_a * C_a) + 1.0f / (C_b * C_b * C_b * C_b))));
    }

    float C_0;
    {
        float C_a = L * 0.4f;
        float C_b = (1.0f - L) * 0.8f;
        C_0 = std::sqrt(1.0f / (1.0f / (C_a * C_a) + 1.0f / (C_b * C_b)));
    }

    return { C_mid, C_0 };
}

// -- OKLab <-> OKHsl --

OKHsl oklabToOkhsl(const OKLab& lab) {
    float C = std::sqrt(lab.a * lab.a + lab.b * lab.b);
    float h = 0.5f + 0.5f * std::atan2(-lab.b, -lab.a) / 3.14159265358979323846f;

    float a_ = (C > 0.0001f) ? lab.a / C : 1.0f;
    float b_ = (C > 0.0001f) ? lab.b / C : 0.0f;

    float L = toeInv(lab.L);
    auto [C_mid, C_0] = getCs(L, a_, b_);
    LC cusp = findCusp(a_, b_);
    float C_max = cusp.C;

    float s;
    if (C < C_0) {
        s = (C_0 > 0.0001f) ? 0.5f * C / C_0 : 0.0f;
    } else {
        s = (C_max - C_0 > 0.0001f) ? 0.5f + 0.5f * (C - C_0) / (C_max - C_0) : 0.5f;
    }

    float l = toe(lab.L);

    return { h * 360.0f, std::clamp(s, 0.0f, 1.0f), std::clamp(l, 0.0f, 1.0f) };
}

OKLab okhslToOklab(const OKHsl& hsl) {
    if (hsl.l >= 1.0f) return { 1.0f, 0.0f, 0.0f };
    if (hsl.l <= 0.0f) return { 0.0f, 0.0f, 0.0f };

    float h = hsl.h / 360.0f;
    float a_ = std::cos(2.0f * 3.14159265358979323846f * h);
    float b_ = std::sin(2.0f * 3.14159265358979323846f * h);

    float L = toeInv(hsl.l);

    auto [C_mid, C_0] = getCs(L, a_, b_);
    LC cusp = findCusp(a_, b_);
    float C_max = cusp.C;

    float C;
    float s = hsl.s;
    if (s < 0.5f) {
        C = C_0 * 2.0f * s;
    } else {
        C = C_0 + (C_max - C_0) * 2.0f * (s - 0.5f);
    }

    float Lfinal = toeInv(hsl.l);
    return { hsl.l > 0.0f ? toeInv(hsl.l) : 0.0f, C * a_, C * b_ };
}

// -- Convenience functions --

OKHsl rgbToOkhsl(const RGB& c) {
    return oklabToOkhsl(linearToOklab(srgbToLinear(c)));
}

RGB okhslToRgb(const OKHsl& c) {
    return linearToSrgb(oklabToLinear(okhslToOklab(c)));
}

OKLab rgbToOklab(const RGB& c) {
    return linearToOklab(srgbToLinear(c));
}

RGB oklabToRgb(const OKLab& c) {
    return linearToSrgb(oklabToLinear(c));
}

// -- Interpolation --

OKLab lerpOklab(const OKLab& a, const OKLab& b, float t) {
    return {
        a.L + (b.L - a.L) * t,
        a.a + (b.a - a.a) * t,
        a.b + (b.b - a.b) * t
    };
}

OKHsl lerpOkhsl(const OKHsl& a, const OKHsl& b, float t) {
    return {
        lerpHue(a.h, b.h, t),
        a.s + (b.s - a.s) * t,
        a.l + (b.l - a.l) * t
    };
}

// -- Hue utilities --

float wrapHue(float h) {
    h = std::fmod(h, 360.0f);
    if (h < 0.0f) h += 360.0f;
    return h;
}

float hueDistance(float h1, float h2) {
    float d = std::fmod(h2 - h1 + 540.0f, 360.0f) - 180.0f;
    return d;
}

float lerpHue(float h1, float h2, float t) {
    float d = hueDistance(h1, h2);
    return wrapHue(h1 + d * t);
}

// -- Gamut clipping --

RGB clampRgb(const RGB& c) {
    return {
        std::clamp(c.r, 0.0f, 1.0f),
        std::clamp(c.g, 0.0f, 1.0f),
        std::clamp(c.b, 0.0f, 1.0f),
        std::clamp(c.a, 0.0f, 1.0f)
    };
}

OKLab gamutClipOklab(const OKLab& c) {
    RGB rgb = oklabToRgb(c);
    return rgbToOklab(clampRgb(rgb));
}

// -- Shader uniform export --

std::array<float, 20> paletteToShaderUniform(const std::array<OKHsl, 5>& palette) {
    std::array<float, 20> out{};
    for (int i = 0; i < 5; ++i) {
        RGB rgb = clampRgb(okhslToRgb(palette[i]));
        out[i * 4 + 0] = rgb.r;
        out[i * 4 + 1] = rgb.g;
        out[i * 4 + 2] = rgb.b;
        out[i * 4 + 3] = rgb.a;
    }
    return out;
}

} // namespace dvds
