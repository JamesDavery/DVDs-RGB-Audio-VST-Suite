#pragma once
#include "GUI/ColorWheelComponent.h"
#include "GUI/PaletteStrip.h"
#include "GUI/AudioReactPanel.h"
#include "GUI/ShaderBrowser.h"
#include "GUI/MixerPanel.h"
#include "GUI/Scene3DPanel.h"
#include <memory>
#include <string>

namespace dvds {

enum class MainTab {
    Shaders = 0,
    Video,
    Scene3D,
    Settings
};

class MainLayout {
public:
    MainLayout();
    ~MainLayout();

    void initialize(int width, int height);
    void resized(int width, int height);
    void paint(/* Graphics& g */);

    ColorWheelComponent& getColorWheel() { return colorWheel_; }
    PaletteStrip& getPaletteStrip() { return paletteStrip_; }
    AudioReactPanel& getAudioReactPanel() { return audioReactPanel_; }
    ShaderBrowser& getShaderBrowser() { return shaderBrowser_; }
    MixerPanel& getMixerPanel() { return mixerPanel_; }
    Scene3DPanel& getScene3DPanel() { return scene3DPanel_; }

    void setActiveTab(MainTab tab);
    MainTab getActiveTab() const { return activeTab_; }

private:
    int width_ = 1200, height_ = 800;

    // Left panel: Color Wheel + Palette Strip
    ColorWheelComponent colorWheel_;
    PaletteStrip paletteStrip_;

    // Center: Visual viewport (OpenGL - handled by PluginEditor)

    // Right panel: Audio Reactivity
    AudioReactPanel audioReactPanel_;

    // Bottom panel: Tabbed browser
    ShaderBrowser shaderBrowser_;
    MixerPanel mixerPanel_;
    Scene3DPanel scene3DPanel_;

    MainTab activeTab_ = MainTab::Shaders;

    struct PanelBounds {
        int x, y, w, h;
    };
    PanelBounds leftPanel_, centerPanel_, rightPanel_, bottomPanel_;

    void layoutPanels();
};

} // namespace dvds
