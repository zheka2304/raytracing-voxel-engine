#ifndef VOXEL_ENGINE_SHADERS_H
#define VOXEL_ENGINE_SHADERS_H

#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <typeindex>

#include <glad/glad.h>
#include "voxel/common/logger.h"

namespace voxel {
namespace opengl {

class ShaderManager;

class Shader {
private:
    std::string m_shader_name;

public:
    Shader(std::string const& shader_name);
    virtual ~Shader();

    std::string getName();
};

class ComputeShader : public Shader {
private:
    GLuint m_handle;

public:
    ComputeShader(const std::string& shader_name);
    ComputeShader(ShaderManager& shader_manager, const std::string& shader_name, const std::string& shader_source);
    virtual ~ComputeShader();
};


class ShaderManager {
private:
    std::string m_shader_directory;
    Logger m_logger;

    std::unordered_map<std::string, std::pair<std::type_index, std::unique_ptr<Shader>>> m_shader_map;
    std::unordered_map<std::type_index, std::unique_ptr<Shader>> m_shader_fallback_map;

public:
    ShaderManager(const std::string& shader_directory, const Logger& logger);
    ~ShaderManager();

private:
    std::string loadRawShaderSource(const std::string& source_name);
    std::string resolveIncludesInShaderSource(const std::string& source_name, const std::string& source);
    std::string addDefinesToShaderSource(const std::string& source, const std::vector<std::string>& defines);

public:
    std::string loadAndParseShaderSource(const std::string& source_name, const std::vector<std::string>& defines);

    template<typename T>
    bool addShader(std::unique_ptr<T> shader_ptr) {
        std::string shader_name = shader_ptr->getName();
        if (m_shader_map.find(shader_name) != m_shader_map.end()) {
            m_logger.message(Logger::flag_error, "ShaderManager", "adding shader %s failed: shader with this name already exists", shader_name.data());
            return false;
        }

        m_shader_map[shader_name] = std::pair<std::type_index, std::unique_ptr<Shader>>(std::type_index(typeid(T)), shader_ptr);
        return true;
    }

    template<typename T>
    T& getShader(const std::string& shader_name) {
        auto shader_type = std::type_index(typeid(T));

        auto found = m_shader_map.find(shader_name);
        if (found != m_shader_map.end()) {
            if (found->second.first == shader_type) {
                return *found->second.second.get();
            } else {
                m_logger.message(Logger::flag_error, "ShaderManager", "getting shader %s with incorrect type", shader_name.data());
            }
        } else {
            m_logger.message(Logger::flag_error, "ShaderManager", "shader %s not found", shader_name.data());
        }

        auto found_fallback = m_shader_fallback_map.find(shader_type);
        if (found_fallback != m_shader_fallback_map.end()) {
            return *found_fallback->second.get();
        } else {
            // each shader type class must have (std::string name) constructor
            // this will create invalid shader to fallback to
            return *(m_shader_fallback_map[shader_type] = std::make_unique<T>(shader_name, false)).get();
        }
    }

};

} // opengl
} // voxel

#endif //VOXEL_ENGINE_SHADERS_H
