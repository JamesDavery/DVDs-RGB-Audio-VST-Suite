#pragma once
#include <string>
#include <memory>
#include <functional>

namespace dvds {

enum class VideoCodec { H264, HAP, ProRes, Unknown };

struct VideoFrame {
    unsigned int textureId = 0;
    int width = 0, height = 0;
    double timestamp = 0.0;
    bool valid = false;
};

class VideoPlayer {
public:
    VideoPlayer();
    ~VideoPlayer();

    bool open(const std::string& path);
    void close();
    bool isOpen() const { return isOpen_; }

    void play();
    void pause();
    void stop();
    void seek(double seconds);
    void setLoop(bool loop);
    void setPlaybackRate(float rate);

    VideoFrame getFrame();
    void advanceFrame(double deltaTime);

    double getDuration() const { return duration_; }
    double getPosition() const { return position_; }
    int getWidth() const { return width_; }
    int getHeight() const { return height_; }
    float getFPS() const { return fps_; }
    bool isPlaying() const { return playing_; }

    unsigned int getCurrentTexture() const { return currentTexture_; }

private:
    bool isOpen_ = false;
    bool playing_ = false;
    bool looping_ = true;
    float playbackRate_ = 1.0f;
    double duration_ = 0.0;
    double position_ = 0.0;
    int width_ = 0, height_ = 0;
    float fps_ = 30.0f;
    unsigned int currentTexture_ = 0;
    std::string filePath_;
    // FFmpeg decoder context would go here
};

} // namespace dvds
