#pragma once
#include "Visuals/ShaderPipeline.h"
#include <vector>
#include <string>
#include <functional>

namespace dvds {

class ShaderBrowser {
public:
    ShaderBrowser();
    ~ShaderBrowser();

    void setShaderList(const std::vector<ShaderInfo>& shaders);
    void setISFDirectory(const std::string& path);
    void refresh();

    void paint(/* Graphics& g */);
    void resized(int width, int height);
    void mouseDown(float x, float y);

    int getSelectedIndex() const { return selectedIndex_; }
    std::function<void(int, const std::string&)> onShaderSelected;

private:
    struct BrowserEntry {
        std::string name;
        std::string category;
        bool isISF;
        std::string filePath;
        int originalIndex;
    };

    std::vector<BrowserEntry> entries_;
    int selectedIndex_ = 0;
    int scrollOffset_ = 0;
    int width_ = 400, height_ = 300;
    float itemHeight_ = 32.0f;
    std::string filterText_;
    std::string isfDirectory_;
};

} // namespace dvds
