#include "Visuals/ISFLoader.h"
#include <nlohmann/json.hpp>
#include <fstream>
#include <sstream>
#include <regex>

using json = nlohmann::json;

namespace dvds {

ISFLoader::ISFLoader() = default;

ISFFile ISFLoader::loadFromFile(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) {
        lastError_ = "Cannot open file: " + path;
        return {};
    }
    std::stringstream ss;
    ss << file.rdbuf();

    auto result = loadFromString(ss.str(), path);
    result.filePath = path;

    // Extract name from filename
    auto lastSlash = path.find_last_of("/\\");
    auto lastDot = path.find_last_of('.');
    if (lastSlash != std::string::npos && lastDot != std::string::npos)
        result.name = path.substr(lastSlash + 1, lastDot - lastSlash - 1);
    else if (lastDot != std::string::npos)
        result.name = path.substr(0, lastDot);

    return result;
}

ISFFile ISFLoader::loadFromString(const std::string& source, const std::string& name) {
    ISFFile isf;
    isf.name = name;

    auto [jsonStr, shaderBody] = splitSource(source);
    if (jsonStr.empty()) {
        lastError_ = "No JSON metadata found in ISF source";
        return isf;
    }

    isf.metadata = parseMetadata(jsonStr);
    if (!isf.metadata.valid) {
        lastError_ = isf.metadata.parseError;
        return isf;
    }

    isf.fragmentShader = convertToGLSL330(shaderBody, isf.metadata);
    return isf;
}

std::pair<std::string, std::string> ISFLoader::splitSource(const std::string& source) {
    // ISF format: JSON metadata in /* ... */ comment block, then GLSL code
    auto startComment = source.find("/*");
    auto endComment = source.find("*/");

    if (startComment == std::string::npos || endComment == std::string::npos) {
        return { "", source };
    }

    std::string jsonStr = source.substr(startComment + 2, endComment - startComment - 2);
    std::string shaderBody = source.substr(endComment + 2);

    // Trim whitespace
    while (!shaderBody.empty() && (shaderBody[0] == '\n' || shaderBody[0] == '\r'))
        shaderBody.erase(0, 1);

    return { jsonStr, shaderBody };
}

ISFInput::Type ISFLoader::stringToInputType(const std::string& s) {
    if (s == "float")    return ISFInput::Type::Float;
    if (s == "bool")     return ISFInput::Type::Bool;
    if (s == "long")     return ISFInput::Type::Long;
    if (s == "color")    return ISFInput::Type::Color;
    if (s == "point2D")  return ISFInput::Type::Point2D;
    if (s == "image")    return ISFInput::Type::Image;
    if (s == "audio")    return ISFInput::Type::Audio;
    if (s == "audioFFT") return ISFInput::Type::AudioFFT;
    if (s == "event")    return ISFInput::Type::Event;
    return ISFInput::Type::Float;
}

ISFMetadata ISFLoader::parseMetadata(const std::string& jsonStr) {
    ISFMetadata meta;
    try {
        auto j = json::parse(jsonStr);

        if (j.contains("DESCRIPTION"))
            meta.description = j["DESCRIPTION"].get<std::string>();
        if (j.contains("CREDIT"))
            meta.credit = j["CREDIT"].get<std::string>();
        if (j.contains("ISFVSN"))
            meta.isfVersion = j["ISFVSN"].get<std::string>();

        if (j.contains("CATEGORIES")) {
            for (auto& cat : j["CATEGORIES"])
                meta.categories.push_back(cat.get<std::string>());
        }

        if (j.contains("INPUTS")) {
            for (auto& inp : j["INPUTS"]) {
                ISFInput input;
                input.name = inp["NAME"].get<std::string>();
                input.type = stringToInputType(inp["TYPE"].get<std::string>());

                if (inp.contains("LABEL"))
                    input.label = inp["LABEL"].get<std::string>();
                if (inp.contains("MIN"))
                    input.minVal = inp["MIN"].get<float>();
                if (inp.contains("MAX"))
                    input.maxVal = inp["MAX"].get<float>();
                if (inp.contains("DEFAULT")) {
                    if (inp["DEFAULT"].is_number())
                        input.defaultVal = inp["DEFAULT"].get<float>();
                    else if (inp["DEFAULT"].is_array()) {
                        for (auto& v : inp["DEFAULT"])
                            input.defaultColor.push_back(v.get<float>());
                    }
                }
                if (inp.contains("LABELS")) {
                    for (auto& l : inp["LABELS"])
                        input.labels.push_back(l.get<std::string>());
                }
                if (inp.contains("VALUES")) {
                    for (auto& v : inp["VALUES"])
                        input.values.push_back(v.get<int>());
                }

                meta.inputs.push_back(input);
            }
        }

        if (j.contains("PASSES")) {
            for (auto& p : j["PASSES"]) {
                ISFPass pass;
                if (p.contains("TARGET"))
                    pass.target = p["TARGET"].get<std::string>();
                if (p.contains("PERSISTENT"))
                    pass.persistent = p["PERSISTENT"].get<bool>();
                if (p.contains("WIDTH"))
                    pass.width = p["WIDTH"].get<std::string>();
                if (p.contains("HEIGHT"))
                    pass.height = p["HEIGHT"].get<std::string>();
                if (p.contains("FLOAT"))
                    pass.floatBuffer = p["FLOAT"].get<bool>();
                meta.passes.push_back(pass);
            }
        }

        if (j.contains("PERSISTENT_BUFFERS")) {
            for (auto& buf : j["PERSISTENT_BUFFERS"])
                meta.persistentBuffers.push_back(buf.get<std::string>());
        }

        meta.valid = true;
    } catch (const json::exception& e) {
        meta.valid = false;
        meta.parseError = std::string("JSON parse error: ") + e.what();
    }
    return meta;
}

std::string ISFLoader::generateUniformDeclarations(const ISFMetadata& meta) {
    std::string decls;
    for (const auto& inp : meta.inputs) {
        switch (inp.type) {
            case ISFInput::Type::Float:
            case ISFInput::Type::Event:
                decls += "uniform float " + inp.name + ";\n";
                break;
            case ISFInput::Type::Bool:
                decls += "uniform bool " + inp.name + ";\n";
                break;
            case ISFInput::Type::Long:
                decls += "uniform int " + inp.name + ";\n";
                break;
            case ISFInput::Type::Color:
                decls += "uniform vec4 " + inp.name + ";\n";
                break;
            case ISFInput::Type::Point2D:
                decls += "uniform vec2 " + inp.name + ";\n";
                break;
            case ISFInput::Type::Image:
            case ISFInput::Type::Audio:
            case ISFInput::Type::AudioFFT:
                decls += "uniform sampler2D " + inp.name + ";\n";
                decls += "uniform vec2 _" + inp.name + "_imgRect;\n";
                break;
        }
    }
    // Persistent buffer uniforms
    for (const auto& buf : meta.persistentBuffers) {
        decls += "uniform sampler2D " + buf + ";\n";
        decls += "uniform vec2 _" + buf + "_imgRect;\n";
    }
    return decls;
}

std::string ISFLoader::generateISFCompatPreamble(const ISFMetadata& meta) {
    return R"(
uniform int PASSINDEX;
uniform vec2 RENDERSIZE;
uniform float TIME;
uniform float TIMEDELTA;
uniform int FRAMEINDEX;
uniform vec4 DATE;

vec2 isf_FragNormCoord;

vec4 IMG_NORM_PIXEL(sampler2D tex, vec2 coord) {
    return texture(tex, coord);
}
vec4 IMG_PIXEL(sampler2D tex, vec2 coord) {
    return texelFetch(tex, ivec2(coord), 0);
}
vec2 IMG_SIZE(sampler2D tex) {
    return vec2(textureSize(tex, 0));
}
)";
}

std::string ISFLoader::convertToGLSL330(const std::string& isfFrag, const ISFMetadata& meta) {
    std::string result = "#version 330 core\n";
    result += "out vec4 gl_FragColorOut;\n";
    result += "in vec2 vTexCoord;\n";

    // DVDs-RGB palette injection
    result += "uniform vec4 uPalette[5];\n";
    result += "uniform float uRMS;\n";
    result += "uniform float uBPM;\n";

    result += generateUniformDeclarations(meta);
    result += generateISFCompatPreamble(meta);
    result += "\n";

    // Replace gl_FragColor with our output
    std::string body = isfFrag;
    // Simple regex replacement for gl_FragColor
    std::regex fragColorRegex(R"(gl_FragColor)");
    body = std::regex_replace(body, fragColorRegex, "gl_FragColorOut");

    // Set up isf_FragNormCoord at start of main
    std::regex mainRegex(R"(void\s+main\s*\(\s*\)\s*\{)");
    body = std::regex_replace(body, mainRegex,
        "void main() {\n    isf_FragNormCoord = vTexCoord;\n");

    result += body;
    return result;
}

} // namespace dvds
