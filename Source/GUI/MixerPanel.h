#pragma once
#include "Visuals/VideoMixer.h"
#include <functional>

namespace dvds {

class MixerPanel {
public:
    MixerPanel();
    ~MixerPanel();

    void setVideoMixer(VideoMixer* mixer);
    void paint(/* Graphics& g */);
    void resized(int width, int height);
    void mouseDown(float x, float y);
    void mouseDrag(float x, float y);

    std::function<void(int, BlendMode)> onBlendModeChanged;
    std::function<void(int, float)> onOpacityChanged;
    std::function<void()> onAddLayer;
    std::function<void(int)> onRemoveLayer;

private:
    VideoMixer* mixer_ = nullptr;
    int width_ = 300, height_ = 400;
    int selectedLayer_ = 0;
    int activeSlider_ = -1;
};

} // namespace dvds
