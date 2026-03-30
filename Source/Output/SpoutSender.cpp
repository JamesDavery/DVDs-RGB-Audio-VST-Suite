#include "Output/SpoutSender.h"

namespace dvds {

SpoutSender::SpoutSender() = default;
SpoutSender::~SpoutSender() { shutdown(); }

bool SpoutSender::initialize(const std::string& name, int w, int h) {
#ifdef _WIN32
    name_ = name; width_ = w; height_ = h;
    // SpoutSender::CreateSender(name.c_str(), w, h);
    initialized_ = true;
    return true;
#else
    return false;
#endif
}

void SpoutSender::shutdown() {
#ifdef _WIN32
    if (initialized_) {
        // SpoutSender::ReleaseSender();
        initialized_ = false;
    }
#endif
}

void SpoutSender::sendTexture(unsigned int textureId, int w, int h) {
#ifdef _WIN32
    if (!initialized_) return;
    // SpoutSender::SendTexture(textureId, GL_TEXTURE_2D, w, h);
#endif
}

void SpoutSender::resize(int w, int h) {
    width_ = w; height_ = h;
    // Update Spout sender dimensions
}

} // namespace dvds
