#include "GUI/ColorWheelComponent.h"
#include <cmath>

namespace dvds {

ColorWheelComponent::ColorWheelComponent() {
    updateHandlesFromConfig();
}

ColorWheelComponent::~ColorWheelComponent() = default;

void ColorWheelComponent::setHarmonyConfig(const HarmonyConfig& config) {
    config_ = config;
    updateHandlesFromConfig();
}

void ColorWheelComponent::setBaseHue(float hue) {
    config_.baseHue = wrapHue(hue);
    updateHandlesFromConfig();
    if (onBaseHueChanged) onBaseHueChanged(config_.baseHue);
    if (onConfigChanged) onConfigChanged(config_);
}

void ColorWheelComponent::setHarmonyMode(HarmonyMode mode) {
    config_.mode = mode;
    updateHandlesFromConfig();
    if (onConfigChanged) onConfigChanged(config_);
}

void ColorWheelComponent::paint(/* Graphics& g */) {
    // Draw color wheel ring using OKHsl -> RGB for each pixel angle
    // For each angle 0-360:
    //   RGB color = okhslToRgb({ angle, config_.baseSaturation, config_.baseLightness })
    //   Draw arc segment with that color

    // Draw 5 handles at their positions
    // Handle colors from current palette
    ColorTheoryEngine engine;
    engine.setConfig(config_);
    auto palette = engine.generatePaletteRGB();

    for (int i = 0; i < 5; ++i) {
        float angle = handles_[i].angle * 3.14159f / 180.0f;
        float r = handles_[i].radius;
        float hx = centerX_ + r * std::cos(angle);
        float hy = centerY_ - r * std::sin(angle);
        // Draw circle at (hx, hy) with color palette[i]
        // If i == 0 (base), draw larger
    }
}

void ColorWheelComponent::mouseDown(float x, float y) {
    dragHandle_ = hitTestHandle(x, y);
    if (dragHandle_ < 0 && isInWheel(x, y)) {
        float angle = pointToAngle(x, y);
        setBaseHue(angle);
        dragHandle_ = 0;
    }
}

void ColorWheelComponent::mouseDrag(float x, float y) {
    if (dragHandle_ < 0) return;

    float angle = pointToAngle(x, y);

    if (config_.mode == HarmonyMode::Custom) {
        config_.customColors[dragHandle_].h = angle;
        updateHandlesFromConfig();
    } else if (dragHandle_ == 0) {
        setBaseHue(angle);
    }
}

void ColorWheelComponent::mouseUp(float x, float y) {
    dragHandle_ = -1;
}

void ColorWheelComponent::resized(int w, int h) {
    width_ = w; height_ = h;
    centerX_ = w * 0.5f;
    centerY_ = h * 0.5f;
    wheelRadius_ = std::min(w, h) * 0.4f;
    wheelInnerRadius_ = wheelRadius_ * 0.6f;
    updateHandlesFromConfig();
}

float ColorWheelComponent::pointToAngle(float x, float y) const {
    float dx = x - centerX_;
    float dy = -(y - centerY_);
    float angle = std::atan2(dy, dx) * 180.0f / 3.14159f;
    return wrapHue(angle);
}

float ColorWheelComponent::pointToRadius(float x, float y) const {
    float dx = x - centerX_;
    float dy = y - centerY_;
    return std::sqrt(dx * dx + dy * dy);
}

bool ColorWheelComponent::isInWheel(float x, float y) const {
    float r = pointToRadius(x, y);
    return r >= wheelInnerRadius_ && r <= wheelRadius_;
}

int ColorWheelComponent::hitTestHandle(float x, float y) const {
    float handleSize = 12.0f;
    for (int i = 0; i < 5; ++i) {
        float angle = handles_[i].angle * 3.14159f / 180.0f;
        float r = handles_[i].radius;
        float hx = centerX_ + r * std::cos(angle);
        float hy = centerY_ - r * std::sin(angle);
        float dx = x - hx, dy = y - hy;
        if (dx * dx + dy * dy < handleSize * handleSize) return i;
    }
    return -1;
}

void ColorWheelComponent::updateHandlesFromConfig() {
    ColorTheoryEngine engine;
    engine.setConfig(config_);
    auto palette = engine.generatePalette();

    float midRadius = (wheelRadius_ + wheelInnerRadius_) * 0.5f;
    for (int i = 0; i < 5; ++i) {
        handles_[i].angle = palette[i].h;
        handles_[i].radius = midRadius;
    }
}

} // namespace dvds
