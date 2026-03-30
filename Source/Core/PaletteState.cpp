#include "Core/PaletteState.h"
#include <cmath>
#include <algorithm>

namespace dvds {

PaletteState::PaletteState() {
    OKHsl defaultColor = { 0.0f, 0.75f, 0.65f };
    current_.fill(defaultColor);
    target_.fill(defaultColor);
    origin_.fill(defaultColor);
}

void PaletteState::setTarget(const std::array<OKHsl, 5>& target) {
    std::lock_guard<std::mutex> lock(mutex_);
    origin_ = current_;
    target_ = target;
    progress_ = 0.0f;
    transitioning_ = true;
}

void PaletteState::setTransitionTime(float seconds) {
    transitionTime_ = std::max(0.001f, seconds);
}

void PaletteState::update(float deltaTime) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!transitioning_) return;

    progress_ += deltaTime / transitionTime_;
    if (progress_ >= 1.0f) {
        progress_ = 1.0f;
        transitioning_ = false;
        current_ = target_;
        return;
    }

    float t = progress_ * progress_ * (3.0f - 2.0f * progress_); // smoothstep

    for (int i = 0; i < 5; ++i) {
        current_[i] = lerpOkhsl(origin_[i], target_[i], t);
    }
}

void PaletteState::snapToTarget() {
    std::lock_guard<std::mutex> lock(mutex_);
    current_ = target_;
    progress_ = 1.0f;
    transitioning_ = false;
}

std::array<RGB, 5> PaletteState::getCurrentRGB() const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::array<RGB, 5> out;
    for (int i = 0; i < 5; ++i)
        out[i] = clampRgb(okhslToRgb(current_[i]));
    return out;
}

std::array<float, 20> PaletteState::getShaderUniform() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return paletteToShaderUniform(current_);
}

} // namespace dvds
