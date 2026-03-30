#pragma once
#include <string>

namespace dvds {

class SyphonSender {
public:
    SyphonSender();
    ~SyphonSender();

    bool initialize(const std::string& name, int width, int height);
    void shutdown();
    bool isInitialized() const { return initialized_; }

    void sendTexture(unsigned int textureId, int width, int height);
    void resize(int width, int height);

    const std::string& getName() const { return name_; }

private:
    bool initialized_ = false;
    std::string name_ = "DVDs-RGB Output";
    int width_ = 1920, height_ = 1080;
    // Syphon server handle (Objective-C wrapped)
};

} // namespace dvds
