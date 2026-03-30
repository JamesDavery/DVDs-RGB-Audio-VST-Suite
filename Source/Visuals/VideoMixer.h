#pragma once
#include "Visuals/VideoPlayer.h"
#include "Visuals/ShaderPipeline.h"
#include <vector>
#include <memory>
#include <array>

namespace dvds {

enum class BlendMode {
    Normal = 0, Add, Multiply, Screen, Overlay, Difference, Exclusion, SoftLight, HardLight,
    Count
};

std::string blendModeToString(BlendMode mode);

struct VideoLayer {
    std::unique_ptr<VideoPlayer> player;
    BlendMode blendMode = BlendMode::Normal;
    float opacity = 1.0f;
    bool visible = true;
    bool colorGradeWithPalette = true;
};

class VideoMixer {
public:
    VideoMixer();
    ~VideoMixer();

    void initialize(int width, int height);
    void resize(int width, int height);

    int addLayer();
    void removeLayer(int index);
    VideoLayer* getLayer(int index);
    int getLayerCount() const { return static_cast<int>(layers_.size()); }

    void setLayerBlendMode(int index, BlendMode mode);
    void setLayerOpacity(int index, float opacity);
    void setLayerVisible(int index, bool visible);

    void setPalette(const std::array<float, 20>& palette);
    void render(float time);

    unsigned int getOutputTexture() const { return outputTexture_; }

private:
    std::vector<std::unique_ptr<VideoLayer>> layers_;
    int width_ = 1920, height_ = 1080;
    unsigned int outputFBO_ = 0;
    unsigned int outputTexture_ = 0;
    std::array<float, 20> palette_{};
    std::unique_ptr<ShaderProgram> blendProgram_;
    std::unique_ptr<ShaderProgram> colorGradeProgram_;
};

} // namespace dvds
