#pragma once
#include "Core/OKLab.h"
#include <array>
#include <functional>
#include <string>

namespace dvds {

class PaletteStrip {
public:
    PaletteStrip();
    ~PaletteStrip();

    void setPalette(const std::array<OKHsl, 5>& palette);
    void setPaletteRGB(const std::array<RGB, 5>& palette);
    const std::array<OKHsl, 5>& getPalette() const { return palette_; }

    void paint(/* Graphics& g */);
    void resized(int width, int height);
    void mouseDown(float x, float y);

    int getSelectedIndex() const { return selectedIndex_; }
    std::string getHexString(int index) const;
    std::string getRGBString(int index) const;
    std::string getOKHslString(int index) const;

    std::function<void(int)> onSwatchSelected;
    std::function<void(int, const OKHsl&)> onSwatchEdited;

private:
    std::array<OKHsl, 5> palette_;
    std::array<RGB, 5> paletteRGB_;
    int width_ = 500, height_ = 60;
    int selectedIndex_ = 0;
    float swatchWidth_ = 100.0f;
    float swatchGap_ = 4.0f;
};

} // namespace dvds
