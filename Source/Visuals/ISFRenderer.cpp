#include "Visuals/ISFRenderer.h"

namespace dvds {

ISFRenderer::ISFRenderer() { palette_.fill(0.0f); }
ISFRenderer::~ISFRenderer() = default;

bool ISFRenderer::load(const ISFFile& isf) {
    if (!isf.metadata.valid) return false;
    metadata_ = isf.metadata;

    passPrograms_.clear();

    if (metadata_.passes.empty()) {
        auto prog = std::make_unique<ShaderProgram>();
        prog->compileFragment(isf.fragmentShader);
        passPrograms_.push_back(std::move(prog));
    } else {
        for (size_t i = 0; i < metadata_.passes.size(); ++i) {
            auto prog = std::make_unique<ShaderProgram>();
            prog->compileFragment(isf.fragmentShader);
            passPrograms_.push_back(std::move(prog));
        }
    }

    for (const auto& inp : metadata_.inputs) {
        if (inp.type == ISFInput::Type::Float || inp.type == ISFInput::Type::Event)
            floatInputs_[inp.name] = inp.defaultVal;
        else if (inp.type == ISFInput::Type::Bool)
            boolInputs_[inp.name] = inp.defaultVal > 0.5f;
        else if (inp.type == ISFInput::Type::Color)
            colorInputs_[inp.name] = { inp.defaultColor.size() > 0 ? inp.defaultColor[0] : 0.0f,
                                        inp.defaultColor.size() > 1 ? inp.defaultColor[1] : 0.0f,
                                        inp.defaultColor.size() > 2 ? inp.defaultColor[2] : 0.0f,
                                        inp.defaultColor.size() > 3 ? inp.defaultColor[3] : 1.0f };
    }

    loaded_ = true;
    return true;
}

void ISFRenderer::initialize(int w, int h) {
    width_ = w; height_ = h;
    createPassResources();
}

void ISFRenderer::resize(int w, int h) {
    width_ = w; height_ = h;
    createPassResources();
}

void ISFRenderer::setInputFloat(const std::string& name, float val) { floatInputs_[name] = val; }
void ISFRenderer::setInputBool(const std::string& name, bool val) { boolInputs_[name] = val; }
void ISFRenderer::setInputColor(const std::string& name, float r, float g, float b, float a) {
    colorInputs_[name] = { r, g, b, a };
}
void ISFRenderer::setInputPoint2D(const std::string& name, float x, float y) {}
void ISFRenderer::setInputImage(const std::string& name, unsigned int textureId) {}
void ISFRenderer::setAudioTexture(const std::string& name, unsigned int textureId) {}

void ISFRenderer::setPalette(const std::array<float, 20>& p) { palette_ = p; }
void ISFRenderer::setTime(float t) { time_ = t; }
void ISFRenderer::setTimeDelta(float dt) { timeDelta_ = dt; }
void ISFRenderer::setFrameIndex(int idx) { frameIndex_ = idx; }
void ISFRenderer::setRMS(float rms) { rms_ = rms; }
void ISFRenderer::setBPM(float bpm) { bpm_ = bpm; }

void ISFRenderer::render() {
    if (!loaded_) return;
    for (size_t i = 0; i < passPrograms_.size(); ++i) {
        auto& prog = passPrograms_[i];
        if (!prog || !prog->isValid()) continue;
        prog->bind();
        setCommonUniforms(*prog, static_cast<int>(i));
        // Draw fullscreen quad per pass
        // glBindVertexArray(quadVAO); glDrawArrays(GL_TRIANGLES, 0, 6);
        prog->unbind();
    }
}

void ISFRenderer::createPassResources() {
    // Create FBOs and textures for each pass
    // Persistent buffers maintain state between frames
}

void ISFRenderer::setCommonUniforms(ShaderProgram& prog, int passIndex) {
    prog.setInt("PASSINDEX", passIndex);
    prog.setVec2("RENDERSIZE", static_cast<float>(width_), static_cast<float>(height_));
    prog.setFloat("TIME", time_);
    prog.setFloat("TIMEDELTA", timeDelta_);
    prog.setInt("FRAMEINDEX", frameIndex_);
    prog.setFloat("uRMS", rms_);
    prog.setFloat("uBPM", bpm_);

    for (int i = 0; i < 5; ++i) {
        prog.setVec4("uPalette[" + std::to_string(i) + "]",
            palette_[i*4], palette_[i*4+1], palette_[i*4+2], palette_[i*4+3]);
    }

    for (auto& [name, val] : floatInputs_) prog.setFloat(name, val);
    for (auto& [name, val] : boolInputs_) prog.setBool(name, val);
    for (auto& [name, val] : colorInputs_) prog.setVec4(name, val[0], val[1], val[2], val[3]);
}

} // namespace dvds
