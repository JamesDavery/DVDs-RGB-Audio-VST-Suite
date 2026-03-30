#include "Visuals/VideoMixer.h"

namespace dvds {

std::string blendModeToString(BlendMode mode) {
    switch (mode) {
        case BlendMode::Normal:     return "Normal";
        case BlendMode::Add:        return "Add";
        case BlendMode::Multiply:   return "Multiply";
        case BlendMode::Screen:     return "Screen";
        case BlendMode::Overlay:    return "Overlay";
        case BlendMode::Difference: return "Difference";
        case BlendMode::Exclusion:  return "Exclusion";
        case BlendMode::SoftLight:  return "Soft Light";
        case BlendMode::HardLight:  return "Hard Light";
        default: return "Normal";
    }
}

VideoMixer::VideoMixer() { palette_.fill(0.0f); }
VideoMixer::~VideoMixer() = default;

void VideoMixer::initialize(int w, int h) { width_ = w; height_ = h; }
void VideoMixer::resize(int w, int h) { width_ = w; height_ = h; }

int VideoMixer::addLayer() {
    layers_.push_back(std::make_unique<VideoLayer>());
    layers_.back()->player = std::make_unique<VideoPlayer>();
    return static_cast<int>(layers_.size()) - 1;
}

void VideoMixer::removeLayer(int idx) {
    if (idx >= 0 && idx < static_cast<int>(layers_.size()))
        layers_.erase(layers_.begin() + idx);
}

VideoLayer* VideoMixer::getLayer(int idx) {
    if (idx >= 0 && idx < static_cast<int>(layers_.size()))
        return layers_[idx].get();
    return nullptr;
}

void VideoMixer::setLayerBlendMode(int idx, BlendMode mode) {
    if (auto* l = getLayer(idx)) l->blendMode = mode;
}

void VideoMixer::setLayerOpacity(int idx, float opacity) {
    if (auto* l = getLayer(idx)) l->opacity = opacity;
}

void VideoMixer::setLayerVisible(int idx, bool visible) {
    if (auto* l = getLayer(idx)) l->visible = visible;
}

void VideoMixer::setPalette(const std::array<float, 20>& p) { palette_ = p; }

void VideoMixer::render(float time) {
    // For each visible layer: render video frame, apply palette color grade, composite with blend mode
    for (auto& layer : layers_) {
        if (!layer->visible || !layer->player->isOpen()) continue;
        layer->player->advanceFrame(1.0 / 60.0);
        // Bind layer texture, blend onto output FBO
    }
}

} // namespace dvds
