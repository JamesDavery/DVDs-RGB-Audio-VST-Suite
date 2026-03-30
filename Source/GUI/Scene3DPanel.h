#pragma once
#include "Visuals/Scene3D.h"
#include <functional>

namespace dvds {

class Scene3DPanel {
public:
    Scene3DPanel();
    ~Scene3DPanel();

    void setScene(Scene3D* scene);
    void paint(/* Graphics& g */);
    void resized(int width, int height);
    void mouseDown(float x, float y);
    void mouseDrag(float x, float y);
    void mouseWheel(float delta);

    std::function<void(const std::string&)> onLoadModel;

private:
    Scene3D* scene_ = nullptr;
    int width_ = 400, height_ = 400;
    float lastMouseX_ = 0, lastMouseY_ = 0;
    bool orbiting_ = false;
};

} // namespace dvds
