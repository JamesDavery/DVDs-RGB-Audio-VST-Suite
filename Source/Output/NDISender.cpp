#include "Output/NDISender.h"

namespace dvds {

NDISender::NDISender() = default;
NDISender::~NDISender() { shutdown(); }

bool NDISender::initialize(const std::string& name, int w, int h) {
    name_ = name; width_ = w; height_ = h;
    // NDIlib_send_create(), etc.
    initialized_ = true;
    return true;
}

void NDISender::shutdown() {
    if (initialized_) {
        // NDIlib_send_destroy();
        initialized_ = false;
    }
}

void NDISender::sendFrame(const unsigned char* rgbaData, int w, int h) {
    if (!initialized_) return;
    // NDIlib_send_send_video_v2();
}

void NDISender::sendTexture(unsigned int textureId, int w, int h) {
    if (!initialized_) return;
    // Read pixels from texture, send via NDI
}

void NDISender::resize(int w, int h) { width_ = w; height_ = h; }

} // namespace dvds
