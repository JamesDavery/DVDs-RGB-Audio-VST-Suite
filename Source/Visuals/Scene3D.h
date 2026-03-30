#pragma once
#include <string>
#include <vector>
#include <array>
#include <memory>

namespace dvds {

struct Vertex3D {
    float position[3];
    float normal[3];
    float texCoord[2];
};

struct Mesh {
    std::vector<Vertex3D> vertices;
    std::vector<unsigned int> indices;
    unsigned int vao = 0, vbo = 0, ebo = 0;
    std::string name;
};

struct Camera3D {
    float position[3] = { 0.0f, 0.0f, 5.0f };
    float target[3] = { 0.0f, 0.0f, 0.0f };
    float up[3] = { 0.0f, 1.0f, 0.0f };
    float fov = 45.0f;
    float nearPlane = 0.1f;
    float farPlane = 100.0f;
    float orbitAngle = 0.0f;
    float orbitDistance = 5.0f;
    float orbitHeight = 2.0f;
};

class Scene3D {
public:
    Scene3D();
    ~Scene3D();

    void initialize(int width, int height);
    void resize(int width, int height);

    bool loadModel(const std::string& path);
    void clearScene();

    void setPalette(const std::array<float, 20>& palette);
    void setAudioReactive(float bassZoom, float midRotate, float treblePulse);
    void setCameraOrbit(float angle, float distance, float height);

    void render(float time);

    unsigned int getOutputTexture() const { return outputTexture_; }
    Camera3D& getCamera() { return camera_; }

private:
    int width_ = 1920, height_ = 1080;
    std::vector<Mesh> meshes_;
    Camera3D camera_;
    std::array<float, 20> palette_{};
    unsigned int fbo_ = 0, outputTexture_ = 0, depthBuffer_ = 0;
    float bassZoom_ = 0.0f, midRotate_ = 0.0f, treblePulse_ = 0.0f;
    // ShaderProgram for PBR-lite rendering
};

} // namespace dvds
