#pragma once
#include "Visuals/ShaderPipeline.h"
#include <string>

namespace dvds {

class GenerativeScene {
public:
    GenerativeScene();
    ~GenerativeScene();

    void initialize(int width, int height);
    void resize(int width, int height);
    void selectScene(int index);
    void setPalette(const std::array<float, 20>& palette);
    void setAudioData(float rms, float bpm, float beat, const float* fft, int fftSize);
    void render(float time);

    int getSceneCount() const;
    std::string getSceneName(int index) const;
    unsigned int getOutputTexture() const;

private:
    ShaderPipeline pipeline_;
    int currentScene_ = 0;
};

} // namespace dvds
