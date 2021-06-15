#include "shaders.h"

#include <fstream>
#include <sstream>
#include <regex>


namespace voxel {
namespace opengl {

Shader::Shader(const std::string& shader_name) :
    m_shader_name(shader_name) {
}

Shader::~Shader() {
}

std::string Shader::getName() {
    return m_shader_name;
}



ComputeShader::ComputeShader(const std::string& shader_name) : Shader(shader_name) {
    m_handle = 0;
}

ComputeShader::ComputeShader(ShaderManager& shader_manager, const std::string& shader_name, const std::string& shader_source) : Shader(shader_name) {

}

ComputeShader::~ComputeShader() {

}



ShaderManager::ShaderManager(const std::string& shader_directory, const Logger& logger) :
    m_shader_directory(shader_directory), m_logger(logger) {
}

ShaderManager::~ShaderManager() {
}

std::string ShaderManager::loadRawShaderSource(const std::string& source_name) {
    std::ifstream istream(m_shader_directory + source_name);
    if (istream.is_open()) {
        return std::string(std::istreambuf_iterator<char>(istream), std::istreambuf_iterator<char>());;
    } else {
        return "";
    }
}

std::string ShaderManager::resolveIncludesInShaderSource(const std::string& source_name, const std::string& source) {
    std::regex include_regex("#include\\s+[\"<]([A-Za-z0-9_\\-.]+)[\">]");
    std::string result = source;

    while (true) {
        auto includes_begin = std::sregex_iterator(result.begin(), result.end(), include_regex);
        auto includes_end = std::sregex_iterator();
        if (includes_begin != includes_end) {
            std::smatch match = *includes_begin;
            std::string include_source_name = match[1].str();
            std::string include_source = loadRawShaderSource(include_source_name);
            if (include_source.empty()) {
                m_logger.message(Logger::flag_error, "ShaderManager", "failed to resolve shader include %s in %s", include_source_name.data(), source_name.data());
            }
            include_source = resolveIncludesInShaderSource(include_source_name, include_source);
            result = result.substr(0, match.position()) + include_source + result.substr(match.position() + match.length());
        } else {
            break;
        }
    }

    return result;
}

std::string ShaderManager::addDefinesToShaderSource(const std::string& source, const std::vector<std::string>& defines) {
    if (defines.empty()) {
        return source;
    }

    std::stringstream defines_string;
    for (const auto& define : defines) {
        defines_string << "#define " << define << "\n";
    }

    std::regex version_regex("#version\\s+.*");
    auto versions_begin = std::sregex_iterator(source.begin(), source.end(), version_regex);
    auto versions_end = std::sregex_iterator();
    if (versions_begin != versions_end) {
        int version_end_index = versions_begin->position() + versions_begin->length();
        return source.substr(0, version_end_index) + "\n" + defines_string.str() + source.substr(version_end_index);
    } else {
        return defines_string.str() + source;
    }
}

std::string ShaderManager::loadAndParseShaderSource(const std::string& source_name, const std::vector<std::string>& defines) {
    std::string source = loadRawShaderSource(source_name);
    if (source.empty()) {
        m_logger.message(Logger::flag_error, "ShaderManager", "failed to load shader source: %s", source_name.data());
        return "";
    }
    source = resolveIncludesInShaderSource(source_name, source);
    source = addDefinesToShaderSource(source, defines);
    return source;
}

} // opengl
} // voxel