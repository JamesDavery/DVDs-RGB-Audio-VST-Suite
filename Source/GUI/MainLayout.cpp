#include "GUI/MainLayout.h"

namespace dvds {

MainLayout::MainLayout() = default;
MainLayout::~MainLayout() = default;

void MainLayout::initialize(int w, int h) {
    width_ = w; height_ = h;
    layoutPanels();
}

void MainLayout::resized(int w, int h) {
    width_ = w; height_ = h;
    layoutPanels();
}

void MainLayout::layoutPanels() {
    int leftW = 300;
    int rightW = 350;
    int bottomH = 200;
    int centerW = width_ - leftW - rightW;
    int topH = height_ - bottomH;

    leftPanel_ = { 0, 0, leftW, topH };
    centerPanel_ = { leftW, 0, centerW, topH };
    rightPanel_ = { leftW + centerW, 0, rightW, topH };
    bottomPanel_ = { 0, topH, width_, bottomH };

    // Color wheel takes top portion of left panel, palette strip below
    int wheelSize = std::min(leftW, topH - 80);
    colorWheel_.resized(wheelSize, wheelSize);
    paletteStrip_.resized(leftW, 60);

    audioReactPanel_.resized(rightW, topH);
    shaderBrowser_.resized(width_, bottomH);
    mixerPanel_.resized(width_, bottomH);
    scene3DPanel_.resized(centerW, topH);
}

void MainLayout::paint(/* Graphics& g */) {
    // Draw panel backgrounds
    // Draw panel borders/separators
    // Delegate painting to child components

    colorWheel_.paint();
    paletteStrip_.paint();
    audioReactPanel_.paint();

    // Bottom tab bar
    // Draw tab buttons: Shaders | Video | 3D | Settings
    switch (activeTab_) {
        case MainTab::Shaders:  shaderBrowser_.paint(); break;
        case MainTab::Video:    mixerPanel_.paint(); break;
        case MainTab::Scene3D:  scene3DPanel_.paint(); break;
        case MainTab::Settings: /* Settings panel */ break;
    }
}

void MainLayout::setActiveTab(MainTab tab) {
    activeTab_ = tab;
}

} // namespace dvds
