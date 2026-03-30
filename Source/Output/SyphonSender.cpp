#include "Output/SyphonSender.h"

namespace dvds {

SyphonSender::SyphonSender() = default;
SyphonSender::~SyphonSender() { shutdown(); }

bool SyphonSender::initialize(const std::string& name, int w, int h) {
#ifdef __APPLE__
    name_ = name; width_ = w; height_ = h;
    // [[SyphonServer alloc] initWithName:...];
    initialized_ = true;
    return true;
#else
    return false;
#endif
}

void SyphonSender::shutdown() {
#ifdef __APPLE__
    if (initialized_) {
        // [syphonServer stop];
        initialized_ = false;
    }
#endif
}

void SyphonSender::sendTexture(unsigned int textureId, int w, int h) {
#ifdef __APPLE__
    if (!initialized_) return;
    // [syphonServer publishFrameTexture:...];
#endif
}

void SyphonSender::resize(int w, int h) { width_ = w; height_ = h; }

} // namespace dvds
