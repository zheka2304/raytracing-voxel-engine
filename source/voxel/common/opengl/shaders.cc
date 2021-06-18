#include "shaders.h"

#include <fstream>
#include <sstream>
#include <regex>
#include <json/value.h>
#include <json/reader.h>
#include <iostream>


namespace voxel {
namespace opengl {

Shader::Shader(const std::string& shader_name) :
    m_shader_name(shader_name) {
}

Shader::Shader(Shader&& other) : m_shader_name(std::move(other.m_shader_name)) {
}

Shader::~Shader() {
}

std::string Shader::getName() {
    return m_shader_name;
}

bool Shader::isValid() {
    return true;
}



ComputeShader::ComputeShader(const std::string& shader_name) : Shader(shader_name) {
    m_handle = 0;
}

ComputeShader::ComputeShader(ShaderManager& shader_manager, const std::string& shader_name, const std::string& shader_source) : Shader(shader_name) {
    GLint is_compilation_successful;
    const int compilation_info_log_size = 1024;
    GLchar compilation_info_log[compilation_info_log_size];

    // compile shader
    GLuint shader = glCreateShader(GL_COMPUTE_SHADER);
    const char* shader_source_c = shader_source.c_str();
    glShaderSource(shader, 1, &shader_source_c, nullptr);
    glCompileShader(shader);

    glGetShaderiv(shader, GL_COMPILE_STATUS, &is_compilation_successful);
    if (!is_compilation_successful) {
        glGetShaderInfoLog(shader, compilation_info_log_size, nullptr, compilation_info_log);
        shader_manager.getLogger().message(Logger::flag_error, "ShaderManager", "failed to compile compute shader %s:\n%s", shader_name.data(), compilation_info_log);
    }

    // link shader
    m_handle = glCreateProgram();
    glAttachShader(m_handle, shader);
    glLinkProgram(m_handle);

    glGetProgramiv(m_handle, GL_LINK_STATUS, &is_compilation_successful);
    if (!is_compilation_successful) {
        glGetProgramInfoLog(m_handle, compilation_info_log_size, nullptr, compilation_info_log);
        shader_manager.getLogger().message(Logger::flag_error, "ShaderManager", "failed to link compute shader %s:\n%s", shader_name.data(), compilation_info_log);
    }

    // cleanup
    glDeleteShader(shader);
}

ComputeShader::ComputeShader(ComputeShader&& other) : Shader(std::move(other)) {
    m_handle = other.m_handle;
    other.m_handle = 0;
}

ComputeShader::~ComputeShader() {

}

bool ComputeShader::isValid() {
    return m_handle != 0;
}

void ComputeShader::dispatch(int size_x, int size_y, int size_z) {
    if (m_handle != 0) {
        glUseProgram(m_handle);
        glDispatchCompute(size_x, size_y, size_z);
        glUseProgram(0);
    }
}



GraphicsShader::GraphicsShader(const std::string& shader_name) : Shader(shader_name) {
    m_handle = 0;
}

GraphicsShader::GraphicsShader(ShaderManager& shader_manager, const std::string& shader_name, const std::string& vertex_shader_source, const std::string& fragment_shader_source) : Shader(shader_name) {
    GLint is_compilation_successful;
    const int compilation_info_log_size = 1024;
    GLchar compilation_info_log[compilation_info_log_size];

    // vertex shader
    GLuint vertex_shader_handle = glCreateShader(GL_VERTEX_SHADER);
    const char* vertex_shader_source_c = vertex_shader_source.c_str();
    glShaderSource(vertex_shader_handle, 1, &vertex_shader_source_c, nullptr);
    glCompileShader(vertex_shader_handle);

    glGetShaderiv(vertex_shader_handle, GL_COMPILE_STATUS, &is_compilation_successful);
    if (!is_compilation_successful) {
        glGetShaderInfoLog(vertex_shader_handle, compilation_info_log_size, nullptr, compilation_info_log);
        shader_manager.getLogger().message(Logger::flag_error, "ShaderManager", "failed to compile vertex shader %s:\n%s", shader_name.data(), compilation_info_log);
    }

    // fragment shader
    GLuint fragment_shader_handle = glCreateShader(GL_FRAGMENT_SHADER);
    const char* fragment_shader_source_c = fragment_shader_source.c_str();
    glShaderSource(fragment_shader_handle, 1, &fragment_shader_source_c, nullptr);
    glCompileShader(fragment_shader_handle);

    glGetShaderiv(fragment_shader_handle, GL_COMPILE_STATUS, &is_compilation_successful);
    if (!is_compilation_successful) {
        glGetShaderInfoLog(fragment_shader_handle, compilation_info_log_size, nullptr, compilation_info_log);
        shader_manager.getLogger().message(Logger::flag_error, "ShaderManager", "failed to compile fragment shader %s:\n%s", shader_name.data(), compilation_info_log);
    }

    // link shaders
    m_handle = glCreateProgram();
    glAttachShader(m_handle, vertex_shader_handle);
    glAttachShader(m_handle, fragment_shader_handle);
    glLinkProgram(m_handle);

    glGetProgramiv(m_handle, GL_LINK_STATUS, &is_compilation_successful);
    if (!is_compilation_successful) {
        glGetProgramInfoLog(m_handle, compilation_info_log_size, nullptr, compilation_info_log);
        shader_manager.getLogger().message(Logger::flag_error, "ShaderManager", "failed to link graphics shader %s:\n%s", shader_name.data(), compilation_info_log);
    }

    // cleanup
    glDeleteShader(vertex_shader_handle);
    glDeleteShader(fragment_shader_handle);
}

GraphicsShader::GraphicsShader(GraphicsShader&& other) : Shader(std::move(other)) {
    m_handle = other.m_handle;
    other.m_handle = 0;
}

GraphicsShader::~GraphicsShader() {
    if (m_handle != 0) {
        glDeleteProgram(m_handle);
    }
}

bool GraphicsShader::isValid() {
    return m_handle != 0;
}

void GraphicsShader::bind() {
    glUseProgram(m_handle);
}

void GraphicsShader::unbind() {
    glUseProgram(0);
}


ShaderManager::Constant::Constant(const std::string& name, const std::variant<int, float, std::string>& value) :
        m_name(name), m_value(value) {
}

std::string ShaderManager::Constant::getName() {
    return m_name;
}

std::string ShaderManager::Constant::toString() {
    std::stringstream ss;
    if (m_value.index() == 0) {
        ss << std::get<int>(m_value);
    } else if (m_value.index() == 1) {
        ss << std::get<float>(m_value);
    } else if (m_value.index() == 2) {
        ss << std::get<std::string>(m_value);
    }
    return ss.str();
}


ShaderManager::ShaderManager(const std::string& shader_directory, const Logger& logger) :
    m_shader_directory(shader_directory), m_logger(logger) {
}

ShaderManager::~ShaderManager() {
}

Logger& ShaderManager::getLogger() {
    return m_logger;
}

std::string ShaderManager::loadRawSource(const std::string& source_name) {
    std::ifstream istream(m_shader_directory + source_name);
    if (istream.is_open()) {
        return std::string(std::istreambuf_iterator<char>(istream), std::istreambuf_iterator<char>());;
    } else {
        return "";
    }
}

std::string ShaderManager::resolveIncludesInShaderSource(const std::string& source_name, const std::string& source) {
    std::stringstream result;
    std::regex include_regex("#include\\s+[\"<]([A-Za-z0-9_\\-.]+)[\">]");

    int last_pos = 0;
    auto includes_begin = std::sregex_iterator(source.begin(), source.end(), include_regex);
    auto includes_end = std::sregex_iterator();
    for (auto it = includes_begin; it != includes_end; it++) {
        std::match_results match = *it;
        std::string include_source_name = match[1].str();
        std::string include_source = loadRawSource(include_source_name);
        if (include_source.empty()) {
            m_logger.message(Logger::flag_error, "ShaderManager", "failed to resolve shader include %s in %s", include_source_name.data(), source_name.data());
        }
        include_source = resolveIncludesInShaderSource(include_source_name, include_source);
        result << source.substr(last_pos, match.position() - last_pos) << include_source << "\n";
        last_pos = match.position() + match.length();
    }
    result << source.substr(last_pos);
    return result.str();
}

std::string ShaderManager::resolveUniqueConstantsInShaderSource(const std::string& source_name,
                                                             const std::string& source) {
    std::stringstream result;
    std::regex constant_regex("\\$\\{\\s*([A-Za-z0-9_.\\-]+)\\s*\\}");

    int last_pos = 0;
    auto constants_begin = std::sregex_iterator(source.begin(), source.end(), constant_regex);
    auto constants_end = std::sregex_iterator();
    for (auto it = constants_begin; it != constants_end; it++) {
        std::match_results match = *it;
        result << source.substr(last_pos, match.position() - last_pos) << getOrCreateConstant(match[1].str()).toString();
        last_pos = match.position() + match.length();
    }
    result << source.substr(last_pos);
    return result.str();
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
    std::string source = loadRawSource(source_name);
    if (source.empty()) {
        m_logger.message(Logger::flag_error, "ShaderManager", "failed to load shader source: %s", source_name.data());
        return "";
    }
    source = resolveIncludesInShaderSource(source_name, source);
    source = addDefinesToShaderSource(source, defines);
    source = resolveUniqueConstantsInShaderSource(source_name, source);
    return source;
}

void ShaderManager::loadDeclarationsJson(const std::string& list_name) {
    std::ifstream istream(m_shader_directory + list_name);
    if (!istream.is_open()) {
        m_logger.message(Logger::flag_error, "ShaderManager", "failed to load declarations file: %s", list_name.data());
        return;
    }

    // loading json
    std::string json_errors;
    Json::Value json_root;
    if (Json::parseFromStream(Json::CharReaderBuilder(), istream, &json_root, &json_errors)) {
        // get constants
        Json::Value constants_json = json_root["constants"];
        if (constants_json.isObject()) {
            // iterate over all constants
            for (auto it = constants_json.begin(); it != constants_json.end(); it++) {
                std::string constant_name = it.key().asString();
                Json::Value constant_value = *it;
                if (constant_value.isInt()) {
                    setConstant<int>(constant_name, constant_value.asInt());
                } else if (constant_value.isDouble()) {
                    setConstant<float>(constant_name, constant_value.asFloat());
                } else if (constant_value.isString()) {
                    setConstant<std::string>(constant_name, constant_value.asString());
                } else {
                    m_logger.message(Logger::flag_error, "ShaderManager",
                                     "failed to add shader constant %s, unsupported value type",
                                     constant_name.data());
                }
            }
        }

        // get shaders
        Json::Value shaders_json = json_root["shaders"];
        if (shaders_json.isObject()) {
            // iterate over all listed shaders
            for (auto it = shaders_json.begin(); it != shaders_json.end(); it++) {
                std::string shader_name = it.key().asString();
                Json::Value shader_description = *it;
                // for each valid shader description
                if (shader_description.isObject()) {
                    std::string shader_type = shader_description["type"].asString();

                    // get define list
                    std::vector<std::string> defines;
                    for (auto define : shader_description["defines"]) {
                        defines.emplace_back(define.asString());
                    }

                    // resolve compute shaders
                    if (shader_type == "compute") {
                        std::string source = loadAndParseShaderSource(shader_description["source"].asString(), defines);
                        if (!source.empty()) {
                            addShader<ComputeShader>(std::make_unique<ComputeShader>(*this, shader_name, source));
                        }

                    // resolve graphics shaders
                    } else if (shader_type == "graphics") {
                        std::string vertex_source = loadAndParseShaderSource(shader_description["vertex"].asString(), defines);
                        std::string fragment_source = loadAndParseShaderSource(shader_description["fragment"].asString(), defines);
                        if (!vertex_source.empty() && !fragment_source.empty()) {
                            addShader<GraphicsShader>(
                                    std::make_unique<GraphicsShader>(*this, shader_name, vertex_source,
                                                                     fragment_source));
                        }

                    // otherwise report error
                    } else {
                        m_logger.message(Logger::flag_error, "ShaderManager", "invalid shader type %s",
                                         shader_type.data());
                    }

                }
            }
        }
    } else {
        m_logger.message(Logger::flag_error, "ShaderManager",
                         "failed to load shader list: %s, failed to parse json: \n%s", list_name.data(),
                         json_errors.data());
    }
}

ShaderManager::Constant& ShaderManager::getConstant(const std::string& name) {
    auto found = m_constants_map.find(name);
    if (found == m_constants_map.end()) {
        static ShaderManager::Constant fallback_constant("fallback_constant", 0);
        return fallback_constant;
    }
    return found->second;
}

ShaderManager::Constant& ShaderManager::getOrCreateConstant(const std::string& name) {
    auto found = m_constants_map.find(name);
    if (found != m_constants_map.end()) {
        return found->second;
    }
    return m_constants_map.emplace(name, ShaderManager::Constant(name, int(m_constants_map.size() + 1))).first->second;
}

} // opengl
} // voxel