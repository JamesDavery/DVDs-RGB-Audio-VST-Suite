#pragma once
#include <string>
#include <vector>
#include <variant>
#include <optional>

namespace dvds {

struct ISFInput {
    enum class Type {
        Float, Bool, Long, Color, Point2D, Image, Audio, AudioFFT, Event
    };

    std::string name;
    std::string label;
    Type type;
    float minVal = 0.0f;
    float maxVal = 1.0f;
    float defaultVal = 0.0f;
    std::vector<float> defaultColor; // for color type
    std::vector<std::string> labels; // for long type (menu)
    std::vector<int> values;         // for long type
    int maxSamples = 0;              // for audio/audioFFT
};

struct ISFPass {
    std::string target;
    bool persistent = false;
    std::optional<std::string> width;
    std::optional<std::string> height;
    bool floatBuffer = false;
};

struct ISFMetadata {
    std::string description;
    std::string credit;
    std::string isfVersion;
    std::vector<std::string> categories;
    std::vector<ISFInput> inputs;
    std::vector<ISFPass> passes;
    std::vector<std::string> persistentBuffers;
    bool valid = false;
    std::string parseError;
};

struct ISFFile {
    std::string filePath;
    std::string name;
    ISFMetadata metadata;
    std::string vertexShader;
    std::string fragmentShader;
};

class ISFLoader {
public:
    ISFLoader();

    ISFFile loadFromFile(const std::string& path);
    ISFFile loadFromString(const std::string& source, const std::string& name = "");

    static ISFMetadata parseMetadata(const std::string& jsonStr);
    static std::pair<std::string, std::string> splitSource(const std::string& source);
    static std::string convertToGLSL330(const std::string& isfFrag, const ISFMetadata& meta);

    const std::string& getLastError() const { return lastError_; }

private:
    std::string lastError_;

    static ISFInput::Type stringToInputType(const std::string& s);
    static std::string generateUniformDeclarations(const ISFMetadata& meta);
    static std::string generateISFCompatPreamble(const ISFMetadata& meta);
};

} // namespace dvds
