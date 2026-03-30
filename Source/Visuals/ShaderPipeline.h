#pragma once
#include "Core/OKLab.h"
#include <string>
#include <unordered_map>
#include <array>
#include <vector>
#include <memory>
#include <functional>

namespace dvds {

struct ShaderUniformInfo {
    enum class Type { Float, Vec2, Vec3, Vec4, Int, Bool, Sampler2D, FloatArray };
    std::string name;
    Type type;
    int location = -1;
    int arraySize = 1;
};

class ShaderProgram {
public:
    ShaderProgram();
    ~ShaderProgram();

    bool compileFromSource(const std::string& vertSrc, const std::string& fragSrc);
    bool compileFragment(const std::string& fragSrc);
    void bind();
    void unbind();
    bool isValid() const { return programId_ != 0; }

    void setFloat(const std::string& name, float v);
    void setVec2(const std::string& name, float x, float y);
    void setVec3(const std::string& name, float x, float y, float z);
    void setVec4(const std::string& name, float x, float y, float z, float w);
    void setInt(const std::string& name, int v);
    void setBool(const std::string& name, bool v);
    void setFloatArray(const std::string& name, const float* data, int count);
    void setMat4(const std::string& name, const float* data);

    int getUniformLocation(const std::string& name);

    unsigned int getProgramId() const { return programId_; }
    const std::string& getLastError() const { return lastError_; }

private:
    unsigned int programId_ = 0;
    unsigned int vertId_ = 0;
    unsigned int fragId_ = 0;
    std::string lastError_;
    std::unordered_map<std::string, int> uniformCache_;

    static const char* kDefaultVertexShader;
    unsigned int compileShader(unsigned int type, const std::string& src);
};

struct ShaderInfo {
    std::string name;
    std::string category;
    std::string fragmentSource;
    std::string description;
};

class ShaderPipeline {
public:
    ShaderPipeline();
    ~ShaderPipeline();

    void initialize(int width, int height);
    void resize(int width, int height);

    void setActiveShader(int index);
    int getActiveShaderIndex() const { return activeShaderIndex_; }
    const ShaderInfo* getActiveShaderInfo() const;

    void setPalette(const std::array<float, 20>& palette);
    void setTime(float t);
    void setResolution(float w, float h);
    void setFFTData(const float* data, int size);
    void setRMS(float rms);
    void setBPM(float bpm);
    void setBeat(float beat);
    void setBeatPhase(float phase);

    void render();

    int getShaderCount() const { return static_cast<int>(shaders_.size()); }
    const ShaderInfo& getShaderInfo(int index) const { return shaders_[index]; }

    unsigned int getOutputTexture() const { return outputTexture_; }

    void addShader(const ShaderInfo& info);
    void loadBuiltInShaders();

private:
    int width_ = 1920, height_ = 1080;
    int activeShaderIndex_ = 0;
    std::vector<ShaderInfo> shaders_;
    std::unique_ptr<ShaderProgram> activeProgram_;
    unsigned int fbo_ = 0;
    unsigned int outputTexture_ = 0;
    float time_ = 0.0f;
    float resolution_[2] = { 1920.0f, 1080.0f };
    std::array<float, 20> palette_{};
    std::vector<float> fftData_;
    float rms_ = 0.0f, bpm_ = 120.0f, beat_ = 0.0f, beatPhase_ = 0.0f;

    unsigned int quadVAO_ = 0, quadVBO_ = 0;
    void createQuad();
};

} // namespace dvds
