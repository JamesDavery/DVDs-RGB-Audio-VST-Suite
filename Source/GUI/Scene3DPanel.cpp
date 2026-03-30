#include "GUI/Scene3DPanel.h"
#include <cmath>

namespace dvds {

Scene3DPanel::Scene3DPanel() = default;
Scene3DPanel::~Scene3DPanel() = default;

void Scene3DPanel::setScene(Scene3D* s) { scene_ = s; }

void Scene3DPanel::paint(/* Graphics& g */) {
    if (!scene_) return;
    // Display the 3D scene output texture
    // Draw camera control overlay (orbit, zoom indicators)
    // Draw "Load Model" button
}

void Scene3DPanel::resized(int w, int h) { width_ = w; height_ = h; }

void Scene3DPanel::mouseDown(float x, float y) {
    lastMouseX_ = x; lastMouseY_ = y;
    orbiting_ = true;
}

void Scene3DPanel::mouseDrag(float x, float y) {
    if (!orbiting_ || !scene_) return;
    float dx = x - lastMouseX_;
    auto& cam = scene_->getCamera();
    cam.orbitAngle += dx * 0.5f;
    cam.orbitHeight += (y - lastMouseY_) * 0.02f;
    lastMouseX_ = x; lastMouseY_ = y;
}

void Scene3DPanel::mouseWheel(float delta) {
    if (!scene_) return;
    auto& cam = scene_->getCamera();
    cam.orbitDistance = std::max(1.0f, cam.orbitDistance - delta * 0.5f);
}

} // namespace dvds
