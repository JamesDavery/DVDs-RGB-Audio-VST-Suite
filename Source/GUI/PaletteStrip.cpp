#include "GUI/PaletteStrip.h"
#include <sstream>
#include <iomanip>

namespace dvds {

PaletteStrip::PaletteStrip() {
    OKHsl def = { 0.0f, 0.75f, 0.65f };
    palette_.fill(def);
    for (int i = 0; i < 5; ++i) paletteRGB_[i] = okhslToRgb(def);
}

PaletteStrip::~PaletteStrip() = default;

void PaletteStrip::setPalette(const std::array<OKHsl, 5>& p) {
    palette_ = p;
    for (int i = 0; i < 5; ++i) paletteRGB_[i] = clampRgb(okhslToRgb(p[i]));
}

void PaletteStrip::setPaletteRGB(const std::array<RGB, 5>& p) {
    paletteRGB_ = p;
    for (int i = 0; i < 5; ++i) palette_[i] = rgbToOkhsl(p[i]);
}

void PaletteStrip::paint(/* Graphics& g */) {
    float x = 0.0f;
    for (int i = 0; i < 5; ++i) {
        // Draw filled rectangle at (x, 0, swatchWidth_, height_) with paletteRGB_[i]
        // If selected, draw border
        x += swatchWidth_ + swatchGap_;
    }
}

void PaletteStrip::resized(int w, int h) {
    width_ = w; height_ = h;
    swatchGap_ = 4.0f;
    swatchWidth_ = (w - swatchGap_ * 4.0f) / 5.0f;
}

void PaletteStrip::mouseDown(float x, float y) {
    int idx = static_cast<int>(x / (swatchWidth_ + swatchGap_));
    if (idx >= 0 && idx < 5) {
        selectedIndex_ = idx;
        if (onSwatchSelected) onSwatchSelected(idx);
    }
}

std::string PaletteStrip::getHexString(int idx) const {
    if (idx < 0 || idx >= 5) return "";
    uint32_t hex = paletteRGB_[idx].toHex();
    std::stringstream ss;
    ss << "#" << std::uppercase << std::hex << std::setfill('0') << std::setw(6) << hex;
    return ss.str();
}

std::string PaletteStrip::getRGBString(int idx) const {
    if (idx < 0 || idx >= 5) return "";
    auto& c = paletteRGB_[idx];
    return "rgb(" + std::to_string(int(c.r * 255)) + ", "
                  + std::to_string(int(c.g * 255)) + ", "
                  + std::to_string(int(c.b * 255)) + ")";
}

std::string PaletteStrip::getOKHslString(int idx) const {
    if (idx < 0 || idx >= 5) return "";
    auto& c = palette_[idx];
    std::stringstream ss;
    ss << std::fixed << std::setprecision(1);
    ss << "okhsl(" << c.h << ", " << c.s << ", " << c.l << ")";
    return ss.str();
}

} // namespace dvds
