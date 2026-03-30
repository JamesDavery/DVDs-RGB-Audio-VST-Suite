#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "GUI/MainLayout.h"
#include "Visuals/ShaderPipeline.h"
#include "Visuals/ISFLoader.h"
#include "Visuals/ISFRenderer.h"
#include "Visuals/GenerativeScene.h"
#include "Visuals/VideoMixer.h"
#include "Visuals/Scene3D.h"
#include "Visuals/ParticleSystem.h"
#include "Output/OutputManager.h"

class DVDsRGBAudioEditor : public juce::AudioProcessorEditor,
                            public juce::OpenGLRenderer,
                            private juce::Timer {
public:
    DVDsRGBAudioEditor(DVDsRGBAudioProcessor&);
    ~DVDsRGBAudioEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;

    // OpenGLRenderer
    void newOpenGLContextCreated() override;
    void renderOpenGL() override;
    void openGLContextClosing() override;

    void mouseDown(const juce::MouseEvent&) override;
    void mouseDrag(const juce::MouseEvent&) override;
    void mouseUp(const juce::MouseEvent&) override;
    void mouseWheelMove(const juce::MouseEvent&, const juce::MouseWheelDetails&) override;

private:
    DVDsRGBAudioProcessor& processorRef_;

    juce::OpenGLContext openGLContext_;
    dvds::MainLayout mainLayout_;
    dvds::ShaderPipeline shaderPipeline_;
    dvds::ISFLoader isfLoader_;
    dvds::ISFRenderer isfRenderer_;
    dvds::GenerativeScene generativeScene_;
    dvds::VideoMixer videoMixer_;
    dvds::Scene3D scene3D_;
    dvds::ParticleSystem particleSystem_;
    dvds::OutputManager outputManager_;

    float elapsedTime_ = 0.0f;
    float lastFrameTime_ = 0.0f;
    int frameCount_ = 0;

    void timerCallback() override;
    void updateVisualsFromProcessor();
    void setupCallbacks();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DVDsRGBAudioEditor)
};
