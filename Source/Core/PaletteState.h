#pragma once
#include "Core/OKLab.h"
#include "Core/ColorTheoryEngine.h"
#include <array>
#include <mutex>

namespace dvds {

class PaletteState {
public:
    PaletteState();

    void setTarget(const std::array<OKHsl, 5>& target);
    void setTransitionTime(float seconds);
    void update(float deltaTime);
    void snapToTarget();

    const std::array<OKHsl, 5>& getCurrent() const { return current_; }
    const std::array<OKHsl, 5>& getTarget() const { return target_; }
    std::array<RGB, 5> getCurrentRGB() const;
    std::array<float, 20> getShaderUniform() const;

    bool isTransitioning() const { return transitioning_; }
    float getTransitionProgress() const { return progress_; }

private:
    std::array<OKHsl, 5> current_;
    std::array<OKHsl, 5> target_;
    std::array<OKHsl, 5> origin_;
    float transitionTime_ = 0.3f;
    float progress_ = 1.0f;
    bool transitioning_ = false;
    mutable std::mutex mutex_;
};

} // namespace dvds
