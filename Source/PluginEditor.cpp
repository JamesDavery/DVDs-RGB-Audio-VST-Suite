#include "PluginEditor.h"
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

juce::Colour DVDsRGBAudioEditor::toJuceColour(const dvds::RGB& c) const {
    return juce::Colour::fromFloatRGBA(
        juce::jlimit(0.0f, 1.0f, c.r),
        juce::jlimit(0.0f, 1.0f, c.g),
        juce::jlimit(0.0f, 1.0f, c.b), 1.0f);
}

DVDsRGBAudioEditor::DVDsRGBAudioEditor(DVDsRGBAudioProcessor& p)
    : AudioProcessorEditor(&p), processorRef_(p)
{
    setSize(1100, 750);
    setResizable(true, true);
    setResizeLimits(800, 550, 3840, 2160);

    // Init palette
    dvds::OKHsl def = { 0.0f, 0.8f, 0.65f };
    currentPalette_.fill(def);
    for (auto& c : currentPaletteRGB_) c = dvds::RGB(0.8f, 0.2f, 0.2f);
    spectrum_.resize(128, 0.0f);

    // Title
    titleLabel_.setText("DVDs-RGB Audio VST Suite", juce::dontSendNotification);
    titleLabel_.setFont(juce::Font(18.0f, juce::Font::bold));
    titleLabel_.setColour(juce::Label::textColourId, juce::Colours::white);
    titleLabel_.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(titleLabel_);

    // Harmony mode
    harmonyModeBox_.addItemList({
        "Analogous", "Monochromatic", "Triad", "Complementary",
        "Split-Complementary", "Double-Split", "Square", "Compound",
        "Shades", "Custom"
    }, 1);
    harmonyModeBox_.setSelectedId(1, juce::dontSendNotification);
    addAndMakeVisible(harmonyModeBox_);
    harmonyAttach_ = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
        processorRef_.getParameters(), "harmonyMode", harmonyModeBox_);

    // Shader select
    shaderSelectBox_.addItemList({
        "Plasma", "Voronoi", "Aurora", "Tunnel", "Neon Rings",
        "Fractal Noise", "Kaleidoscope", "Grid Pulse", "Liquid", "Radial Burst"
    }, 1);
    shaderSelectBox_.setSelectedId(1, juce::dontSendNotification);
    shaderSelectBox_.onChange = [this]() { activeShader_ = shaderSelectBox_.getSelectedId() - 1; };
    addAndMakeVisible(shaderSelectBox_);

    // Sliders
    auto setupKnob = [this](juce::Slider& s, double min, double max, double def) {
        s.setSliderStyle(juce::Slider::RotaryVerticalDrag);
        s.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
        s.setRange(min, max, 0.01);
        s.setValue(def, juce::dontSendNotification);
        s.setColour(juce::Slider::rotarySliderFillColourId, juce::Colour(0xFF8855EE));
        s.setColour(juce::Slider::rotarySliderOutlineColourId, juce::Colour(0xFF333355));
        s.setColour(juce::Slider::thumbColourId, juce::Colours::white);
        addAndMakeVisible(s);
    };

    setupKnob(baseHueSlider_, 0, 360, 0);
    setupKnob(spreadSlider_, 0, 1, 0.5);
    setupKnob(satSlider_, 0, 2, 1.0);
    setupKnob(lightSlider_, 0, 2, 1.0);
    setupKnob(reactDepthSlider_, 0, 1, 0.75);

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
        wheelBaseHue_ = (float)baseHueSlider_.getValue();
    };

    startTimerHz(30);
}

DVDsRGBAudioEditor::~DVDsRGBAudioEditor() {
    stopTimer();
}

void DVDsRGBAudioEditor::timerCallback() {
    elapsedTime_ += 1.0f / 30.0f;
    updateFromProcessor();
    repaint();
}

void DVDsRGBAudioEditor::updateFromProcessor() {
    auto& engine = processorRef_.getColorEngine();
    auto& fft = processorRef_.getFFTAnalyzer();
    auto& bands = processorRef_.getBandSplitter();
    auto& beat = processorRef_.getBeatDetector();

    wheelBaseHue_ = (float)baseHueSlider_.getValue();
    engine.setBaseHue(wheelBaseHue_);
    int modeIdx = harmonyModeBox_.getSelectedId() - 1;
    if (modeIdx >= 0 && modeIdx < (int)dvds::HarmonyMode::Count)
        engine.setHarmonyMode(static_cast<dvds::HarmonyMode>(modeIdx));
    engine.setSpread((float)spreadSlider_.getValue() * 180.0f);
    engine.setGlobalSaturationMultiplier((float)satSlider_.getValue());
    engine.setGlobalLightnessMultiplier((float)lightSlider_.getValue());

    currentPalette_ = engine.generatePalette();
    currentPaletteRGB_ = engine.generatePaletteRGB();

    // Audio data
    rms_ = fft.getRMS();
    bpm_ = beat.getBPM();
    beat_ = beat.isBeat();
    bandEnergies_ = bands.getAllNormalizedEnergies();

    const auto& mag = fft.getMagnitudeSpectrum();
    spectrum_.resize(128);
    int step = dvds::FFTAnalyzer::kBinCount / 128;
    for (int i = 0; i < 128; ++i)
        spectrum_[i] = mag[i * step];
}

// ==================== Drawing ====================

void DVDsRGBAudioEditor::paint(juce::Graphics& g) {
    g.fillAll(juce::Colour(0xFF10101C));

    int w = getWidth();
    int h = getHeight();
    int leftW = 270;
    int rightW = 240;
    int topBar = 32;
    int centerW = w - leftW - rightW;

    // Top bar
    g.setColour(juce::Colour(0xFF181828));
    g.fillRect(0, 0, w, topBar);

    // Panel dividers
    g.setColour(juce::Colour(0xFF2A2A44));
    g.drawVerticalLine(leftW, (float)topBar, (float)h);
    g.drawVerticalLine(w - rightW, (float)topBar, (float)h);

    // Panel headers
    g.setColour(juce::Colour(0xFF222238));
    g.fillRect(0, topBar, leftW, 18);
    g.fillRect(leftW + 1, topBar, centerW - 1, 18);
    g.fillRect(w - rightW + 1, topBar, rightW - 1, 18);
    g.setColour(juce::Colours::white.withAlpha(0.5f));
    g.setFont(10.0f);
    g.drawText("COLOR THEORY", 0, topBar, leftW, 18, juce::Justification::centred);
    g.drawText("SHADER PREVIEW", leftW, topBar, centerW, 18, juce::Justification::centred);
    g.drawText("AUDIO ANALYSIS", w - rightW, topBar, rightW, 18, juce::Justification::centred);

    int contentY = topBar + 20;
    int contentH = h - contentY;

    // Left: Color wheel + palette + knob labels
    auto wheelArea = juce::Rectangle<float>(10.0f, (float)contentY + 5, (float)leftW - 20, (float)leftW - 20);
    if (wheelArea.getHeight() > contentH * 0.45f)
        wheelArea.setHeight(contentH * 0.45f);
    wheelArea.setWidth(wheelArea.getHeight()); // keep square
    wheelArea.setX((leftW - wheelArea.getWidth()) * 0.5f);
    wheelBounds_ = wheelArea;
    drawColorWheel(g, wheelArea);

    float paletteY = wheelArea.getBottom() + 8;
    drawPaletteStrip(g, juce::Rectangle<float>(8, paletteY, (float)leftW - 16, 45));

    // Knob labels
    float knobLabelY = paletteY + 50;
    g.setColour(juce::Colours::white.withAlpha(0.4f));
    g.setFont(9.0f);
    int knobW = 50;
    const char* labels[] = { "HUE", "SPREAD", "SAT", "LIGHT", "REACT" };
    for (int i = 0; i < 5; ++i) {
        int x = 5 + i * knobW;
        g.drawText(labels[i], x, (int)knobLabelY, knobW, 12, juce::Justification::centred);
    }

    // Harmony / shader labels
    float comboY = knobLabelY + 75;
    g.setColour(juce::Colours::white.withAlpha(0.5f));
    g.setFont(10.0f);
    g.drawText("Harmony:", 8, (int)comboY, 60, 20, juce::Justification::centredLeft);
    g.drawText("Shader:", 8, (int)comboY + 28, 60, 20, juce::Justification::centredLeft);

    // Mode info
    g.setColour(juce::Colours::white.withAlpha(0.3f));
    g.setFont(9.0f);
    auto modeName = dvds::harmonyModeToString(processorRef_.getColorEngine().getConfig().mode);
    g.drawText("Mode: " + juce::String(modeName) + " | Hue: " + juce::String(int(wheelBaseHue_)),
               8, h - 18, leftW - 16, 16, juce::Justification::centredLeft);

    // Center: Shader preview
    drawShaderPreview(g, juce::Rectangle<float>((float)leftW + 4, (float)contentY + 2,
                                                 (float)centerW - 8, (float)contentH - 4));

    // Right: FFT + bands
    float fftAreaTop = (float)contentY + 4;
    float fftH = contentH * 0.45f;
    drawFFTPanel(g, juce::Rectangle<float>((float)(w - rightW + 6), fftAreaTop,
                                            (float)rightW - 12, fftH));
    drawBandMeters(g, juce::Rectangle<float>((float)(w - rightW + 6), fftAreaTop + fftH + 8,
                                              (float)rightW - 12, contentH * 0.35f));

    // BPM / RMS / Beat
    float infoY = fftAreaTop + fftH + 8 + contentH * 0.35f + 8;
    g.setColour(juce::Colours::white.withAlpha(0.6f));
    g.setFont(12.0f);
    g.drawText("RMS: " + juce::String(rms_, 3), w - rightW + 10, (int)infoY, rightW / 2, 20,
               juce::Justification::centredLeft);
    g.drawText("BPM: " + juce::String(int(bpm_)), w - rightW / 2, (int)infoY, rightW / 2 - 10, 20,
               juce::Justification::centredRight);

    // Beat indicator
    float beatCircleX = (float)(w - rightW / 2);
    float beatCircleY = infoY + 28;
    g.setColour(beat_ ? juce::Colours::white : juce::Colour(0xFF333355));
    g.fillEllipse(beatCircleX - 12, beatCircleY - 12, 24, 24);
    if (beat_) {
        g.setColour(juce::Colours::white.withAlpha(0.3f));
        g.fillEllipse(beatCircleX - 18, beatCircleY - 18, 36, 36);
    }
    g.setColour(juce::Colours::white.withAlpha(0.4f));
    g.setFont(9.0f);
    g.drawText("BEAT", beatCircleX - 20, beatCircleY + 16, 40, 12, juce::Justification::centred);
}

void DVDsRGBAudioEditor::drawColorWheel(juce::Graphics& g, juce::Rectangle<float> area) {
    float cx = area.getCentreX();
    float cy = area.getCentreY();
    float outerR = area.getWidth() * 0.5f;
    float innerR = outerR * 0.62f;
    float midR = (outerR + innerR) * 0.5f;

    // Draw hue ring
    for (int deg = 0; deg < 360; deg += 2) {
        float a1 = (deg - 90) * (float)M_PI / 180.0f;
        float a2 = (deg + 2 - 90) * (float)M_PI / 180.0f;

        dvds::RGB rgb = dvds::clampRgb(dvds::okhslToRgb({ (float)deg, 0.85f, 0.65f }));
        g.setColour(toJuceColour(rgb));

        juce::Path seg;
        seg.addCentredArc(cx, cy, outerR, outerR, 0, a1, a2, true);
        seg.addCentredArc(cx, cy, innerR, innerR, 0, a2, a1, false);
        seg.closeSubPath();
        g.fillPath(seg);
    }

    // Inner dark fill
    g.setColour(juce::Colour(0xFF10101C));
    g.fillEllipse(cx - innerR + 1, cy - innerR + 1, (innerR - 1) * 2, (innerR - 1) * 2);

    // Base hue line
    float hRad = (wheelBaseHue_ - 90.0f) * (float)M_PI / 180.0f;
    g.setColour(juce::Colours::white);
    g.drawLine(cx + innerR * std::cos(hRad), cy + innerR * std::sin(hRad),
               cx + outerR * std::cos(hRad), cy + outerR * std::sin(hRad), 2.5f);

    // Palette handles
    for (int i = 0; i < 5; ++i) {
        float h = currentPalette_[i].h;
        float hr = (h - 90.0f) * (float)M_PI / 180.0f;
        float hx = cx + midR * std::cos(hr);
        float hy = cy + midR * std::sin(hr);
        float sz = (i == 0) ? 9.0f : 6.0f;

        g.setColour(toJuceColour(currentPaletteRGB_[i]));
        g.fillEllipse(hx - sz, hy - sz, sz * 2, sz * 2);
        g.setColour(juce::Colours::white.withAlpha(0.9f));
        g.drawEllipse(hx - sz, hy - sz, sz * 2, sz * 2, 1.5f);
    }

    // Center text
    g.setColour(juce::Colours::white.withAlpha(0.7f));
    g.setFont(15.0f);
    g.drawText(juce::String(int(wheelBaseHue_)) + juce::CharPointer_UTF8("\xc2\xb0"),
               cx - 25, cy - 10, 50, 20, juce::Justification::centred);
}

void DVDsRGBAudioEditor::drawPaletteStrip(juce::Graphics& g, juce::Rectangle<float> area) {
    float gap = 3.0f;
    float sw = (area.getWidth() - gap * 4.0f) / 5.0f;

    for (int i = 0; i < 5; ++i) {
        float x = area.getX() + i * (sw + gap);
        auto col = toJuceColour(currentPaletteRGB_[i]);
        g.setColour(col);
        g.fillRoundedRectangle(x, area.getY(), sw, area.getHeight() - 14, 4.0f);

        // Hex label
        g.setColour(col.getBrightness() > 0.5f ? juce::Colours::black.withAlpha(0.7f)
                                                 : juce::Colours::white.withAlpha(0.7f));
        g.setFont(9.0f);
        juce::String hex = "#" + juce::String::toHexString((int)currentPaletteRGB_[i].toHex())
                                    .paddedLeft('0', 6).toUpperCase();
        g.drawText(hex, x, area.getBottom() - 13, sw, 12, juce::Justification::centred);
    }
}

void DVDsRGBAudioEditor::drawShaderPreview(juce::Graphics& g, juce::Rectangle<float> area) {
    // Software-rendered shader preview using palette colors
    int pw = juce::jmin((int)area.getWidth(), 400);
    int ph = juce::jmin((int)area.getHeight(), 300);
    if (pw < 10 || ph < 10) return;

    juce::Image img(juce::Image::ARGB, pw, ph, true);
    {
        juce::Image::BitmapData bmp(img, juce::Image::BitmapData::writeOnly);
        float t = elapsedTime_;

        for (int y = 0; y < ph; ++y) {
            for (int x = 0; x < pw; ++x) {
                float u = (float)x / (float)pw;
                float v = (float)y / (float)ph;
                float val = 0.0f;

                switch (activeShader_) {
                    case 0: // Plasma
                        val = std::sin((u * 10.0f + t) * (1.0f + rms_ * 2.0f));
                        val += std::sin((v * 10.0f + t) * 1.2f);
                        val += std::sin((u * 10.0f + v * 10.0f + t * 0.7f) * 0.8f);
                        val += std::sin(std::sqrt((u-0.5f)*(u-0.5f)+(v-0.5f)*(v-0.5f)) * 20.0f - t * 2.0f);
                        val = val * 0.25f + 0.5f;
                        break;
                    case 1: // Voronoi (simplified)
                    {
                        float minD = 10.0f;
                        int ci2 = 0;
                        for (int k = 0; k < 12; ++k) {
                            float px = std::fmod(std::sin((float)k * 127.1f) * 43758.5f, 1.0f) * 0.5f + 0.25f;
                            float py = std::fmod(std::sin((float)k * 311.7f) * 43758.5f, 1.0f) * 0.5f + 0.25f;
                            px += 0.15f * std::sin(t * 0.5f + (float)k);
                            py += 0.15f * std::cos(t * 0.3f + (float)k * 1.3f);
                            float d = std::sqrt((u-px)*(u-px) + (v-py)*(v-py));
                            if (d < minD) { minD = d; ci2 = k % 5; }
                        }
                        auto c = currentPaletteRGB_[ci2];
                        float edge = juce::jlimit(0.0f, 1.0f, minD * 8.0f);
                        bmp.setPixelColour(x, y, toJuceColour(c).withMultipliedBrightness(edge));
                        continue;
                    }
                    case 2: // Aurora
                    {
                        float wave = std::sin(u * 3.0f + t * 0.5f + rms_ * 3.0f) * 0.15f;
                        wave += std::sin(u * 5.0f - t * 0.3f) * 0.1f;
                        float center = 0.5f + wave;
                        float spread = 0.15f + rms_ * 0.1f;
                        float aurora = juce::jlimit(0.0f, 1.0f, 1.0f - std::abs(v - center) / spread);
                        aurora *= aurora;
                        float idx = std::fmod(u * 2.0f + t * 0.1f, 1.0f) * 4.0f;
                        int i0 = juce::jlimit(0, 4, (int)idx);
                        int i1 = juce::jlimit(0, 4, i0 + 1);
                        float f = idx - (int)idx;
                        auto c0 = currentPaletteRGB_[i0];
                        auto c1 = currentPaletteRGB_[i1];
                        float r2 = c0.r + (c1.r - c0.r) * f;
                        float g2 = c0.g + (c1.g - c0.g) * f;
                        float b2 = c0.b + (c1.b - c0.b) * f;
                        bmp.setPixelColour(x, y, juce::Colour::fromFloatRGBA(
                            juce::jlimit(0.0f,1.0f,r2 * aurora),
                            juce::jlimit(0.0f,1.0f,g2 * aurora),
                            juce::jlimit(0.0f,1.0f,b2 * aurora), 1.0f));
                        continue;
                    }
                    case 3: // Tunnel
                    {
                        float ux = u * 2.0f - 1.0f, uy = v * 2.0f - 1.0f;
                        float dist = std::sqrt(ux*ux + uy*uy);
                        float angle = std::atan2(uy, ux);
                        float tunnel = 1.0f / (dist + 0.01f);
                        float idx = std::fmod(std::abs(std::sin(tunnel + t * 0.5f) * 2.5f + angle), 5.0f);
                        val = idx / 5.0f;
                        break;
                    }
                    case 4: // Neon Rings
                    {
                        float ux = u * 2.0f - 1.0f, uy = v * 2.0f - 1.0f;
                        float dist = std::sqrt(ux*ux + uy*uy);
                        float glow = 0.0f;
                        int bestRing = 0;
                        for (int r = 0; r < 5; ++r) {
                            float radius = 0.15f + r * 0.15f + std::sin(t * 0.5f + r) * 0.03f;
                            float ring = std::abs(dist - radius);
                            float g2 = 0.004f / (ring + 0.002f);
                            if (g2 > glow) bestRing = r;
                            glow += g2;
                        }
                        glow = juce::jlimit(0.0f, 3.0f, glow) * 0.3f;
                        auto c = currentPaletteRGB_[bestRing];
                        bmp.setPixelColour(x, y, juce::Colour::fromFloatRGBA(
                            juce::jlimit(0.0f,1.0f,c.r*glow),
                            juce::jlimit(0.0f,1.0f,c.g*glow),
                            juce::jlimit(0.0f,1.0f,c.b*glow), 1.0f));
                        continue;
                    }
                    case 5: // Fractal noise
                    {
                        float n = 0.0f, amp = 0.5f;
                        float px = u * 4.0f + t * 0.3f, py = v * 4.0f;
                        for (int oct = 0; oct < 4; ++oct) {
                            float ix = std::floor(px), iy = std::floor(py);
                            float fx = px - ix, fy = py - iy;
                            float h00 = std::fmod(std::sin(ix * 127.1f + iy * 311.7f) * 43758.5f, 1.0f);
                            float h10 = std::fmod(std::sin((ix+1) * 127.1f + iy * 311.7f) * 43758.5f, 1.0f);
                            float h01 = std::fmod(std::sin(ix * 127.1f + (iy+1) * 311.7f) * 43758.5f, 1.0f);
                            float h11 = std::fmod(std::sin((ix+1) * 127.1f + (iy+1) * 311.7f) * 43758.5f, 1.0f);
                            if (h00 < 0) h00 = -h00; if (h10 < 0) h10 = -h10;
                            if (h01 < 0) h01 = -h01; if (h11 < 0) h11 = -h11;
                            fx = fx*fx*(3.0f-2.0f*fx); fy = fy*fy*(3.0f-2.0f*fy);
                            n += amp * ((h00*(1-fx)+h10*fx)*(1-fy) + (h01*(1-fx)+h11*fx)*fy);
                            px *= 2.0f; py *= 2.0f; amp *= 0.5f;
                        }
                        val = juce::jlimit(0.0f, 1.0f, n);
                        break;
                    }
                    case 6: // Kaleidoscope
                    {
                        float ux = u * 2 - 1, uy = v * 2 - 1;
                        float angle = std::atan2(uy, ux);
                        float dist = std::sqrt(ux*ux+uy*uy);
                        float segs = 8.0f;
                        angle = std::fmod(angle + (float)M_PI, (float)(2*M_PI / segs));
                        angle = std::abs(angle - (float)(M_PI / segs));
                        float px = std::cos(angle) * dist + t * 0.1f;
                        float py = std::sin(angle) * dist;
                        val = std::sin(px * 10.0f + rms_ * 5.0f) * std::sin(py * 10.0f);
                        val = val * 0.5f + 0.5f;
                        val *= (1.0f - dist * 0.4f);
                        break;
                    }
                    case 7: // Grid Pulse
                    {
                        float grid = 10.0f;
                        float cellX = std::floor(u * grid);
                        float cellY = std::floor(v * grid);
                        float cu = std::fmod(u * grid, 1.0f);
                        float cv = std::fmod(v * grid, 1.0f);
                        float pulse = std::sin(t * 2.0f + cellX * 0.5f + cellY * 0.7f) * 0.5f + 0.5f;
                        int ci = (int)std::fmod(std::abs(cellX + cellY), 5.0f);
                        float border = (cu > 0.06f && cu < 0.94f && cv > 0.06f && cv < 0.94f) ? 1.0f : 0.0f;
                        auto c = currentPaletteRGB_[ci];
                        float bright = pulse * border;
                        bmp.setPixelColour(x, y, juce::Colour::fromFloatRGBA(
                            juce::jlimit(0.0f,1.0f,c.r*bright),
                            juce::jlimit(0.0f,1.0f,c.g*bright),
                            juce::jlimit(0.0f,1.0f,c.b*bright), 1.0f));
                        continue;
                    }
                    case 8: // Liquid
                    {
                        float px = u * 3.0f, py = v * 3.0f;
                        float lv = std::sin(px + std::sin(py + t * 0.3f) * 2.0f + rms_ * 3.0f);
                        lv += std::sin(py * 1.5f + std::sin(px * 0.5f + t * 0.21f));
                        lv = lv / 2.0f * 0.5f + 0.5f;
                        val = juce::jlimit(0.0f, 1.0f, lv);
                        break;
                    }
                    case 9: // Radial Burst
                    {
                        float ux = u*2-1, uy = v*2-1;
                        float angle = std::atan2(uy, ux) / (2.0f * (float)M_PI) + 0.5f;
                        float dist = std::sqrt(ux*ux+uy*uy);
                        float rays = std::sin(angle * 12.0f * (float)(2*M_PI) + t + rms_ * 5.0f) * 0.5f + 0.5f;
                        float ring = std::sin(dist * 20.0f - t * 3.0f) * 0.5f + 0.5f;
                        float pattern = rays * ring;
                        pattern *= juce::jlimit(0.0f, 1.0f, 1.2f - dist);
                        int ci = (int)std::fmod(std::abs(angle * 5.0f + dist * 3.0f), 5.0f);
                        auto c = currentPaletteRGB_[ci];
                        bmp.setPixelColour(x, y, juce::Colour::fromFloatRGBA(
                            juce::jlimit(0.0f,1.0f,c.r*pattern),
                            juce::jlimit(0.0f,1.0f,c.g*pattern),
                            juce::jlimit(0.0f,1.0f,c.b*pattern), 1.0f));
                        continue;
                    }
                    default:
                        val = u;
                        break;
                }

                // Map val to palette gradient
                val = juce::jlimit(0.0f, 1.0f, val);
                float idx = val * 4.0f;
                int i0 = juce::jlimit(0, 4, (int)idx);
                int i1 = juce::jlimit(0, 4, i0 + 1);
                float f = idx - (float)i0;
                auto c0 = currentPaletteRGB_[i0];
                auto c1 = currentPaletteRGB_[i1];
                bmp.setPixelColour(x, y, juce::Colour::fromFloatRGBA(
                    juce::jlimit(0.0f, 1.0f, c0.r + (c1.r - c0.r) * f),
                    juce::jlimit(0.0f, 1.0f, c0.g + (c1.g - c0.g) * f),
                    juce::jlimit(0.0f, 1.0f, c0.b + (c1.b - c0.b) * f), 1.0f));
            }
        }
    }

    g.drawImage(img, area, juce::RectanglePlacement::stretchToFit);

    // Shader name overlay
    auto shaderNames = shaderSelectBox_.getText();
    g.setColour(juce::Colours::white.withAlpha(0.5f));
    g.setFont(11.0f);
    g.drawText(shaderNames, area.getX() + 8, area.getY() + 4, 200, 16, juce::Justification::centredLeft);
}

void DVDsRGBAudioEditor::drawFFTPanel(juce::Graphics& g, juce::Rectangle<float> area) {
    g.setColour(juce::Colour(0xFF0A0A18));
    g.fillRoundedRectangle(area, 4.0f);

    if (spectrum_.empty()) return;

    int numBars = juce::jmin((int)spectrum_.size(), (int)(area.getWidth() / 2));
    float barW = area.getWidth() / (float)numBars;

    for (int i = 0; i < numBars; ++i) {
        float mag = juce::jlimit(0.0f, 1.0f, spectrum_[i] * 6.0f);
        float barH = mag * area.getHeight() * 0.9f;
        float t = (float)i / (float)numBars;
        int ci = juce::jlimit(0, 4, (int)(t * 4.99f));
        auto col = toJuceColour(currentPaletteRGB_[ci]);
        g.setColour(col.withAlpha(0.75f));
        g.fillRect(area.getX() + i * barW, area.getBottom() - barH, barW - 1, barH);
    }
}

void DVDsRGBAudioEditor::drawBandMeters(juce::Graphics& g, juce::Rectangle<float> area) {
    g.setColour(juce::Colour(0xFF0A0A18));
    g.fillRoundedRectangle(area, 4.0f);

    const char* names[] = { "SUB", "LOW", "MID", "HI-M", "PRES", "AIR" };
    float meterW = (area.getWidth() - 10) / 6.0f;

    for (int i = 0; i < 6; ++i) {
        float x = area.getX() + 5 + i * meterW;
        float energy = juce::jlimit(0.0f, 1.0f, bandEnergies_[i]);
        float fillH = energy * (area.getHeight() - 18);

        g.setColour(juce::Colour(0xFF1A1A33));
        g.fillRoundedRectangle(x, area.getY() + 2, meterW - 3, area.getHeight() - 18, 2.0f);

        int ci = juce::jlimit(0, 4, i);
        auto col = toJuceColour(currentPaletteRGB_[ci]);
        if (col.getBrightness() < 0.1f) col = juce::Colour(0xFF4466AA);
        g.setColour(col);
        g.fillRoundedRectangle(x, area.getY() + 2 + (area.getHeight() - 18 - fillH),
                               meterW - 3, fillH, 2.0f);

        g.setColour(juce::Colours::white.withAlpha(0.5f));
        g.setFont(8.0f);
        g.drawText(names[i], x, area.getBottom() - 14, meterW - 3, 12, juce::Justification::centred);
    }
}

// ==================== Mouse handling ====================

float DVDsRGBAudioEditor::pointToHue(float x, float y, juce::Rectangle<float> area) const {
    float cx = area.getCentreX();
    float cy = area.getCentreY();
    float angle = std::atan2(y - cy, x - cx);
    float deg = angle * 180.0f / (float)M_PI + 90.0f;
    return dvds::wrapHue(deg);
}

void DVDsRGBAudioEditor::mouseDown(const juce::MouseEvent& e) {
    if (wheelBounds_.contains((float)e.x, (float)e.y)) {
        isWheelDrag_ = true;
        float hue = pointToHue((float)e.x, (float)e.y, wheelBounds_);
        baseHueSlider_.setValue(hue, juce::sendNotificationSync);
    }
}

void DVDsRGBAudioEditor::mouseDrag(const juce::MouseEvent& e) {
    if (isWheelDrag_) {
        float hue = pointToHue((float)e.x, (float)e.y, wheelBounds_);
        baseHueSlider_.setValue(hue, juce::sendNotificationSync);
    }
}

// ==================== Layout ====================

void DVDsRGBAudioEditor::resized() {
    int w = getWidth();
    int leftW = 270;
    int topBar = 32;
    float contentY = topBar + 20.0f;

    titleLabel_.setBounds(0, 2, w, 28);

    // Wheel area calculation for knob placement
    float wheelH = juce::jmin((float)leftW - 20, (getHeight() - contentY) * 0.45f);
    float paletteY = contentY + 5 + wheelH + 8;
    float knobLabelY = paletteY + 50;
    float knobY = knobLabelY + 12;
    int knobW = 50;
    int knobH = 50;

    baseHueSlider_.setBounds(5, (int)knobY, knobW, knobH);
    spreadSlider_.setBounds(5 + knobW, (int)knobY, knobW, knobH);
    satSlider_.setBounds(5 + knobW * 2, (int)knobY, knobW, knobH);
    lightSlider_.setBounds(5 + knobW * 3, (int)knobY, knobW, knobH);
    reactDepthSlider_.setBounds(5 + knobW * 4, (int)knobY, knobW, knobH);

    float comboY = knobY + knobH + 8;
    harmonyModeBox_.setBounds(68, (int)comboY, leftW - 78, 22);
    shaderSelectBox_.setBounds(68, (int)comboY + 28, leftW - 78, 22);
}
