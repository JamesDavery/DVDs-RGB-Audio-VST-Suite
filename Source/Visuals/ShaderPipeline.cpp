#include "Visuals/ShaderPipeline.h"
#include <cstring>

// OpenGL headers will come from JUCE's OpenGL module in actual build.
// This file provides the structural implementation; GL calls are stubbed
// with comments for non-OpenGL compilation contexts (e.g., unit tests).

namespace dvds {

const char* ShaderProgram::kDefaultVertexShader = R"(
#version 330 core
layout(location = 0) in vec2 aPos;
layout(location = 1) in vec2 aTexCoord;
out vec2 vTexCoord;
out vec2 vFragCoord;
uniform vec2 uResolution;
void main() {
    gl_Position = vec4(aPos, 0.0, 1.0);
    vTexCoord = aTexCoord;
    vFragCoord = aTexCoord * uResolution;
}
)";

ShaderProgram::ShaderProgram() = default;
ShaderProgram::~ShaderProgram() {
    // In real build: glDeleteProgram(programId_); etc.
}

bool ShaderProgram::compileFromSource(const std::string& vertSrc, const std::string& fragSrc) {
    // OpenGL compilation would happen here
    // For structure: store sources, set programId_ on success
    lastError_.clear();
    // Placeholder: assume success in non-GL context
    programId_ = 1; // dummy
    return true;
}

bool ShaderProgram::compileFragment(const std::string& fragSrc) {
    return compileFromSource(kDefaultVertexShader, fragSrc);
}

void ShaderProgram::bind() {
    // glUseProgram(programId_);
}

void ShaderProgram::unbind() {
    // glUseProgram(0);
}

int ShaderProgram::getUniformLocation(const std::string& name) {
    auto it = uniformCache_.find(name);
    if (it != uniformCache_.end()) return it->second;
    // int loc = glGetUniformLocation(programId_, name.c_str());
    int loc = -1; // placeholder
    uniformCache_[name] = loc;
    return loc;
}

void ShaderProgram::setFloat(const std::string& name, float v) {
    // int loc = getUniformLocation(name); glUniform1f(loc, v);
}
void ShaderProgram::setVec2(const std::string& name, float x, float y) {}
void ShaderProgram::setVec3(const std::string& name, float x, float y, float z) {}
void ShaderProgram::setVec4(const std::string& name, float x, float y, float z, float w) {}
void ShaderProgram::setInt(const std::string& name, int v) {}
void ShaderProgram::setBool(const std::string& name, bool v) {}
void ShaderProgram::setFloatArray(const std::string& name, const float* data, int count) {}
void ShaderProgram::setMat4(const std::string& name, const float* data) {}

unsigned int ShaderProgram::compileShader(unsigned int type, const std::string& src) {
    return 0; // placeholder
}

// -- ShaderPipeline --

ShaderPipeline::ShaderPipeline() {
    palette_.fill(0.0f);
    fftData_.resize(256, 0.0f);
}

ShaderPipeline::~ShaderPipeline() = default;

void ShaderPipeline::initialize(int w, int h) {
    width_ = w; height_ = h;
    resolution_[0] = static_cast<float>(w);
    resolution_[1] = static_cast<float>(h);
    loadBuiltInShaders();
    createQuad();
}

void ShaderPipeline::resize(int w, int h) {
    width_ = w; height_ = h;
    resolution_[0] = static_cast<float>(w);
    resolution_[1] = static_cast<float>(h);
}

void ShaderPipeline::setActiveShader(int index) {
    if (index >= 0 && index < static_cast<int>(shaders_.size())) {
        activeShaderIndex_ = index;
        activeProgram_ = std::make_unique<ShaderProgram>();
        activeProgram_->compileFragment(shaders_[index].fragmentSource);
    }
}

const ShaderInfo* ShaderPipeline::getActiveShaderInfo() const {
    if (activeShaderIndex_ >= 0 && activeShaderIndex_ < static_cast<int>(shaders_.size()))
        return &shaders_[activeShaderIndex_];
    return nullptr;
}

void ShaderPipeline::setPalette(const std::array<float, 20>& p) { palette_ = p; }
void ShaderPipeline::setTime(float t) { time_ = t; }
void ShaderPipeline::setResolution(float w, float h) { resolution_[0] = w; resolution_[1] = h; }
void ShaderPipeline::setFFTData(const float* data, int size) {
    fftData_.assign(data, data + std::min(size, 256));
    fftData_.resize(256, 0.0f);
}
void ShaderPipeline::setRMS(float rms) { rms_ = rms; }
void ShaderPipeline::setBPM(float bpm) { bpm_ = bpm; }
void ShaderPipeline::setBeat(float b) { beat_ = b; }
void ShaderPipeline::setBeatPhase(float p) { beatPhase_ = p; }

void ShaderPipeline::render() {
    if (!activeProgram_ || !activeProgram_->isValid()) return;

    activeProgram_->bind();
    activeProgram_->setFloat("uTime", time_);
    activeProgram_->setVec2("uResolution", resolution_[0], resolution_[1]);
    activeProgram_->setFloat("uRMS", rms_);
    activeProgram_->setFloat("uBPM", bpm_);
    activeProgram_->setFloat("uBeat", beat_);
    activeProgram_->setFloat("uBeatPhase", beatPhase_);

    for (int i = 0; i < 5; ++i) {
        std::string name = "uPalette[" + std::to_string(i) + "]";
        activeProgram_->setVec4(name,
            palette_[i * 4], palette_[i * 4 + 1],
            palette_[i * 4 + 2], palette_[i * 4 + 3]);
    }

    // Draw fullscreen quad
    // glBindVertexArray(quadVAO_); glDrawArrays(GL_TRIANGLES, 0, 6);

    activeProgram_->unbind();
}

void ShaderPipeline::addShader(const ShaderInfo& info) {
    shaders_.push_back(info);
}

void ShaderPipeline::createQuad() {
    // Fullscreen quad: 2 triangles covering [-1,1]
    // Created via glGenVertexArrays etc. in actual GL context
}

void ShaderPipeline::loadBuiltInShaders() {
    // All built-in shaders accept palette[5] as uniforms
    shaders_.clear();

    // 1. Plasma (Complementary)
    addShader({ "Plasma", "Complementary", R"(
#version 330 core
out vec4 FragColor;
uniform float uTime;
uniform vec2 uResolution;
uniform vec4 uPalette[5];
uniform float uRMS;

void main() {
    vec2 uv = gl_FragCoord.xy / uResolution;
    float v = 0.0;
    v += sin((uv.x * 10.0 + uTime) * (1.0 + uRMS));
    v += sin((uv.y * 10.0 + uTime) * 1.2);
    v += sin((uv.x * 10.0 + uv.y * 10.0 + uTime * 0.7) * 0.8);
    v += sin(length(uv - 0.5) * 20.0 - uTime * 2.0);
    v = v * 0.25 + 0.5;

    vec4 c1 = uPalette[0];
    vec4 c2 = uPalette[1];
    vec4 c3 = uPalette[2];
    vec4 c4 = uPalette[3];
    vec4 c5 = uPalette[4];

    vec4 color;
    if (v < 0.25) color = mix(c1, c2, v * 4.0);
    else if (v < 0.5) color = mix(c2, c3, (v - 0.25) * 4.0);
    else if (v < 0.75) color = mix(c3, c4, (v - 0.5) * 4.0);
    else color = mix(c4, c5, (v - 0.75) * 4.0);

    FragColor = color;
}
)", "Classic plasma with palette colors driven by complementary harmony" });

    // 2. Voronoi (Triadic)
    addShader({ "Voronoi", "Triadic", R"(
#version 330 core
out vec4 FragColor;
uniform float uTime;
uniform vec2 uResolution;
uniform vec4 uPalette[5];
uniform float uRMS;
uniform float uBeatPhase;

vec2 hash2(vec2 p) {
    p = vec2(dot(p, vec2(127.1, 311.7)), dot(p, vec2(269.5, 183.3)));
    return fract(sin(p) * 43758.5453);
}

void main() {
    vec2 uv = gl_FragCoord.xy / uResolution;
    float scale = 6.0 + uRMS * 4.0;
    vec2 p = uv * scale;
    vec2 ip = floor(p);
    vec2 fp = fract(p);

    float minDist = 10.0;
    float secondDist = 10.0;
    vec2 closestCell = vec2(0.0);

    for (int y = -1; y <= 1; y++) {
        for (int x = -1; x <= 1; x++) {
            vec2 neighbor = vec2(float(x), float(y));
            vec2 point = hash2(ip + neighbor);
            point = 0.5 + 0.5 * sin(uTime * 0.5 + 6.2831 * point);
            float d = length(neighbor + point - fp);
            if (d < minDist) {
                secondDist = minDist;
                minDist = d;
                closestCell = ip + neighbor;
            } else if (d < secondDist) {
                secondDist = d;
            }
        }
    }

    float edge = secondDist - minDist;
    int cellIdx = int(mod(closestCell.x + closestCell.y * 3.0, 5.0));
    vec4 cellColor = uPalette[clamp(cellIdx, 0, 4)];

    float edgeMix = smoothstep(0.0, 0.05 + uRMS * 0.1, edge);
    FragColor = mix(uPalette[4], cellColor, edgeMix);
}
)", "Voronoi cells colored by triadic palette" });

    // 3. Tunnel (Full Palette)
    addShader({ "Tunnel", "Full Palette", R"(
#version 330 core
out vec4 FragColor;
uniform float uTime;
uniform vec2 uResolution;
uniform vec4 uPalette[5];
uniform float uRMS;
uniform float uBeat;

void main() {
    vec2 uv = (gl_FragCoord.xy - 0.5 * uResolution) / min(uResolution.x, uResolution.y);
    float angle = atan(uv.y, uv.x);
    float dist = length(uv);
    float tunnel = 1.0 / (dist + 0.001);

    float tx = tunnel + uTime * 0.5;
    float ty = angle / 3.14159265;

    float stripe = sin(tx * 8.0) * 0.5 + 0.5;
    float ring = sin(ty * 6.0 + uTime) * 0.5 + 0.5;

    float idx = mod(stripe * 3.0 + ring * 2.0, 5.0);
    int i0 = int(idx);
    int i1 = int(mod(float(i0) + 1.0, 5.0));
    float f = fract(idx);

    vec4 color = mix(uPalette[clamp(i0, 0, 4)], uPalette[clamp(i1, 0, 4)], f);
    color *= smoothstep(0.0, 0.3, dist) * (1.0 + uBeat * 0.5);
    color.a = 1.0;
    FragColor = color;
}
)", "Infinite tunnel with all 5 palette colors" });

    // 4. Fractal Noise (Monochromatic)
    addShader({ "Fractal Noise", "Monochromatic", R"(
#version 330 core
out vec4 FragColor;
uniform float uTime;
uniform vec2 uResolution;
uniform vec4 uPalette[5];
uniform float uRMS;

float hash(vec2 p) {
    return fract(sin(dot(p, vec2(127.1, 311.7))) * 43758.5453);
}

float noise(vec2 p) {
    vec2 i = floor(p);
    vec2 f = fract(p);
    f = f * f * (3.0 - 2.0 * f);
    float a = hash(i);
    float b = hash(i + vec2(1.0, 0.0));
    float c = hash(i + vec2(0.0, 1.0));
    float d = hash(i + vec2(1.0, 1.0));
    return mix(mix(a, b, f.x), mix(c, d, f.x), f.y);
}

float fbm(vec2 p) {
    float val = 0.0, amp = 0.5;
    for (int i = 0; i < 6; i++) {
        val += amp * noise(p);
        p *= 2.0;
        amp *= 0.5;
    }
    return val;
}

void main() {
    vec2 uv = gl_FragCoord.xy / uResolution;
    float n = fbm(uv * 4.0 + uTime * 0.3 + uRMS * 2.0);
    int idx = clamp(int(n * 5.0), 0, 4);
    float f = fract(n * 5.0);
    int idx2 = clamp(idx + 1, 0, 4);
    FragColor = mix(uPalette[idx], uPalette[idx2], f);
}
)", "Layered fractal noise with monochromatic palette gradient" });

    // 5. Kaleidoscope (Triadic)
    addShader({ "Kaleidoscope", "Triadic", R"(
#version 330 core
out vec4 FragColor;
uniform float uTime;
uniform vec2 uResolution;
uniform vec4 uPalette[5];
uniform float uRMS;
uniform float uBeatPhase;

void main() {
    vec2 uv = (gl_FragCoord.xy - 0.5 * uResolution) / min(uResolution.x, uResolution.y);
    float angle = atan(uv.y, uv.x);
    float dist = length(uv);

    float segments = 6.0;
    angle = mod(angle, 3.14159265 * 2.0 / segments);
    angle = abs(angle - 3.14159265 / segments);

    vec2 p = vec2(cos(angle), sin(angle)) * dist;
    p += uTime * 0.1;

    float pattern = sin(p.x * 10.0 + uRMS * 5.0) * sin(p.y * 10.0);
    pattern = pattern * 0.5 + 0.5;

    int idx = clamp(int(pattern * 4.99), 0, 4);
    vec4 color = uPalette[idx];
    color *= 1.0 - dist * 0.5;
    color.a = 1.0;
    FragColor = color;
}
)", "Kaleidoscopic pattern using triadic colors" });

    // 6. Aurora (Analogous)
    addShader({ "Aurora", "Analogous", R"(
#version 330 core
out vec4 FragColor;
uniform float uTime;
uniform vec2 uResolution;
uniform vec4 uPalette[5];
uniform float uRMS;

float noise(vec2 p) {
    return fract(sin(dot(p, vec2(12.9898, 78.233))) * 43758.5453);
}

void main() {
    vec2 uv = gl_FragCoord.xy / uResolution;

    float wave1 = sin(uv.x * 3.0 + uTime * 0.5 + uRMS * 3.0) * 0.15;
    float wave2 = sin(uv.x * 5.0 - uTime * 0.3) * 0.1;
    float wave3 = sin(uv.x * 8.0 + uTime * 0.7) * 0.05;

    float center = 0.5 + wave1 + wave2 + wave3;
    float spread = 0.15 + uRMS * 0.1;
    float aurora = smoothstep(spread, 0.0, abs(uv.y - center));

    float colorIdx = uv.x * 4.0 + uTime * 0.2;
    int i0 = int(mod(colorIdx, 5.0));
    int i1 = int(mod(float(i0) + 1.0, 5.0));
    float f = fract(colorIdx);

    vec4 auroraColor = mix(uPalette[clamp(i0, 0, 4)], uPalette[clamp(i1, 0, 4)], f);
    vec4 bg = uPalette[0] * 0.1;
    FragColor = mix(bg, auroraColor, aurora);
    FragColor.a = 1.0;
}
)", "Flowing aurora with analogous palette transition" });

    // 7. Grid Pulse (Square)
    addShader({ "Grid Pulse", "Square", R"(
#version 330 core
out vec4 FragColor;
uniform float uTime;
uniform vec2 uResolution;
uniform vec4 uPalette[5];
uniform float uRMS;
uniform float uBeat;

void main() {
    vec2 uv = gl_FragCoord.xy / uResolution;
    float grid = 10.0;
    vec2 cell = floor(uv * grid);
    vec2 cellUv = fract(uv * grid);

    float pulse = sin(uTime * 2.0 + cell.x * 0.5 + cell.y * 0.7) * 0.5 + 0.5;
    pulse *= 1.0 + uBeat * 0.5;

    int colorIdx = int(mod(cell.x + cell.y, 5.0));
    vec4 cellColor = uPalette[clamp(colorIdx, 0, 4)];

    float border = step(0.05, cellUv.x) * step(cellUv.x, 0.95) *
                   step(0.05, cellUv.y) * step(cellUv.y, 0.95);

    vec4 color = cellColor * pulse * border;
    color += uPalette[4] * 0.05 * (1.0 - border);
    color.a = 1.0;
    FragColor = color;
}
)", "Pulsing grid using square harmony, reactive to beat" });

    // 8. Smoke (Monochromatic)
    addShader({ "Smoke", "Monochromatic", R"(
#version 330 core
out vec4 FragColor;
uniform float uTime;
uniform vec2 uResolution;
uniform vec4 uPalette[5];
uniform float uRMS;

float hash(vec2 p) { return fract(sin(dot(p, vec2(41.1, 289.7))) * 43758.5453); }

float vnoise(vec2 p) {
    vec2 i = floor(p), f = fract(p);
    f = f * f * (3.0 - 2.0 * f);
    return mix(mix(hash(i), hash(i + vec2(1,0)), f.x),
               mix(hash(i + vec2(0,1)), hash(i + vec2(1,1)), f.x), f.y);
}

void main() {
    vec2 uv = gl_FragCoord.xy / uResolution;
    uv.y += uTime * 0.05;

    float smoke = 0.0;
    float amp = 1.0;
    vec2 p = uv * 3.0;
    for (int i = 0; i < 5; i++) {
        smoke += vnoise(p + uTime * 0.1 * float(i+1)) * amp;
        p *= 2.1;
        amp *= 0.5;
    }
    smoke = smoke / 2.0;

    float t = smoke * (1.0 + uRMS);
    int i0 = clamp(int(t * 4.0), 0, 4);
    int i1 = clamp(i0 + 1, 0, 4);
    FragColor = mix(uPalette[i0], uPalette[i1], fract(t * 4.0));
    FragColor.a = 1.0;
}
)", "Drifting smoke colored by monochromatic shades" });

    // 9. Split Tone (Complementary)
    addShader({ "Split Tone", "Complementary", R"(
#version 330 core
out vec4 FragColor;
uniform float uTime;
uniform vec2 uResolution;
uniform vec4 uPalette[5];
uniform float uRMS;

void main() {
    vec2 uv = gl_FragCoord.xy / uResolution;
    float wave = sin(uv.x * 20.0 + uTime + uRMS * 10.0) *
                 cos(uv.y * 15.0 - uTime * 0.7) * 0.5 + 0.5;
    float split = step(0.5, wave);
    vec4 color = mix(uPalette[1], uPalette[3], split);
    float edge = smoothstep(0.48, 0.52, wave);
    color = mix(uPalette[2], color, abs(edge * 2.0 - 1.0));
    color.a = 1.0;
    FragColor = color;
}
)", "Sharp split-tone effect using complementary pair" });

    // 10. Gradient Flow (Analogous)
    addShader({ "Gradient Flow", "Analogous", R"(
#version 330 core
out vec4 FragColor;
uniform float uTime;
uniform vec2 uResolution;
uniform vec4 uPalette[5];
uniform float uRMS;

void main() {
    vec2 uv = gl_FragCoord.xy / uResolution;
    float t = uv.x + sin(uv.y * 3.0 + uTime) * 0.1 + uRMS * 0.2;
    t = mod(t + uTime * 0.05, 1.0);
    float idx = t * 4.0;
    int i0 = clamp(int(idx), 0, 4);
    int i1 = clamp(i0 + 1, 0, 4);
    FragColor = mix(uPalette[i0], uPalette[i1], fract(idx));
    FragColor.a = 1.0;
}
)", "Smooth flowing gradient through analogous palette" });

    // 11-20: Additional shaders
    addShader({ "Hex Grid", "Triadic", R"(
#version 330 core
out vec4 FragColor;
uniform float uTime;
uniform vec2 uResolution;
uniform vec4 uPalette[5];
uniform float uRMS;

vec2 hexCenter(vec2 p) {
    vec2 q = vec2(p.x * 2.0 / sqrt(3.0), p.y);
    vec2 r = floor(q + 0.5);
    return r;
}

void main() {
    vec2 uv = (gl_FragCoord.xy / uResolution - 0.5) * 10.0;
    vec2 hc = hexCenter(uv);
    float d = length(uv - hc * vec2(sqrt(3.0)/2.0, 1.0));
    float ring = smoothstep(0.45, 0.4, d);
    int ci = int(mod(hc.x + hc.y * 2.0, 5.0));
    float pulse = sin(uTime + hc.x * 0.3 + hc.y * 0.5) * 0.5 + 0.5;
    pulse *= 1.0 + uRMS;
    vec4 color = uPalette[clamp(ci, 0, 4)] * ring * pulse;
    color.a = 1.0;
    FragColor = color;
}
)", "Hexagonal grid with triadic coloring" });

    addShader({ "Ink Bleed", "Monochromatic", R"(
#version 330 core
out vec4 FragColor;
uniform float uTime;
uniform vec2 uResolution;
uniform vec4 uPalette[5];
uniform float uRMS;

float n(vec2 p) { return fract(sin(dot(p, vec2(12.9898,78.233))) * 43758.5453); }

void main() {
    vec2 uv = gl_FragCoord.xy / uResolution;
    float ink = 0.0;
    vec2 p = uv * 5.0;
    for (int i = 0; i < 4; i++) {
        ink += n(floor(p)) * pow(0.5, float(i));
        p *= 2.0;
    }
    ink = smoothstep(0.3 - uRMS * 0.2, 0.7 + uRMS * 0.1, ink);
    FragColor = mix(uPalette[0], uPalette[4], ink);
    FragColor.a = 1.0;
}
)", "Ink bleeding effect with monochromatic tones" });

    addShader({ "Waveform", "Full Palette", R"(
#version 330 core
out vec4 FragColor;
uniform float uTime;
uniform vec2 uResolution;
uniform vec4 uPalette[5];
uniform float uRMS;
uniform float uBeatPhase;

void main() {
    vec2 uv = gl_FragCoord.xy / uResolution;
    float wave = sin(uv.x * 30.0 + uTime * 3.0) * uRMS * 0.3;
    wave += sin(uv.x * 15.0 - uTime * 2.0) * 0.1;
    float center = 0.5 + wave;
    float d = abs(uv.y - center);
    float glow = 0.01 / (d + 0.001);
    glow = min(glow, 2.0);
    int ci = int(mod(uv.x * 5.0 + uTime * 0.5, 5.0));
    FragColor = uPalette[clamp(ci, 0, 4)] * glow;
    FragColor.a = 1.0;
}
)", "Audio waveform visualization across palette" });

    addShader({ "Radial Burst", "Full Palette", R"(
#version 330 core
out vec4 FragColor;
uniform float uTime;
uniform vec2 uResolution;
uniform vec4 uPalette[5];
uniform float uRMS;
uniform float uBeat;

void main() {
    vec2 uv = (gl_FragCoord.xy - 0.5 * uResolution) / min(uResolution.x, uResolution.y);
    float angle = atan(uv.y, uv.x) / 6.28318 + 0.5;
    float dist = length(uv);
    float rays = sin(angle * 12.0 + uTime + uRMS * 5.0) * 0.5 + 0.5;
    float ring = sin(dist * 20.0 - uTime * 3.0) * 0.5 + 0.5;
    float pattern = rays * ring * (1.0 + uBeat);
    int ci = int(mod(angle * 5.0 + dist * 3.0, 5.0));
    vec4 color = uPalette[clamp(ci, 0, 4)] * pattern;
    color *= smoothstep(1.0, 0.0, dist);
    color.a = 1.0;
    FragColor = color;
}
)", "Radial burst with all palette colors on beat" });

    addShader({ "Liquid", "Analogous", R"(
#version 330 core
out vec4 FragColor;
uniform float uTime;
uniform vec2 uResolution;
uniform vec4 uPalette[5];
uniform float uRMS;

void main() {
    vec2 uv = gl_FragCoord.xy / uResolution;
    vec2 p = uv * 3.0;
    float t = uTime * 0.3;
    float v = sin(p.x + sin(p.y + t) * 2.0 + uRMS * 3.0);
    v += sin(p.y * 1.5 + sin(p.x * 0.5 + t * 0.7));
    v += sin(length(p - vec2(1.5)) * 3.0 - t);
    v = v / 3.0 * 0.5 + 0.5;
    float idx = v * 4.0;
    int i0 = clamp(int(idx), 0, 4);
    int i1 = clamp(i0+1, 0, 4);
    FragColor = mix(uPalette[i0], uPalette[i1], fract(idx));
    FragColor.a = 1.0;
}
)", "Flowing liquid with analogous color blending" });

    addShader({ "Particle Trails", "Full Palette", R"(
#version 330 core
out vec4 FragColor;
uniform float uTime;
uniform vec2 uResolution;
uniform vec4 uPalette[5];
uniform float uRMS;

void main() {
    vec2 uv = gl_FragCoord.xy / uResolution;
    float brightness = 0.0;
    int closest = 0;
    for (int i = 0; i < 15; i++) {
        float fi = float(i);
        vec2 pos = vec2(
            0.5 + 0.3 * sin(uTime * 0.3 + fi * 1.1 + uRMS * fi),
            0.5 + 0.3 * cos(uTime * 0.4 + fi * 0.9)
        );
        float d = length(uv - pos);
        float glow = 0.003 / (d * d + 0.001);
        brightness += glow;
        if (glow > 0.5) closest = int(mod(fi, 5.0));
    }
    brightness = min(brightness, 3.0);
    FragColor = uPalette[clamp(closest, 0, 4)] * brightness * 0.5;
    FragColor.a = 1.0;
}
)", "Floating particle trails using full palette" });

    addShader({ "Fractal Flame", "Full Palette", R"(
#version 330 core
out vec4 FragColor;
uniform float uTime;
uniform vec2 uResolution;
uniform vec4 uPalette[5];
uniform float uRMS;

void main() {
    vec2 uv = (gl_FragCoord.xy - 0.5 * uResolution) / min(uResolution.x, uResolution.y);
    vec2 z = uv * (2.0 + uRMS);
    float intensity = 0.0;
    for (int i = 0; i < 20; i++) {
        float r = length(z);
        float a = atan(z.y, z.x);
        z = vec2(sin(a * 3.0 + uTime * 0.1) / r, cos(a * 2.0 - uTime * 0.15) / r);
        z += uv;
        intensity += 1.0 / (1.0 + length(z) * 10.0);
    }
    intensity /= 20.0;
    float idx = intensity * 4.0;
    int i0 = clamp(int(idx), 0, 4);
    int i1 = clamp(i0+1, 0, 4);
    FragColor = mix(uPalette[i0], uPalette[i1], fract(idx)) * 2.0;
    FragColor.a = 1.0;
}
)", "Fractal flame iterations using all palette colors" });

    addShader({ "Neon Rings", "Complementary", R"(
#version 330 core
out vec4 FragColor;
uniform float uTime;
uniform vec2 uResolution;
uniform vec4 uPalette[5];
uniform float uRMS;
uniform float uBeat;

void main() {
    vec2 uv = (gl_FragCoord.xy - 0.5 * uResolution) / min(uResolution.x, uResolution.y);
    float dist = length(uv);
    float glow = 0.0;
    for (int i = 0; i < 5; i++) {
        float radius = 0.1 + float(i) * 0.12 + uRMS * 0.05;
        float ring = abs(dist - radius);
        glow += 0.003 / (ring + 0.001) * (1.0 + uBeat * 0.3);
    }
    glow = min(glow, 3.0);
    float angle = atan(uv.y, uv.x) / 6.28318 + 0.5;
    int ci = int(mod(angle * 5.0, 5.0));
    FragColor = uPalette[clamp(ci, 0, 4)] * glow * 0.3;
    FragColor.a = 1.0;
}
)", "Concentric neon rings with complementary colors" });

    addShader({ "Matrix Rain", "Monochromatic", R"(
#version 330 core
out vec4 FragColor;
uniform float uTime;
uniform vec2 uResolution;
uniform vec4 uPalette[5];
uniform float uRMS;

float hash(float n) { return fract(sin(n) * 43758.5453); }

void main() {
    vec2 uv = gl_FragCoord.xy / uResolution;
    float columns = 40.0;
    float col = floor(uv.x * columns);
    float speed = hash(col) * 2.0 + 0.5 + uRMS;
    float offset = hash(col * 7.3) * 100.0;
    float drop = fract(uv.y + uTime * speed * 0.1 + offset);
    float bright = pow(drop, 8.0);
    int ci = int(mod(col, 5.0));
    FragColor = uPalette[clamp(ci, 0, 4)] * bright;
    FragColor.a = 1.0;
}
)", "Digital rain effect with monochromatic palette" });

    addShader({ "Interference", "Split-Complementary", R"(
#version 330 core
out vec4 FragColor;
uniform float uTime;
uniform vec2 uResolution;
uniform vec4 uPalette[5];
uniform float uRMS;
uniform float uBeatPhase;

void main() {
    vec2 uv = (gl_FragCoord.xy - 0.5 * uResolution) / min(uResolution.x, uResolution.y);

    float v1 = sin(length(uv - vec2(0.3, 0.0)) * 30.0 - uTime * 2.0);
    float v2 = sin(length(uv + vec2(0.3, 0.0)) * 30.0 - uTime * 2.0 + uRMS * 5.0);
    float v3 = sin(length(uv - vec2(0.0, 0.3)) * 25.0 + uTime * 1.5);

    float pattern = (v1 + v2 + v3) / 3.0 * 0.5 + 0.5;
    float idx = pattern * 4.0 + uBeatPhase;
    int i0 = int(mod(idx, 5.0));
    int i1 = int(mod(float(i0) + 1.0, 5.0));
    FragColor = mix(uPalette[clamp(i0,0,4)], uPalette[clamp(i1,0,4)], fract(idx));
    FragColor.a = 1.0;
}
)", "Wave interference pattern with split-complementary colors" });
}

} // namespace dvds
