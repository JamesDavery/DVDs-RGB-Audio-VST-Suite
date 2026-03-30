#include "Visuals/ParticleSystem.h"
#include <cmath>
#include <algorithm>
#include <cstdlib>

namespace dvds {

ParticleSystem::ParticleSystem() { palette_.fill(0.0f); }
ParticleSystem::~ParticleSystem() = default;

void ParticleSystem::initialize(int maxParticles) {
    particles_.resize(maxParticles);
    for (auto& p : particles_) p.life = -1.0f;
    activeCount_ = 0;
}

void ParticleSystem::setConfig(const ParticleEmitterConfig& config) { config_ = config; }
void ParticleSystem::setPalette(const std::array<float, 20>& p) { palette_ = p; }

void ParticleSystem::setAudioEnergy(float energy, bool beat) {
    audioEnergy_ = energy;
    beatActive_ = beat;
}

static float randFloat(float min, float max) {
    float t = static_cast<float>(rand()) / RAND_MAX;
    return min + t * (max - min);
}

void ParticleSystem::emitParticle() {
    for (auto& p : particles_) {
        if (p.life >= 0.0f) continue;

        p.life = randFloat(config_.lifeMin, config_.lifeMax);
        p.maxLife = p.life;
        p.size = randFloat(config_.sizeMin, config_.sizeMax);
        p.paletteIndex = rand() % 5;

        p.position[0] = config_.position[0] + randFloat(-0.1f, 0.1f);
        p.position[1] = config_.position[1] + randFloat(-0.1f, 0.1f);
        p.position[2] = config_.position[2] + randFloat(-0.1f, 0.1f);

        float speed = randFloat(config_.speedMin, config_.speedMax);
        float theta = randFloat(0.0f, 6.28318f);
        float phi = randFloat(0.0f, 3.14159f * config_.spread);
        p.velocity[0] = speed * std::sin(phi) * std::cos(theta);
        p.velocity[1] = speed * std::cos(phi);
        p.velocity[2] = speed * std::sin(phi) * std::sin(theta);

        int ci = p.paletteIndex * 4;
        p.color[0] = palette_[ci]; p.color[1] = palette_[ci+1];
        p.color[2] = palette_[ci+2]; p.color[3] = palette_[ci+3];

        ++activeCount_;
        return;
    }
}

void ParticleSystem::update(float dt) {
    float emitRate = config_.emitRate;
    if (config_.audioReactive) {
        emitRate *= (1.0f + audioEnergy_ * config_.audioEmitMul);
        if (beatActive_) emitRate *= 3.0f;
    }

    emitAccumulator_ += emitRate * dt;
    while (emitAccumulator_ >= 1.0f) {
        emitParticle();
        emitAccumulator_ -= 1.0f;
    }

    activeCount_ = 0;
    for (auto& p : particles_) {
        if (p.life < 0.0f) continue;
        p.life -= dt;
        if (p.life < 0.0f) continue;

        p.velocity[1] += config_.gravity * dt;
        p.position[0] += p.velocity[0] * dt;
        p.position[1] += p.velocity[1] * dt;
        p.position[2] += p.velocity[2] * dt;

        float lifeRatio = p.life / p.maxLife;
        p.color[3] = lifeRatio; // fade out
        ++activeCount_;
    }
}

void ParticleSystem::render() {
    // Upload particle positions/colors to GPU, draw as points or quads
}

} // namespace dvds
