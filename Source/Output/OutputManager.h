#pragma once
#include "Output/SpoutSender.h"
#include "Output/SyphonSender.h"
#include "Output/NDISender.h"
#include <memory>
#include <string>

namespace dvds {

struct OutputConfig {
    bool spoutEnabled = false;
    bool syphonEnabled = false;
    bool ndiEnabled = false;
    std::string outputName = "DVDs-RGB Output";
    int width = 1920;
    int height = 1080;
};

class OutputManager {
public:
    OutputManager();
    ~OutputManager();

    void initialize(const OutputConfig& config);
    void shutdown();
    void resize(int width, int height);

    void sendFrame(unsigned int textureId, int width, int height);

    void setSpoutEnabled(bool enabled);
    void setSyphonEnabled(bool enabled);
    void setNDIEnabled(bool enabled);

    bool isSpoutAvailable() const;
    bool isSyphonAvailable() const;
    bool isNDIAvailable() const;

    const OutputConfig& getConfig() const { return config_; }

private:
    OutputConfig config_;
    std::unique_ptr<SpoutSender> spout_;
    std::unique_ptr<SyphonSender> syphon_;
    std::unique_ptr<NDISender> ndi_;
};

} // namespace dvds
