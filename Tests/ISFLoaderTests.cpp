#include <gtest/gtest.h>
#include "Visuals/ISFLoader.h"

using namespace dvds;

static const char* kSimpleISF = R"(/*{
    "DESCRIPTION": "Test shader",
    "CREDIT": "Test",
    "ISFVSN": "2",
    "CATEGORIES": ["Generator"],
    "INPUTS": [
        {
            "NAME": "brightness",
            "TYPE": "float",
            "MIN": 0.0,
            "MAX": 1.0,
            "DEFAULT": 0.5
        },
        {
            "NAME": "color1",
            "TYPE": "color",
            "DEFAULT": [1.0, 0.0, 0.0, 1.0]
        },
        {
            "NAME": "waveImage",
            "TYPE": "audio"
        },
        {
            "NAME": "fftImage",
            "TYPE": "audioFFT",
            "MAX": 256
        }
    ]
}*/

void main() {
    vec2 loc = isf_FragNormCoord;
    vec4 wave = IMG_NORM_PIXEL(waveImage, vec2(loc.x, 0.0));
    gl_FragColor = color1 * brightness * wave.r;
}
)";

static const char* kMultiPassISF = R"(/*{
    "DESCRIPTION": "Multi-pass test",
    "ISFVSN": "2",
    "INPUTS": [
        { "NAME": "gain", "TYPE": "float", "DEFAULT": 1.0 }
    ],
    "PASSES": [
        { "TARGET": "bufferA", "PERSISTENT": true },
        {}
    ],
    "PERSISTENT_BUFFERS": ["bufferA"]
}*/

void main() {
    gl_FragColor = vec4(1.0);
}
)";

TEST(ISFLoaderTest, SplitSource) {
    auto [json, shader] = ISFLoader::splitSource(kSimpleISF);
    EXPECT_FALSE(json.empty());
    EXPECT_FALSE(shader.empty());
    EXPECT_TRUE(shader.find("void main()") != std::string::npos);
}

TEST(ISFLoaderTest, ParseMetadata) {
    auto [json, shader] = ISFLoader::splitSource(kSimpleISF);
    auto meta = ISFLoader::parseMetadata(json);

    EXPECT_TRUE(meta.valid);
    EXPECT_EQ(meta.description, "Test shader");
    EXPECT_EQ(meta.credit, "Test");
    EXPECT_EQ(meta.isfVersion, "2");
    EXPECT_EQ(meta.categories.size(), 1u);
    EXPECT_EQ(meta.categories[0], "Generator");
    EXPECT_EQ(meta.inputs.size(), 4u);
}

TEST(ISFLoaderTest, ParseInputTypes) {
    auto [json, shader] = ISFLoader::splitSource(kSimpleISF);
    auto meta = ISFLoader::parseMetadata(json);

    EXPECT_EQ(meta.inputs[0].name, "brightness");
    EXPECT_EQ(meta.inputs[0].type, ISFInput::Type::Float);
    EXPECT_NEAR(meta.inputs[0].defaultVal, 0.5f, 0.001f);
    EXPECT_NEAR(meta.inputs[0].minVal, 0.0f, 0.001f);
    EXPECT_NEAR(meta.inputs[0].maxVal, 1.0f, 0.001f);

    EXPECT_EQ(meta.inputs[1].name, "color1");
    EXPECT_EQ(meta.inputs[1].type, ISFInput::Type::Color);
    EXPECT_EQ(meta.inputs[1].defaultColor.size(), 4u);

    EXPECT_EQ(meta.inputs[2].name, "waveImage");
    EXPECT_EQ(meta.inputs[2].type, ISFInput::Type::Audio);

    EXPECT_EQ(meta.inputs[3].name, "fftImage");
    EXPECT_EQ(meta.inputs[3].type, ISFInput::Type::AudioFFT);
}

TEST(ISFLoaderTest, ParseMultiPass) {
    auto [json, shader] = ISFLoader::splitSource(kMultiPassISF);
    auto meta = ISFLoader::parseMetadata(json);

    EXPECT_TRUE(meta.valid);
    EXPECT_EQ(meta.passes.size(), 2u);
    EXPECT_EQ(meta.passes[0].target, "bufferA");
    EXPECT_TRUE(meta.passes[0].persistent);
    EXPECT_EQ(meta.persistentBuffers.size(), 1u);
    EXPECT_EQ(meta.persistentBuffers[0], "bufferA");
}

TEST(ISFLoaderTest, ConvertToGLSL330) {
    auto [json, shader] = ISFLoader::splitSource(kSimpleISF);
    auto meta = ISFLoader::parseMetadata(json);
    auto glsl = ISFLoader::convertToGLSL330(shader, meta);

    EXPECT_TRUE(glsl.find("#version 330 core") != std::string::npos);
    EXPECT_TRUE(glsl.find("uniform float brightness") != std::string::npos);
    EXPECT_TRUE(glsl.find("uniform vec4 color1") != std::string::npos);
    EXPECT_TRUE(glsl.find("uniform sampler2D waveImage") != std::string::npos);
    EXPECT_TRUE(glsl.find("uniform sampler2D fftImage") != std::string::npos);
    EXPECT_TRUE(glsl.find("uniform vec4 uPalette[5]") != std::string::npos);
    EXPECT_TRUE(glsl.find("gl_FragColorOut") != std::string::npos);
    EXPECT_TRUE(glsl.find("isf_FragNormCoord") != std::string::npos);
}

TEST(ISFLoaderTest, LoadFromString) {
    ISFLoader loader;
    auto isf = loader.loadFromString(kSimpleISF, "test");

    EXPECT_TRUE(isf.metadata.valid);
    EXPECT_EQ(isf.name, "test");
    EXPECT_FALSE(isf.fragmentShader.empty());
}

TEST(ISFLoaderTest, InvalidJSON) {
    ISFLoader loader;
    auto isf = loader.loadFromString("/*{ invalid json }*/\nvoid main(){}", "bad");
    EXPECT_FALSE(isf.metadata.valid);
}

TEST(ISFLoaderTest, NoMetadata) {
    ISFLoader loader;
    auto isf = loader.loadFromString("void main() { gl_FragColor = vec4(1.0); }", "none");
    EXPECT_FALSE(isf.metadata.valid);
}
