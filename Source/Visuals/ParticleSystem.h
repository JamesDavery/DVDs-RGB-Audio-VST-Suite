#pragma once
#include <vector>
#include <array>

namespace dvds {

struct Particle {
    float position[3];
    float velocity[3];
    float color[4];
    float life;
    float maxLife;
    float size;
    int paletteIndex;
};

struct ParticleEmitterConfig {
    float emitRate = 100.0f;
    float lifeMin = 0.5f, lifeMax = 2.0f;
    float sizeMin = 0.01f, sizeMax = 0.05f;
    float speedMin = 0.5f, speedMax = 2.0f;
    float gravity = -1.0f;
    float spread = 1.0f;
    float position[3] = { 0.0f, 0.0f, 0.0f };
    bool audioReactive = true;
    float audioEmitMul = 2.0f;  // multiply emit rate on beat
};

class ParticleSystem {
public:
    ParticleSystem();
    ~ParticleSystem();

    void initialize(int maxParticles = 10000);
    void setConfig(const ParticleEmitterConfig& config);
    void setPalette(const std::array<float, 20>& palette);
    void setAudioEnergy(float energy, bool beat);

    void update(float deltaTime);
    void render();

    int getActiveCount() const { return activeCount_; }
    unsigned int getOutputTexture() const { return 0; } // rendered into scene

private:
    std::vector<Particle> particles_;
    ParticleEmitterConfig config_;
    std::array<float, 20> palette_{};
    float emitAccumulator_ = 0.0f;
    int activeCount_ = 0;
    float audioEnergy_ = 0.0f;
    bool beatActive_ = false;

    void emitParticle();
};

} // namespace dvds
