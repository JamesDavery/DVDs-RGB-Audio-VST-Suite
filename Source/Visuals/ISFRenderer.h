#pragma once
#include "Visuals/ISFLoader.h"
#include "Visuals/ShaderPipeline.h"
#include <vector>
#include <unordered_map>
#include <memory>

namespace dvds {

class ISFRenderer {
public:
    ISFRenderer();
    ~ISFRenderer();

    bool load(const ISFFile& isf);
    void initialize(int width, int height);
    void resize(int width, int height);

    void setInputFloat(const std::string& name, float val);
    void setInputBool(const std::string& name, bool val);
    void setInputColor(const std::string& name, float r, float g, float b, float a);
    void setInputPoint2D(const std::string& name, float x, float y);
    void setInputImage(const std::string& name, unsigned int textureId);
    void setAudioTexture(const std::string& name, unsigned int textureId);

    void setPalette(const std::array<float, 20>& palette);
    void setTime(float t);
    void setTimeDelta(float dt);
    void setFrameIndex(int idx);
    void setRMS(float rms);
    void setBPM(float bpm);

    void render();

    unsigned int getOutputTexture() const { return outputTexture_; }
    bool isLoaded() const { return loaded_; }
    const ISFMetadata& getMetadata() const { return metadata_; }

private:
    bool loaded_ = false;
    ISFMetadata metadata_;
    int width_ = 1920, height_ = 1080;
    float time_ = 0.0f, timeDelta_ = 0.0f;
    int frameIndex_ = 0;

    std::vector<std::unique_ptr<ShaderProgram>> passPrograms_;
    std::vector<unsigned int> passFBOs_;
    std::vector<unsigned int> passTextures_;
    std::unordered_map<std::string, unsigned int> persistentBuffers_;
    unsigned int outputTexture_ = 0;

    std::unordered_map<std::string, float> floatInputs_;
    std::unordered_map<std::string, bool> boolInputs_;
    std::unordered_map<std::string, std::array<float, 4>> colorInputs_;
    std::array<float, 20> palette_{};
    float rms_ = 0.0f, bpm_ = 120.0f;

    void createPassResources();
    void setCommonUniforms(ShaderProgram& prog, int passIndex);
};

} // namespace dvds
