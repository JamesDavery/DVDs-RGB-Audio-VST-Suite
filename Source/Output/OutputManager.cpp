#include "Output/OutputManager.h"

namespace dvds {

OutputManager::OutputManager()
    : spout_(std::make_unique<SpoutSender>())
    , syphon_(std::make_unique<SyphonSender>())
    , ndi_(std::make_unique<NDISender>()) {}

OutputManager::~OutputManager() { shutdown(); }

void OutputManager::initialize(const OutputConfig& config) {
    config_ = config;
    if (config.spoutEnabled)
        spout_->initialize(config.outputName, config.width, config.height);
    if (config.syphonEnabled)
        syphon_->initialize(config.outputName, config.width, config.height);
    if (config.ndiEnabled)
        ndi_->initialize(config.outputName, config.width, config.height);
}

void OutputManager::shutdown() {
    spout_->shutdown();
    syphon_->shutdown();
    ndi_->shutdown();
}

void OutputManager::resize(int w, int h) {
    config_.width = w; config_.height = h;
    spout_->resize(w, h);
    syphon_->resize(w, h);
    ndi_->resize(w, h);
}

void OutputManager::sendFrame(unsigned int textureId, int w, int h) {
    if (spout_->isInitialized()) spout_->sendTexture(textureId, w, h);
    if (syphon_->isInitialized()) syphon_->sendTexture(textureId, w, h);
    if (ndi_->isInitialized()) ndi_->sendTexture(textureId, w, h);
}

void OutputManager::setSpoutEnabled(bool en) {
    if (en && !spout_->isInitialized())
        spout_->initialize(config_.outputName, config_.width, config_.height);
    else if (!en) spout_->shutdown();
    config_.spoutEnabled = en;
}

void OutputManager::setSyphonEnabled(bool en) {
    if (en && !syphon_->isInitialized())
        syphon_->initialize(config_.outputName, config_.width, config_.height);
    else if (!en) syphon_->shutdown();
    config_.syphonEnabled = en;
}

void OutputManager::setNDIEnabled(bool en) {
    if (en && !ndi_->isInitialized())
        ndi_->initialize(config_.outputName, config_.width, config_.height);
    else if (!en) ndi_->shutdown();
    config_.ndiEnabled = en;
}

bool OutputManager::isSpoutAvailable() const {
#ifdef _WIN32
    return true;
#else
    return false;
#endif
}

bool OutputManager::isSyphonAvailable() const {
#ifdef __APPLE__
    return true;
#else
    return false;
#endif
}

bool OutputManager::isNDIAvailable() const { return true; }

} // namespace dvds
