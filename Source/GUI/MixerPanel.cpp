#include "GUI/MixerPanel.h"

namespace dvds {

MixerPanel::MixerPanel() = default;
MixerPanel::~MixerPanel() = default;

void MixerPanel::setVideoMixer(VideoMixer* m) { mixer_ = m; }

void MixerPanel::paint(/* Graphics& g */) {
    if (!mixer_) return;
    for (int i = 0; i < mixer_->getLayerCount(); ++i) {
        auto* layer = mixer_->getLayer(i);
        if (!layer) continue;
        // Draw layer strip: visibility toggle, name, blend mode dropdown,
        // opacity slider, color grade toggle
    }
    // Draw "Add Layer" button at bottom
}

void MixerPanel::resized(int w, int h) { width_ = w; height_ = h; }

void MixerPanel::mouseDown(float x, float y) {
    // Hit test layer strips, buttons, sliders
}

void MixerPanel::mouseDrag(float x, float y) {
    // Update opacity slider if active
}

} // namespace dvds
