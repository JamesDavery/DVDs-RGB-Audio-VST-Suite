#include "PluginEditor.h"
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

static juce::Colour toJuceColour(const dvds::RGB& c) {
    return juce::Colour::fromFloatRGBA(
        juce::jlimit(0.0f, 1.0f, c.r),
        juce::jlimit(0.0f, 1.0f, c.g),
        juce::jlimit(0.0f, 1.0f, c.b),
        1.0f);
}

// ==================== ColorWheelWidget ====================

ColorWheelWidget::ColorWheelWidget() {
    dvds::OKHsl def = { 0.0f, 0.75f, 0.65f };
    palette_.fill(def);
}

void ColorWheelWidget::paint(juce::Graphics& g) {
    auto bounds = getLocalBounds().toFloat().reduced(4.0f);
    float cx = bounds.getCentreX();
    float cy = bounds.getCentreY();
    float outerR = juce::jmin(bounds.getWidth(), bounds.getHeight()) * 0.5f;
    float innerR = outerR * 0.65f;
    float midR = (outerR + innerR) * 0.5f;
    float ringW = outerR - innerR;

    // Draw color wheel ring
    for (int deg = 0; deg < 360; ++deg) {
        float angle1 = deg * (float)M_PI / 180.0f;
        float angle2 = (deg + 1.5f) * (float)M_PI / 180.0f;

        dvds::RGB rgb = dvds::clampRgb(dvds::okhslToRgb({ (float)deg, 0.85f, 0.65f }));
        g.setColour(toJuceColour(rgb));

        juce::Path segment;
        segment.addCentredArc(cx, cy, outerR, outerR, 0.0f, angle1 - (float)M_PI * 0.5f, angle2 - (float)M_PI * 0.5f, true);
        segment.addCentredArc(cx, cy, innerR, innerR, 0.0f, angle2 - (float)M_PI * 0.5f, angle1 - (float)M_PI * 0.5f, false);
        segment.closeSubPath();
        g.fillPath(segment);
    }

    // Dark inner circle
    g.setColour(juce::Colour(0xFF1A1A2E));
    g.fillEllipse(cx - innerR + 2, cy - innerR + 2, (innerR - 2) * 2, (innerR - 2) * 2);

    // Draw base hue indicator line
    float hueRad = (baseHue_ - 90.0f) * (float)M_PI / 180.0f;
    float lx1 = cx + innerR * std::cos(hueRad);
    float ly1 = cy + innerR * std::sin(hueRad);
    float lx2 = cx + outerR * std::cos(hueRad);
    float ly2 = cy + outerR * std::sin(hueRad);
    g.setColour(juce::Colours::white);
    g.drawLine(lx1, ly1, lx2, ly2, 2.5f);

    // Draw 5 palette handles on the wheel
    for (int i = 0; i < 5; ++i) {
        float h = palette_[i].h;
        float handleRad = (h - 90.0f) * (float)M_PI / 180.0f;
        float hx = cx + midR * std::cos(handleRad);
        float hy = cy + midR * std::sin(handleRad);
        float handleSize = (i == 0) ? 10.0f : 7.0f;

        dvds::RGB rgb = dvds::clampRgb(dvds::okhslToRgb(palette_[i]));
        g.setColour(toJuceColour(rgb));
        g.fillEllipse(hx - handleSize, hy - handleSize, handleSize * 2, handleSize * 2);
        g.setColour(juce::Colours::white);
        g.drawEllipse(hx - handleSize, hy - handleSize, handleSize * 2, handleSize * 2, 1.5f);
    }

    // Draw hue value in center
    g.setColour(juce::Colours::white.withAlpha(0.8f));
    g.setFont(16.0f);
    g.drawText(juce::String(int(baseHue_)) + juce::CharPointer_UTF8("\xc2\xb0"),
               juce::Rectangle<float>(cx - 30, cy - 10, 60, 20),
               juce::Justification::centred);
}

float ColorWheelWidget::pointToHue(float x, float y) const {
    auto bounds = getLocalBounds().toFloat();
    float cx = bounds.getCentreX();
    float cy = bounds.getCentreY();
    float angle = std::atan2(y - cy, x - cx);
    float deg = angle * 180.0f / (float)M_PI + 90.0f;
    return dvds::wrapHue(deg);
}

void ColorWheelWidget::mouseDown(const juce::MouseEvent& e) {
    float hue = pointToHue((float)e.x, (float)e.y);
    setBaseHue(hue);
}

void ColorWheelWidget::mouseDrag(const juce::MouseEvent& e) {
    float hue = pointToHue((float)e.x, (float)e.y);
    setBaseHue(hue);
}

void ColorWheelWidget::setBaseHue(float hue) {
    baseHue_ = dvds::wrapHue(hue);
    if (onHueChanged) onHueChanged(baseHue_);
    repaint();
}

void ColorWheelWidget::setPalette(const std::array<dvds::OKHsl, 5>& p) {
    palette_ = p;
    repaint();
}

// ==================== PaletteStripWidget ====================

void PaletteStripWidget::paint(juce::Graphics& g) {
    auto bounds = getLocalBounds().toFloat();
    float gap = 3.0f;
    float sw = (bounds.getWidth() - gap * 4.0f) / 5.0f;
    float h = bounds.getHeight();

    for (int i = 0; i < 5; ++i) {
        float x = i * (sw + gap);
        auto colour = toJuceColour(palette_[i]);
        g.setColour(colour);
        g.fillRoundedRectangle(x, 0, sw, h - 16, 4.0f);

        // Hex label
        g.setColour(colour.getBrightness() > 0.5f ? juce::Colours::black : juce::Colours::white);
        g.setFont(10.0f);
        juce::String hex = "#" + juce::String::toHexString((int)palette_[i].toHex()).paddedLeft('0', 6).toUpperCase();
        g.drawText(hex, juce::Rectangle<float>(x, h - 15, sw, 14), juce::Justification::centred);
    }
}

void PaletteStripWidget::setPalette(const std::array<dvds::RGB, 5>& p) {
    palette_ = p;
    repaint();
}

// ==================== FFTDisplayWidget ====================

void FFTDisplayWidget::paint(juce::Graphics& g) {
    auto bounds = getLocalBounds().toFloat();
    g.setColour(juce::Colour(0xFF0D0D1A));
    g.fillRoundedRectangle(bounds, 4.0f);

    float specH = bounds.getHeight() * 0.4f;
    float bandH = bounds.getHeight() * 0.25f;
    float infoH = bounds.getHeight() * 0.15f;
    float bandY = specH + 4;
    float infoY = bandY + bandH + 4;

    // Spectrum bars
    if (!spectrum_.empty()) {
        int numBars = juce::jmin((int)spectrum_.size(), (int)(bounds.getWidth() / 2));
        float barW = bounds.getWidth() / (float)numBars;
        for (int i = 0; i < numBars; ++i) {
            float mag = juce::jlimit(0.0f, 1.0f, spectrum_[i] * 5.0f);
            float barH = mag * specH * 0.9f;
            float t = (float)i / (float)numBars;

            juce::Colour barCol;
            if (paletteColors_[0].r > 0 || paletteColors_[0].g > 0 || paletteColors_[0].b > 0) {
                int ci = (int)(t * 4.99f);
                barCol = toJuceColour(paletteColors_[juce::jlimit(0, 4, ci)]);
            } else {
                barCol = juce::Colour::fromHSV(t * 0.7f, 0.8f, 0.9f, 1.0f);
            }
            g.setColour(barCol.withAlpha(0.8f));
            g.fillRect(bounds.getX() + i * barW, specH - barH, barW - 1, barH);
        }
    }

    // Band meters
    const char* bandNames[] = { "SUB", "LOW", "MID", "HI-M", "PRES", "AIR" };
    float meterW = (bounds.getWidth() - 30) / 6.0f;
    for (int i = 0; i < 6; ++i) {
        float x = bounds.getX() + 5 + i * meterW;
        float energy = juce::jlimit(0.0f, 1.0f, bandEnergies_[i]);
        float fillH = energy * (bandH - 14);

        juce::Colour meterCol = (i < 5) ? toJuceColour(paletteColors_[juce::jlimit(0, 4, i)])
                                         : toJuceColour(paletteColors_[4]);
        if (meterCol.getBrightness() < 0.1f) meterCol = juce::Colour(0xFF4488FF);

        g.setColour(juce::Colour(0xFF222244));
        g.fillRoundedRectangle(x, bandY, meterW - 4, bandH - 14, 2.0f);
        g.setColour(meterCol);
        g.fillRoundedRectangle(x, bandY + (bandH - 14) - fillH, meterW - 4, fillH, 2.0f);

        g.setColour(juce::Colours::white.withAlpha(0.6f));
        g.setFont(9.0f);
        g.drawText(bandNames[i], juce::Rectangle<float>(x, bandY + bandH - 13, meterW - 4, 12),
                   juce::Justification::centred);
    }

    // Info line: RMS, BPM, beat indicator
    g.setColour(juce::Colours::white.withAlpha(0.7f));
    g.setFont(12.0f);
    juce::String info = "RMS: " + juce::String(rms_, 2)
                      + "  BPM: " + juce::String(int(bpm_));
    g.drawText(info, juce::Rectangle<float>(bounds.getX() + 5, infoY, bounds.getWidth() * 0.6f, infoH),
               juce::Justification::centredLeft);

    // Beat indicator circle
    float beatX = bounds.getRight() - 25;
    float beatY2 = infoY + infoH * 0.5f;
    g.setColour(beat_ ? juce::Colours::white : juce::Colour(0xFF333355));
    g.fillEllipse(beatX - 8, beatY2 - 8, 16, 16);
    g.setColour(juce::Colours::white.withAlpha(0.4f));
    g.setFont(9.0f);
    g.drawText("BEAT", juce::Rectangle<float>(beatX - 20, beatY2 + 10, 40, 12), juce::Justification::centred);
}

void FFTDisplayWidget::setSpectrum(const float* data, int size) {
    spectrum_.assign(data, data + size);
    repaint();
}

void FFTDisplayWidget::setBandEnergies(const std::array<float, 6>& b) { bandEnergies_ = b; }
void FFTDisplayWidget::setRMS(float r) { rms_ = r; }
void FFTDisplayWidget::setBPM(float b) { bpm_ = b; }
void FFTDisplayWidget::setBeat(bool b) { beat_ = b; }
void FFTDisplayWidget::setPaletteColors(const std::array<dvds::RGB, 5>& c) { paletteColors_ = c; }

// ==================== ShaderViewport ====================

static const char* kViewportVertSrc = R"(
attribute vec2 aPos;
varying vec2 vTexCoord;
void main() {
    gl_Position = vec4(aPos, 0.0, 1.0);
    vTexCoord = aPos * 0.5 + 0.5;
}
)";

static const char* kDefaultFragSrc = R"(
varying vec2 vTexCoord;
uniform float uTime;
uniform vec2 uResolution;
uniform vec4 uPalette0;
uniform vec4 uPalette1;
uniform vec4 uPalette2;
uniform vec4 uPalette3;
uniform vec4 uPalette4;
uniform float uRMS;
uniform float uBeat;
uniform float uBeatPhase;

void main() {
    vec2 uv = vTexCoord;
    float v = 0.0;
    v += sin((uv.x * 10.0 + uTime) * (1.0 + uRMS * 2.0));
    v += sin((uv.y * 10.0 + uTime) * 1.2);
    v += sin((uv.x * 10.0 + uv.y * 10.0 + uTime * 0.7) * 0.8);
    v += sin(length(uv - 0.5) * 20.0 - uTime * 2.0);
    v = v * 0.25 + 0.5;

    vec4 c;
    if (v < 0.2) c = mix(uPalette0, uPalette1, v * 5.0);
    else if (v < 0.4) c = mix(uPalette1, uPalette2, (v - 0.2) * 5.0);
    else if (v < 0.6) c = mix(uPalette2, uPalette3, (v - 0.4) * 5.0);
    else if (v < 0.8) c = mix(uPalette3, uPalette4, (v - 0.6) * 5.0);
    else c = mix(uPalette4, uPalette0, (v - 0.8) * 5.0);

    c.rgb *= 1.0 + uBeat * 0.3;
    gl_FragColor = vec4(c.rgb, 1.0);
}
)";

ShaderViewport::ShaderViewport() {
    palette_.fill(0.5f);
    pendingFragSrc_ = kDefaultFragSrc;
    glContext_.setRenderer(this);
    glContext_.setContinuousRepainting(true);
    glContext_.attachTo(*this);
}

ShaderViewport::~ShaderViewport() {
    glContext_.detach();
}

void ShaderViewport::newOpenGLContextCreated() {
    glInitialized_ = true;
    shaderNeedsRecompile_ = true;
}

void ShaderViewport::renderOpenGL() {
    using namespace juce::gl;

    auto w = getWidth();
    auto h = getHeight();
    if (w <= 0 || h <= 0) return;

    glViewport(0, 0, w, h);
    glClearColor(0.05f, 0.05f, 0.1f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    if (shaderNeedsRecompile_) {
        juce::ScopedLock sl(shaderLock_);
        delete shader_;
        shader_ = new juce::OpenGLShaderProgram(glContext_);

        std::string fragSrc = pendingFragSrc_;
        if (fragSrc.empty()) fragSrc = kDefaultFragSrc;

        if (!shader_->addVertexShader(kViewportVertSrc) ||
            !shader_->addFragmentShader(fragSrc) ||
            !shader_->link()) {
            DBG("Shader compile error: " << shader_->getLastError());
            delete shader_;
            shader_ = nullptr;
        }
        shaderNeedsRecompile_ = false;
    }

    if (!shader_) return;

    shader_->use();

    shader_->setUniform("uTime", time_);
    shader_->setUniform("uResolution", (float)w, (float)h);
    shader_->setUniform("uRMS", rms_);
    shader_->setUniform("uBeat", beat_);
    shader_->setUniform("uBeatPhase", beatPhase_);
    shader_->setUniform("uBPM", bpm_);

    for (int i = 0; i < 5; ++i) {
        juce::String name = "uPalette" + juce::String(i);
        shader_->setUniform(name.toRawUTF8(),
            palette_[i*4], palette_[i*4+1], palette_[i*4+2], palette_[i*4+3]);
    }

    // Draw fullscreen quad
    float quad[] = { -1,-1, 1,-1, -1,1, 1,-1, 1,1, -1,1 };
    GLuint vbo;
    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quad), quad, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, nullptr);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glDisableVertexAttribArray(0);
    glDeleteBuffers(1, &vbo);
}

void ShaderViewport::openGLContextClosing() {
    delete shader_;
    shader_ = nullptr;
    glInitialized_ = false;
}

void ShaderViewport::setShaderSource(const std::string& fragSrc) {
    juce::ScopedLock sl(shaderLock_);
    pendingFragSrc_ = fragSrc;
    shaderNeedsRecompile_ = true;
}

void ShaderViewport::setPalette(const std::array<float, 20>& p) { palette_ = p; }
void ShaderViewport::setTime(float t) { time_ = t; }
void ShaderViewport::setRMS(float r) { rms_ = r; }
void ShaderViewport::setBPM(float b) { bpm_ = b; }
void ShaderViewport::setBeat(float b) { beat_ = b; }
void ShaderViewport::setBeatPhase(float p) { beatPhase_ = p; }

void ShaderViewport::paint(juce::Graphics& g) {
    // OpenGL renders directly; this is fallback if GL isn't ready
    if (!glInitialized_) {
        g.fillAll(juce::Colour(0xFF0A0A1E));
        g.setColour(juce::Colours::white.withAlpha(0.3f));
        g.setFont(14.0f);
        g.drawText("Initializing OpenGL...", getLocalBounds(), juce::Justification::centred);
    }
}

void ShaderViewport::resized() {}

// ==================== Built-in shader sources ====================

std::vector<std::pair<std::string, std::string>> DVDsRGBAudioEditor::getBuiltInShaders() {
    std::vector<std::pair<std::string, std::string>> shaders;

    shaders.push_back({ "Plasma", R"(
varying vec2 vTexCoord;
uniform float uTime;
uniform vec2 uResolution;
uniform vec4 uPalette0, uPalette1, uPalette2, uPalette3, uPalette4;
uniform float uRMS, uBeat;
void main() {
    vec2 uv = vTexCoord;
    float v = 0.0;
    v += sin((uv.x * 10.0 + uTime) * (1.0 + uRMS * 2.0));
    v += sin((uv.y * 10.0 + uTime) * 1.2);
    v += sin((uv.x * 10.0 + uv.y * 10.0 + uTime * 0.7) * 0.8);
    v += sin(length(uv - 0.5) * 20.0 - uTime * 2.0);
    v = v * 0.25 + 0.5;
    vec4 c;
    if (v < 0.2) c = mix(uPalette0, uPalette1, v * 5.0);
    else if (v < 0.4) c = mix(uPalette1, uPalette2, (v - 0.2) * 5.0);
    else if (v < 0.6) c = mix(uPalette2, uPalette3, (v - 0.4) * 5.0);
    else if (v < 0.8) c = mix(uPalette3, uPalette4, (v - 0.6) * 5.0);
    else c = mix(uPalette4, uPalette0, (v - 0.8) * 5.0);
    c.rgb *= 1.0 + uBeat * 0.3;
    gl_FragColor = vec4(c.rgb, 1.0);
}
)" });

    shaders.push_back({ "Voronoi", R"(
varying vec2 vTexCoord;
uniform float uTime;
uniform vec2 uResolution;
uniform vec4 uPalette0, uPalette1, uPalette2, uPalette3, uPalette4;
uniform float uRMS, uBeat;
vec2 hash2(vec2 p) {
    p = vec2(dot(p, vec2(127.1, 311.7)), dot(p, vec2(269.5, 183.3)));
    return fract(sin(p) * 43758.5453);
}
void main() {
    vec2 uv = vTexCoord;
    float scale = 6.0 + uRMS * 4.0;
    vec2 p = uv * scale;
    vec2 ip = floor(p);
    vec2 fp = fract(p);
    float minDist = 10.0;
    vec2 closestCell = vec2(0.0);
    for (int y = -1; y <= 1; y++) {
        for (int x = -1; x <= 1; x++) {
            vec2 neighbor = vec2(float(x), float(y));
            vec2 point = hash2(ip + neighbor);
            point = 0.5 + 0.5 * sin(uTime * 0.5 + 6.2831 * point);
            float d = length(neighbor + point - fp);
            if (d < minDist) { minDist = d; closestCell = ip + neighbor; }
        }
    }
    vec4 colors[5];
    colors[0] = uPalette0; colors[1] = uPalette1; colors[2] = uPalette2;
    colors[3] = uPalette3; colors[4] = uPalette4;
    int ci = int(mod(closestCell.x + closestCell.y * 3.0, 5.0));
    if (ci < 0) ci = 0; if (ci > 4) ci = 4;
    vec4 cellColor = colors[ci];
    float edge = smoothstep(0.0, 0.05, minDist);
    gl_FragColor = mix(uPalette4 * 0.2, cellColor, edge);
    gl_FragColor.a = 1.0;
}
)" });

    shaders.push_back({ "Aurora", R"(
varying vec2 vTexCoord;
uniform float uTime;
uniform vec2 uResolution;
uniform vec4 uPalette0, uPalette1, uPalette2, uPalette3, uPalette4;
uniform float uRMS, uBeat;
void main() {
    vec2 uv = vTexCoord;
    float wave1 = sin(uv.x * 3.0 + uTime * 0.5 + uRMS * 3.0) * 0.15;
    float wave2 = sin(uv.x * 5.0 - uTime * 0.3) * 0.1;
    float wave3 = sin(uv.x * 8.0 + uTime * 0.7) * 0.05;
    float center = 0.5 + wave1 + wave2 + wave3;
    float spread = 0.15 + uRMS * 0.15;
    float aurora = smoothstep(spread, 0.0, abs(uv.y - center));
    float t = uv.x + uTime * 0.1;
    vec4 c;
    float idx = mod(t * 2.0, 4.0);
    if (idx < 1.0) c = mix(uPalette0, uPalette1, fract(idx));
    else if (idx < 2.0) c = mix(uPalette1, uPalette2, fract(idx));
    else if (idx < 3.0) c = mix(uPalette2, uPalette3, fract(idx));
    else c = mix(uPalette3, uPalette4, fract(idx));
    vec4 bg = uPalette0 * 0.08;
    gl_FragColor = mix(bg, c, aurora * (1.0 + uBeat * 0.4));
    gl_FragColor.a = 1.0;
}
)" });

    shaders.push_back({ "Tunnel", R"(
varying vec2 vTexCoord;
uniform float uTime;
uniform vec2 uResolution;
uniform vec4 uPalette0, uPalette1, uPalette2, uPalette3, uPalette4;
uniform float uRMS, uBeat;
void main() {
    vec2 uv = vTexCoord * 2.0 - 1.0;
    uv.x *= uResolution.x / uResolution.y;
    float angle = atan(uv.y, uv.x);
    float dist = length(uv);
    float tunnel = 1.0 / (dist + 0.001);
    float tx = tunnel + uTime * 0.5;
    float ty = angle / 3.14159;
    float stripe = sin(tx * 8.0) * 0.5 + 0.5;
    float ring = sin(ty * 6.0 + uTime) * 0.5 + 0.5;
    float idx = mod(stripe * 3.0 + ring * 2.0, 5.0);
    vec4 colors[5];
    colors[0] = uPalette0; colors[1] = uPalette1; colors[2] = uPalette2;
    colors[3] = uPalette3; colors[4] = uPalette4;
    int i0 = int(idx);
    if (i0 > 4) i0 = 4; if (i0 < 0) i0 = 0;
    int i1 = i0 + 1; if (i1 > 4) i1 = 0;
    vec4 c = mix(colors[i0], colors[i1], fract(idx));
    c *= smoothstep(0.0, 0.3, dist) * (1.0 + uBeat * 0.5);
    gl_FragColor = vec4(c.rgb, 1.0);
}
)" });

    shaders.push_back({ "Neon Rings", R"(
varying vec2 vTexCoord;
uniform float uTime;
uniform vec2 uResolution;
uniform vec4 uPalette0, uPalette1, uPalette2, uPalette3, uPalette4;
uniform float uRMS, uBeat;
void main() {
    vec2 uv = vTexCoord * 2.0 - 1.0;
    uv.x *= uResolution.x / uResolution.y;
    float dist = length(uv);
    float glow = 0.0;
    vec4 colors[5];
    colors[0] = uPalette0; colors[1] = uPalette1; colors[2] = uPalette2;
    colors[3] = uPalette3; colors[4] = uPalette4;
    vec4 totalColor = vec4(0.0);
    for (int i = 0; i < 5; i++) {
        float radius = 0.15 + float(i) * 0.15 + sin(uTime * 0.5 + float(i)) * 0.03;
        radius += uRMS * 0.1;
        float ring = abs(dist - radius);
        float g2 = 0.004 / (ring + 0.002);
        g2 *= 1.0 + uBeat * 0.5;
        totalColor += colors[i] * g2;
        glow += g2;
    }
    totalColor = clamp(totalColor * 0.3, 0.0, 1.0);
    gl_FragColor = vec4(totalColor.rgb, 1.0);
}
)" });

    shaders.push_back({ "Fractal Noise", R"(
varying vec2 vTexCoord;
uniform float uTime;
uniform vec2 uResolution;
uniform vec4 uPalette0, uPalette1, uPalette2, uPalette3, uPalette4;
uniform float uRMS, uBeat;
float hash(vec2 p) { return fract(sin(dot(p, vec2(127.1, 311.7))) * 43758.5453); }
float noise(vec2 p) {
    vec2 i = floor(p); vec2 f = fract(p);
    f = f * f * (3.0 - 2.0 * f);
    return mix(mix(hash(i), hash(i+vec2(1,0)), f.x),
               mix(hash(i+vec2(0,1)), hash(i+vec2(1,1)), f.x), f.y);
}
float fbm(vec2 p) {
    float v = 0.0, a = 0.5;
    for (int i = 0; i < 6; i++) { v += a * noise(p); p *= 2.0; a *= 0.5; }
    return v;
}
void main() {
    vec2 uv = vTexCoord;
    float n = fbm(uv * 4.0 + uTime * 0.3 + uRMS * 2.0);
    vec4 colors[5];
    colors[0] = uPalette0; colors[1] = uPalette1; colors[2] = uPalette2;
    colors[3] = uPalette3; colors[4] = uPalette4;
    float idx = n * 4.0;
    int i0 = int(idx); if (i0 > 4) i0 = 4; if (i0 < 0) i0 = 0;
    int i1 = i0 + 1; if (i1 > 4) i1 = 4;
    gl_FragColor = mix(colors[i0], colors[i1], fract(idx));
    gl_FragColor.a = 1.0;
}
)" });

    shaders.push_back({ "Kaleidoscope", R"(
varying vec2 vTexCoord;
uniform float uTime;
uniform vec2 uResolution;
uniform vec4 uPalette0, uPalette1, uPalette2, uPalette3, uPalette4;
uniform float uRMS, uBeat;
void main() {
    vec2 uv = vTexCoord * 2.0 - 1.0;
    uv.x *= uResolution.x / uResolution.y;
    float angle = atan(uv.y, uv.x);
    float dist = length(uv);
    float segments = 8.0;
    angle = mod(angle, 3.14159 * 2.0 / segments);
    angle = abs(angle - 3.14159 / segments);
    vec2 p = vec2(cos(angle), sin(angle)) * dist;
    p += uTime * 0.1;
    float pattern = sin(p.x * 10.0 + uRMS * 5.0) * sin(p.y * 10.0);
    pattern = pattern * 0.5 + 0.5;
    vec4 colors[5];
    colors[0] = uPalette0; colors[1] = uPalette1; colors[2] = uPalette2;
    colors[3] = uPalette3; colors[4] = uPalette4;
    int ci = int(pattern * 4.99);
    if (ci > 4) ci = 4; if (ci < 0) ci = 0;
    vec4 c = colors[ci];
    c *= (1.0 - dist * 0.4) * (1.0 + uBeat * 0.3);
    gl_FragColor = vec4(c.rgb, 1.0);
}
)" });

    shaders.push_back({ "Grid Pulse", R"(
varying vec2 vTexCoord;
uniform float uTime;
uniform vec2 uResolution;
uniform vec4 uPalette0, uPalette1, uPalette2, uPalette3, uPalette4;
uniform float uRMS, uBeat;
void main() {
    vec2 uv = vTexCoord;
    float grid = 10.0;
    vec2 cell = floor(uv * grid);
    vec2 cellUv = fract(uv * grid);
    float pulse = sin(uTime * 2.0 + cell.x * 0.5 + cell.y * 0.7) * 0.5 + 0.5;
    pulse *= 1.0 + uBeat * 0.5 + uRMS * 0.5;
    vec4 colors[5];
    colors[0] = uPalette0; colors[1] = uPalette1; colors[2] = uPalette2;
    colors[3] = uPalette3; colors[4] = uPalette4;
    int ci = int(mod(cell.x + cell.y, 5.0));
    if (ci < 0) ci = 0; if (ci > 4) ci = 4;
    float border = step(0.06, cellUv.x) * step(cellUv.x, 0.94) *
                   step(0.06, cellUv.y) * step(cellUv.y, 0.94);
    vec4 c = colors[ci] * pulse * border;
    c += uPalette4 * 0.03 * (1.0 - border);
    gl_FragColor = vec4(c.rgb, 1.0);
}
)" });

    shaders.push_back({ "Liquid", R"(
varying vec2 vTexCoord;
uniform float uTime;
uniform vec2 uResolution;
uniform vec4 uPalette0, uPalette1, uPalette2, uPalette3, uPalette4;
uniform float uRMS, uBeat;
void main() {
    vec2 uv = vTexCoord;
    vec2 p = uv * 3.0;
    float t = uTime * 0.3;
    float v = sin(p.x + sin(p.y + t) * 2.0 + uRMS * 3.0);
    v += sin(p.y * 1.5 + sin(p.x * 0.5 + t * 0.7));
    v += sin(length(p - vec2(1.5)) * 3.0 - t);
    v = v / 3.0 * 0.5 + 0.5;
    vec4 colors[5];
    colors[0] = uPalette0; colors[1] = uPalette1; colors[2] = uPalette2;
    colors[3] = uPalette3; colors[4] = uPalette4;
    float idx = v * 4.0;
    int i0 = int(idx); if (i0>4) i0=4; if (i0<0) i0=0;
    int i1 = i0+1; if (i1>4) i1=4;
    gl_FragColor = mix(colors[i0], colors[i1], fract(idx));
    gl_FragColor.rgb *= 1.0 + uBeat * 0.2;
    gl_FragColor.a = 1.0;
}
)" });

    shaders.push_back({ "Radial Burst", R"(
varying vec2 vTexCoord;
uniform float uTime;
uniform vec2 uResolution;
uniform vec4 uPalette0, uPalette1, uPalette2, uPalette3, uPalette4;
uniform float uRMS, uBeat;
void main() {
    vec2 uv = vTexCoord * 2.0 - 1.0;
    uv.x *= uResolution.x / uResolution.y;
    float angle = atan(uv.y, uv.x) / 6.28318 + 0.5;
    float dist = length(uv);
    float rays = sin(angle * 12.0 + uTime + uRMS * 5.0) * 0.5 + 0.5;
    float ring = sin(dist * 20.0 - uTime * 3.0) * 0.5 + 0.5;
    float pattern = rays * ring * (1.0 + uBeat);
    vec4 colors[5];
    colors[0] = uPalette0; colors[1] = uPalette1; colors[2] = uPalette2;
    colors[3] = uPalette3; colors[4] = uPalette4;
    int ci = int(mod(angle * 5.0 + dist * 3.0, 5.0));
    if (ci < 0) ci = 0; if (ci > 4) ci = 4;
    vec4 c = colors[ci] * pattern;
    c *= smoothstep(1.2, 0.0, dist);
    gl_FragColor = vec4(c.rgb, 1.0);
}
)" });

    return shaders;
}

// ==================== DVDsRGBAudioEditor ====================

DVDsRGBAudioEditor::DVDsRGBAudioEditor(DVDsRGBAudioProcessor& p)
    : AudioProcessorEditor(&p), processorRef_(p)
{
    setSize(1200, 800);
    setResizable(true, true);
    setResizeLimits(900, 600, 3840, 2160);

    auto& lf = getLookAndFeel();
    lf.setDefaultSansSerifTypefaceName("Segoe UI");

    // Title
    titleLabel_.setText("DVDs-RGB Audio VST Suite", juce::dontSendNotification);
    titleLabel_.setFont(juce::Font(20.0f, juce::Font::bold));
    titleLabel_.setColour(juce::Label::textColourId, juce::Colours::white);
    titleLabel_.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(titleLabel_);

    // Harmony mode selector
    harmonyLabel_.setText("Harmony:", juce::dontSendNotification);
    harmonyLabel_.setColour(juce::Label::textColourId, juce::Colours::white.withAlpha(0.7f));
    harmonyLabel_.setFont(12.0f);
    addAndMakeVisible(harmonyLabel_);

    harmonyModeBox_.addItemList({
        "Analogous", "Monochromatic", "Triad", "Complementary",
        "Split-Complementary", "Double-Split", "Square", "Compound",
        "Shades", "Custom"
    }, 1);
    harmonyModeBox_.setSelectedId(1, juce::dontSendNotification);
    harmonyModeBox_.onChange = [this]() { onHarmonyChanged(); };
    addAndMakeVisible(harmonyModeBox_);
    harmonyAttach_ = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
        processorRef_.getParameters(), "harmonyMode", harmonyModeBox_);

    // Shader selector
    shaderLabel_.setText("Shader:", juce::dontSendNotification);
    shaderLabel_.setColour(juce::Label::textColourId, juce::Colours::white.withAlpha(0.7f));
    shaderLabel_.setFont(12.0f);
    addAndMakeVisible(shaderLabel_);

    populateShaderList();
    shaderSelectBox_.onChange = [this]() { onShaderSelected(); };
    addAndMakeVisible(shaderSelectBox_);

    // Sliders
    auto setupSlider = [this](juce::Slider& s, const juce::String& suffix, double min, double max, double def) {
        s.setSliderStyle(juce::Slider::RotaryVerticalDrag);
        s.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 50, 14);
        s.setRange(min, max, 0.01);
        s.setValue(def, juce::dontSendNotification);
        s.setColour(juce::Slider::rotarySliderFillColourId, juce::Colour(0xFF6644CC));
        s.setColour(juce::Slider::textBoxTextColourId, juce::Colours::white.withAlpha(0.7f));
        s.setTextValueSuffix(suffix);
        addAndMakeVisible(s);
    };

    setupSlider(baseHueSlider_, juce::CharPointer_UTF8("\xc2\xb0"), 0, 360, 0);
    setupSlider(spreadSlider_, "", 0, 1, 0.5);
    setupSlider(satSlider_, "", 0, 2, 1.0);
    setupSlider(lightSlider_, "", 0, 2, 1.0);
    setupSlider(reactDepthSlider_, "", 0, 1, 0.75);

    hueAttach_ = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        processorRef_.getParameters(), "baseHue", baseHueSlider_);
    spreadAttach_ = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        processorRef_.getParameters(), "spread", spreadSlider_);
    satAttach_ = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        processorRef_.getParameters(), "globalSat", satSlider_);
    lightAttach_ = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        processorRef_.getParameters(), "globalLight", lightSlider_);
    reactAttach_ = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        processorRef_.getParameters(), "reactDepth", reactDepthSlider_);

    baseHueSlider_.onValueChange = [this]() {
        colorWheel_.setBaseHue((float)baseHueSlider_.getValue());
    };

    colorWheel_.onHueChanged = [this](float hue) {
        baseHueSlider_.setValue(hue, juce::sendNotificationAsync);
    };

    // Info label
    infoLabel_.setColour(juce::Label::textColourId, juce::Colours::white.withAlpha(0.5f));
    infoLabel_.setFont(10.0f);
    addAndMakeVisible(infoLabel_);

    addAndMakeVisible(colorWheel_);
    addAndMakeVisible(paletteStrip_);
    addAndMakeVisible(fftDisplay_);
    addAndMakeVisible(shaderViewport_);

    startTimerHz(30);
}

DVDsRGBAudioEditor::~DVDsRGBAudioEditor() {
    stopTimer();
}

void DVDsRGBAudioEditor::populateShaderList() {
    auto shaders = getBuiltInShaders();
    shaderSelectBox_.clear(juce::dontSendNotification);
    for (int i = 0; i < (int)shaders.size(); ++i)
        shaderSelectBox_.addItem(juce::String(shaders[i].first), i + 1);
    shaderSelectBox_.setSelectedId(1, juce::dontSendNotification);
    shaderCount_ = (int)shaders.size();
    onShaderSelected();
}

void DVDsRGBAudioEditor::onShaderSelected() {
    int idx = shaderSelectBox_.getSelectedId() - 1;
    auto shaders = getBuiltInShaders();
    if (idx >= 0 && idx < (int)shaders.size()) {
        shaderViewport_.setShaderSource(shaders[idx].second);
    }
}

void DVDsRGBAudioEditor::onHarmonyChanged() {
    // Handled by parameter attachment
}

void DVDsRGBAudioEditor::timerCallback() {
    elapsedTime_ += 1.0f / 30.0f;
    updateFromProcessor();
}

void DVDsRGBAudioEditor::updateFromProcessor() {
    auto& engine = processorRef_.getColorEngine();
    auto& paletteState = processorRef_.getPaletteState();
    auto& fft = processorRef_.getFFTAnalyzer();
    auto& bands = processorRef_.getBandSplitter();
    auto& beat = processorRef_.getBeatDetector();

    // Update color engine from current parameter values
    float hue = (float)baseHueSlider_.getValue();
    engine.setBaseHue(hue);
    int modeIdx = harmonyModeBox_.getSelectedId() - 1;
    if (modeIdx >= 0 && modeIdx < (int)dvds::HarmonyMode::Count)
        engine.setHarmonyMode(static_cast<dvds::HarmonyMode>(modeIdx));
    engine.setSpread((float)spreadSlider_.getValue() * 180.0f);
    engine.setGlobalSaturationMultiplier((float)satSlider_.getValue());
    engine.setGlobalLightnessMultiplier((float)lightSlider_.getValue());

    auto palette = engine.generatePalette();
    auto paletteRGB = engine.generatePaletteRGB();
    auto shaderPalette = engine.generateShaderPalette();

    // Feed audio color mapper
    auto& mapper = processorRef_.getAudioColorMapper();
    auto modPalette = mapper.getModulatedPalette();
    std::array<dvds::RGB, 5> modRGB;
    std::array<float, 20> modShader;
    for (int i = 0; i < 5; ++i) {
        modRGB[i] = dvds::clampRgb(dvds::okhslToRgb(modPalette[i]));
        modShader[i*4]   = modRGB[i].r;
        modShader[i*4+1] = modRGB[i].g;
        modShader[i*4+2] = modRGB[i].b;
        modShader[i*4+3] = 1.0f;
    }

    // Use modulated palette if audio is active, else base palette
    bool hasAudio = fft.getRMS() > 0.001f;
    auto& displayRGB = hasAudio ? modRGB : paletteRGB;
    auto& displayShader = hasAudio ? modShader : shaderPalette;
    auto& displayOKHsl = hasAudio ? modPalette : palette;

    colorWheel_.setPalette(displayOKHsl);
    paletteStrip_.setPalette(displayRGB);

    // Shader viewport
    shaderViewport_.setPalette(displayShader);
    shaderViewport_.setTime(elapsedTime_);
    shaderViewport_.setRMS(fft.getRMS());
    shaderViewport_.setBPM(beat.getBPM());
    shaderViewport_.setBeat(beat.isBeat() ? 1.0f : 0.0f);
    shaderViewport_.setBeatPhase(beat.getBeatPhase());

    // FFT display
    fftDisplay_.setSpectrum(fft.getMagnitudeSpectrum().data(), 128);
    fftDisplay_.setBandEnergies(bands.getAllNormalizedEnergies());
    fftDisplay_.setRMS(fft.getRMS());
    fftDisplay_.setBPM(beat.getBPM());
    fftDisplay_.setBeat(beat.isBeat());
    fftDisplay_.setPaletteColors(displayRGB);

    // Info label
    auto modeName = dvds::harmonyModeToString(engine.getConfig().mode);
    infoLabel_.setText("Mode: " + juce::String(modeName) + " | Hue: " + juce::String(int(hue))
                     + " | FPS: 30 | Shaders: " + juce::String(shaderCount_),
                     juce::dontSendNotification);

    repaint();
}

void DVDsRGBAudioEditor::paint(juce::Graphics& g) {
    g.fillAll(juce::Colour(0xFF12121E));

    // Panel borders
    g.setColour(juce::Colour(0xFF2A2A44));
    int leftW = 280;
    int rightW = 260;
    g.drawVerticalLine(leftW, 30, (float)getHeight());
    g.drawVerticalLine(getWidth() - rightW, 30, (float)getHeight());
    g.drawHorizontalLine(30, 0, (float)getWidth());

    // Panel labels
    g.setColour(juce::Colours::white.withAlpha(0.4f));
    g.setFont(10.0f);
    g.drawText("COLOR", 0, 32, leftW, 14, juce::Justification::centred);
    g.drawText("VISUAL OUTPUT", leftW, 32, getWidth() - leftW - rightW, 14, juce::Justification::centred);
    g.drawText("AUDIO ANALYSIS", getWidth() - rightW, 32, rightW, 14, juce::Justification::centred);

    // Slider labels
    int sliderY = getHeight() - 120;
    g.setFont(9.0f);
    g.setColour(juce::Colours::white.withAlpha(0.5f));
    int knobW = 56;
    int knobStartX = 0;
    const char* labels[] = { "HUE", "SPREAD", "SAT", "LIGHT", "REACT" };
    for (int i = 0; i < 5; ++i) {
        g.drawText(labels[i], knobStartX + i * knobW, sliderY - 14, knobW, 12, juce::Justification::centred);
    }
}

void DVDsRGBAudioEditor::resized() {
    auto bounds = getLocalBounds();
    int leftW = 280;
    int rightW = 260;
    int topH = 30;
    int centerW = bounds.getWidth() - leftW - rightW;

    titleLabel_.setBounds(0, 2, bounds.getWidth(), 26);

    // Left panel: color wheel + palette + controls
    int wheelSize = juce::jmin(leftW - 20, bounds.getHeight() - 280);
    wheelSize = juce::jmax(wheelSize, 100);
    colorWheel_.setBounds(10, 50, wheelSize, wheelSize);

    int paletteY = 50 + wheelSize + 8;
    paletteStrip_.setBounds(10, paletteY, leftW - 20, 50);

    // Harmony mode
    int controlY = paletteY + 58;
    harmonyLabel_.setBounds(10, controlY, 60, 20);
    harmonyModeBox_.setBounds(70, controlY, leftW - 80, 22);

    // Shader selector
    shaderLabel_.setBounds(10, controlY + 28, 60, 20);
    shaderSelectBox_.setBounds(70, controlY + 28, leftW - 80, 22);

    // Sliders
    int sliderY = controlY + 60;
    int knobW = 56;
    int knobH = 70;
    baseHueSlider_.setBounds(0, sliderY, knobW, knobH);
    spreadSlider_.setBounds(knobW, sliderY, knobW, knobH);
    satSlider_.setBounds(knobW * 2, sliderY, knobW, knobH);
    lightSlider_.setBounds(knobW * 3, sliderY, knobW, knobH);
    reactDepthSlider_.setBounds(knobW * 4, sliderY, knobW, knobH);

    infoLabel_.setBounds(10, bounds.getHeight() - 20, leftW - 20, 18);

    // Center: shader viewport
    shaderViewport_.setBounds(leftW + 2, topH + 16, centerW - 4, bounds.getHeight() - topH - 18);

    // Right panel: FFT display
    fftDisplay_.setBounds(bounds.getWidth() - rightW + 5, topH + 16, rightW - 10, bounds.getHeight() - topH - 18);
}
