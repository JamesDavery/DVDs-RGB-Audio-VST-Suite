#include "Visuals/VideoPlayer.h"

namespace dvds {

VideoPlayer::VideoPlayer() = default;
VideoPlayer::~VideoPlayer() { close(); }

bool VideoPlayer::open(const std::string& path) {
    filePath_ = path;
    // FFmpeg: avformat_open_input, avcodec_open2, etc.
    // For now, structural stub
    isOpen_ = true;
    return true;
}

void VideoPlayer::close() {
    if (!isOpen_) return;
    stop();
    // FFmpeg: avcodec_free_context, avformat_close_input, etc.
    isOpen_ = false;
}

void VideoPlayer::play() { playing_ = true; }
void VideoPlayer::pause() { playing_ = false; }
void VideoPlayer::stop() { playing_ = false; position_ = 0.0; }
void VideoPlayer::seek(double seconds) { position_ = seconds; }
void VideoPlayer::setLoop(bool loop) { looping_ = loop; }
void VideoPlayer::setPlaybackRate(float rate) { playbackRate_ = rate; }

VideoFrame VideoPlayer::getFrame() {
    if (!isOpen_) return {};
    return { currentTexture_, width_, height_, position_, true };
}

void VideoPlayer::advanceFrame(double deltaTime) {
    if (!playing_ || !isOpen_) return;
    position_ += deltaTime * playbackRate_;
    if (position_ >= duration_) {
        if (looping_) position_ = 0.0;
        else { playing_ = false; position_ = duration_; }
    }
    // FFmpeg: decode next frame, upload to texture
}

} // namespace dvds
