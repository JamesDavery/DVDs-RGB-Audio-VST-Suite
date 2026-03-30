#pragma once
#include "Core/OKLab.h"
#include "Core/ColorTheoryEngine.h"
#include <functional>
#include <array>

namespace dvds {

class ColorWheelComponent {
public:
    ColorWheelComponent();
    ~ColorWheelComponent();

    void setHarmonyConfig(const HarmonyConfig& config);
    const HarmonyConfig& getConfig() const { return config_; }

    void setBaseHue(float hue);
    void setHarmonyMode(HarmonyMode mode);

    void paint(/* Graphics context placeholder */);
    void mouseDown(float x, float y);
    void mouseDrag(float x, float y);
    void mouseUp(float x, float y);
    void resized(int width, int height);

    std::function<void(const HarmonyConfig&)> onConfigChanged;
    std::function<void(float)> onBaseHueChanged;

    int getWidth() const { return width_; }
    int getHeight() const { return height_; }

private:
    HarmonyConfig config_;
    int width_ = 300, height_ = 300;
    float centerX_ = 150.0f, centerY_ = 150.0f;
    float wheelRadius_ = 120.0f;
    float wheelInnerRadius_ = 70.0f;
    int dragHandle_ = -1;

    struct HandlePosition {
        float angle;
        float radius;
    };
    std::array<HandlePosition, 5> handles_;

    float pointToAngle(float x, float y) const;
    float pointToRadius(float x, float y) const;
    bool isInWheel(float x, float y) const;
    int hitTestHandle(float x, float y) const;
    void updateHandlesFromConfig();
};

} // namespace dvds
