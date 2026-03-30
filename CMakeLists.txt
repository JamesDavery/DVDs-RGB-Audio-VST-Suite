cmake_minimum_required(VERSION 3.22)
project(DVDs-RGB-Audio-VST-Suite VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)

# -- JUCE --
FetchContent_Declare(
    JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG        7.0.12
    GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(JUCE)

# -- nlohmann/json (ISF parsing) --
FetchContent_Declare(
    json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG        v3.11.3
    GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(json)

# -- tinygltf (3D model loading) --
FetchContent_Declare(
    tinygltf
    GIT_REPOSITORY https://github.com/syoyo/tinygltf.git
    GIT_TAG        v2.9.3
    GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(tinygltf)

juce_add_plugin(DVDsRGBAudioVST
    COMPANY_NAME                "DVDs-RGB"
    IS_SYNTH                    TRUE
    NEEDS_MIDI_INPUT            TRUE
    NEEDS_MIDI_OUTPUT           FALSE
    IS_MIDI_EFFECT              FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS TRUE
    COPY_PLUGIN_AFTER_BUILD     FALSE
    PLUGIN_MANUFACTURER_CODE    DRGB
    PLUGIN_CODE                 Drvs
    FORMATS                     VST3 AU Standalone
    PRODUCT_NAME                "DVDs-RGB Audio VST Suite"
)

target_sources(DVDsRGBAudioVST
    PRIVATE
        Source/PluginProcessor.cpp
        Source/PluginEditor.cpp
        Source/Core/OKLab.cpp
        Source/Core/ColorTheoryEngine.cpp
        Source/Core/PaletteState.cpp
        Source/Audio/FFTAnalyzer.cpp
        Source/Audio/BandSplitter.cpp
        Source/Audio/EnvelopeFollower.cpp
        Source/Audio/BeatDetector.cpp
        Source/Audio/AudioColorMapper.cpp
        Source/Visuals/ShaderPipeline.cpp
        Source/Visuals/ISFLoader.cpp
        Source/Visuals/ISFRenderer.cpp
        Source/Visuals/GenerativeScene.cpp
        Source/Visuals/VideoPlayer.cpp
        Source/Visuals/VideoMixer.cpp
        Source/Visuals/Scene3D.cpp
        Source/Visuals/ParticleSystem.cpp
        Source/Output/SpoutSender.cpp
        Source/Output/SyphonSender.cpp
        Source/Output/NDISender.cpp
        Source/Output/OutputManager.cpp
        Source/GUI/ColorWheelComponent.cpp
        Source/GUI/PaletteStrip.cpp
        Source/GUI/ShaderBrowser.cpp
        Source/GUI/MixerPanel.cpp
        Source/GUI/AudioReactPanel.cpp
        Source/GUI/Scene3DPanel.cpp
        Source/GUI/MainLayout.cpp
)

juce_generate_juce_header(DVDsRGBAudioVST)

target_compile_definitions(DVDsRGBAudioVST
    PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0
        JUCE_DISPLAY_SPLASH_SCREEN=0
)

target_include_directories(DVDsRGBAudioVST
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/Source
)

target_link_libraries(DVDsRGBAudioVST
    PRIVATE
        juce::juce_audio_utils
        juce::juce_audio_processors
        juce::juce_dsp
        juce::juce_opengl
        juce::juce_gui_extra
        nlohmann_json::nlohmann_json
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)

# -- Tests --
option(BUILD_TESTS "Build unit tests" OFF)
if(BUILD_TESTS)
    enable_testing()
    FetchContent_Declare(
        googletest
        GIT_REPOSITORY https://github.com/google/googletest.git
        GIT_TAG        v1.14.0
        GIT_SHALLOW    TRUE
    )
    FetchContent_MakeAvailable(googletest)

    add_executable(DVDsRGBTests
        Tests/ColorTheoryTests.cpp
        Tests/FFTAnalyzerTests.cpp
        Tests/ISFLoaderTests.cpp
        Tests/AudioColorMapperTests.cpp
        Source/Core/OKLab.cpp
        Source/Core/ColorTheoryEngine.cpp
        Source/Core/PaletteState.cpp
        Source/Audio/FFTAnalyzer.cpp
        Source/Audio/BandSplitter.cpp
        Source/Audio/EnvelopeFollower.cpp
        Source/Audio/BeatDetector.cpp
        Source/Audio/AudioColorMapper.cpp
        Source/Visuals/ISFLoader.cpp
    )

    target_include_directories(DVDsRGBTests
        PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}/Source
    )

    target_link_libraries(DVDsRGBTests
        PRIVATE
            gtest_main
            juce::juce_dsp
            juce::juce_core
            nlohmann_json::nlohmann_json
    )

    include(GoogleTest)
    gtest_discover_tests(DVDsRGBTests)
endif()
