#include "Visuals/GenerativeScene.h"

namespace dvds {

GenerativeScene::GenerativeScene() = default;
GenerativeScene::~GenerativeScene() = default;

void GenerativeScene::initialize(int w, int h) { pipeline_.initialize(w, h); }
void GenerativeScene::resize(int w, int h) { pipeline_.resize(w, h); }

void GenerativeScene::selectScene(int index) {
    pipeline_.setActiveShader(index);
    currentScene_ = index;
}

void GenerativeScene::setPalette(const std::array<float, 20>& p) { pipeline_.setPalette(p); }

void GenerativeScene::setAudioData(float rms, float bpm, float beat, const float* fft, int fftSize) {
    pipeline_.setRMS(rms);
    pipeline_.setBPM(bpm);
    pipeline_.setBeat(beat);
    if (fft && fftSize > 0) pipeline_.setFFTData(fft, fftSize);
}

void GenerativeScene::render(float time) {
    pipeline_.setTime(time);
    pipeline_.render();
}

int GenerativeScene::getSceneCount() const { return pipeline_.getShaderCount(); }
std::string GenerativeScene::getSceneName(int idx) const {
    if (idx >= 0 && idx < pipeline_.getShaderCount())
        return pipeline_.getShaderInfo(idx).name;
    return "";
}
unsigned int GenerativeScene::getOutputTexture() const { return pipeline_.getOutputTexture(); }

} // namespace dvds
