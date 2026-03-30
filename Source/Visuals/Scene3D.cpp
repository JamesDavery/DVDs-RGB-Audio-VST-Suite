#include "Visuals/Scene3D.h"

namespace dvds {

Scene3D::Scene3D() { palette_.fill(0.0f); }
Scene3D::~Scene3D() = default;

void Scene3D::initialize(int w, int h) { width_ = w; height_ = h; }
void Scene3D::resize(int w, int h) { width_ = w; height_ = h; }

bool Scene3D::loadModel(const std::string& path) {
    // tinygltf / OBJ loading would go here
    // Parse vertices, normals, texcoords; create VAO/VBO/EBO
    return true;
}

void Scene3D::clearScene() {
    meshes_.clear();
}

void Scene3D::setPalette(const std::array<float, 20>& p) { palette_ = p; }

void Scene3D::setAudioReactive(float bass, float mid, float treble) {
    bassZoom_ = bass;
    midRotate_ = mid;
    treblePulse_ = treble;
}

void Scene3D::setCameraOrbit(float angle, float distance, float height) {
    camera_.orbitAngle = angle;
    camera_.orbitDistance = distance;
    camera_.orbitHeight = height;
}

void Scene3D::render(float time) {
    // Apply audio reactivity to camera
    float zoom = camera_.orbitDistance * (1.0f - bassZoom_ * 0.3f);
    float angle = camera_.orbitAngle + midRotate_ * time * 30.0f;

    // Update camera position from orbit params
    camera_.position[0] = zoom * std::cos(angle * 3.14159f / 180.0f);
    camera_.position[1] = camera_.orbitHeight;
    camera_.position[2] = zoom * std::sin(angle * 3.14159f / 180.0f);

    // Render: bind FBO, clear, set up projection/view matrices,
    // draw each mesh with PBR-lite shader using palette colors
    // for diffuse/emissive materials
}

} // namespace dvds
