#include "GUI/ShaderBrowser.h"
#include <algorithm>

namespace dvds {

ShaderBrowser::ShaderBrowser() = default;
ShaderBrowser::~ShaderBrowser() = default;

void ShaderBrowser::setShaderList(const std::vector<ShaderInfo>& shaders) {
    entries_.clear();
    for (int i = 0; i < static_cast<int>(shaders.size()); ++i) {
        entries_.push_back({ shaders[i].name, shaders[i].category, false, "", i });
    }
}

void ShaderBrowser::setISFDirectory(const std::string& path) {
    isfDirectory_ = path;
    refresh();
}

void ShaderBrowser::refresh() {
    // Scan ISF directory for .fs files, parse metadata for names/categories
    // Add them to entries_ with isISF = true
}

void ShaderBrowser::paint(/* Graphics& g */) {
    int visibleItems = static_cast<int>(height_ / itemHeight_);
    for (int i = 0; i < visibleItems && (i + scrollOffset_) < static_cast<int>(entries_.size()); ++i) {
        int idx = i + scrollOffset_;
        const auto& entry = entries_[idx];
        // Draw item background (highlighted if selected)
        // Draw name and category tag
        // Draw ISF badge if applicable
    }
}

void ShaderBrowser::resized(int w, int h) { width_ = w; height_ = h; }

void ShaderBrowser::mouseDown(float x, float y) {
    int idx = scrollOffset_ + static_cast<int>(y / itemHeight_);
    if (idx >= 0 && idx < static_cast<int>(entries_.size())) {
        selectedIndex_ = idx;
        if (onShaderSelected)
            onShaderSelected(entries_[idx].originalIndex, entries_[idx].filePath);
    }
}

} // namespace dvds
