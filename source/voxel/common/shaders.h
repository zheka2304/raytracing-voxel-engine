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
    // shaders can be moved, but not copied, as any other opengl entity with integer handle
    Shader(const Shader& other) = delete;
    Shader(Shader&& other);
    virtual ~Shader();

    std::string getName();
    virtual bool isValid();
};

class ComputeShader : public Shader {
private:
    GLuint m_handle;

public:
    ComputeShader(const std::string& shader_name);
    ComputeShader(ShaderManager& shader_manager, const std::string& shader_name, const std::string& shader_source);
    ComputeShader(const ComputeShader& other) = delete;
    ComputeShader(ComputeShader&& other);
    virtual ~ComputeShader();

    bool isValid() override;
};

class GraphicsShader : public Shader {
private:
    GLuint m_handle;

public:
    GraphicsShader(const std::string& shader_name);
    GraphicsShader(ShaderManager& shader_manager, const std::string& shader_name, const std::string& vertex_shader_source, const std::string& fragment_shader_source);
    GraphicsShader(const GraphicsShader& other) = delete;
    GraphicsShader(GraphicsShader&& other);
    virtual ~GraphicsShader();

    bool isValid() override;
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
    std::string loadRawSource(const std::string& source_name);
    std::string resolveIncludesInShaderSource(const std::string& source_name, const std::string& source);
    std::string addDefinesToShaderSource(const std::string& source, const std::vector<std::string>& defines);

public:
    std::string loadAndParseShaderSource(const std::string& source_name, const std::vector<std::string>& defines);
    void loadShaderList(const std::string& list_name);

    template<typename T>
    bool addShader(std::unique_ptr<Shader> shader_ptr) {
        // invalid shaders are silently ignored, because all errors must be already reported
        if (!shader_ptr->isValid()) {
            return false;
        }

        // check if name is already occupied
        std::string shader_name = shader_ptr->getName();
        if (m_shader_map.find(shader_name) != m_shader_map.end()) {
            m_logger.message(Logger::flag_error, "ShaderManager", "adding shader %s failed: shader with this name already exists", shader_name.data());
            return false;
        }

        // add shader and its type to map and report it
        m_shader_map.emplace(shader_name, std::pair<std::type_index, std::unique_ptr<Shader>>(std::type_index(typeid(T)), std::move(shader_ptr)));
        m_logger.message(Logger::flag_info, "ShaderManager", "successfully added shader %s", shader_name.data());
        return true;
    }

    template<typename T>
    T& getShader(const std::string& shader_name) {
        auto shader_type = std::type_index(typeid(T));

        // find shader in map by name and check it type
        auto found = m_shader_map.find(shader_name);
        if (found != m_shader_map.end()) {
            if (found->second.first == shader_type) {
                return *static_cast<T*>(found->second.second.get());
            } else {
                m_logger.message(Logger::flag_error, "ShaderManager", "getting shader %s with incorrect type", shader_name.data());
            }
        } else {
            m_logger.message(Logger::flag_error, "ShaderManager", "shader %s not found", shader_name.data());
        }

        // if required shader was not found, fallback to default shader for this type
        auto found_fallback = m_shader_fallback_map.find(shader_type);
        if (found_fallback != m_shader_fallback_map.end()) {
            return *static_cast<T*>(found_fallback->second.get());
        } else {
            // if no default shader exists, create it with default shader constructor
            // !! each shader class must have (std::string name) constructor to make this work

            // emplace shader_type and unique_ptr of newly created fallback shader into map,
            // from returning pair<iterator, bool> get iterator, get unique_ptr,
            // get pointer, cast it to T* and then return reference, fuck c++
            return *static_cast<T*>(m_shader_fallback_map.emplace(shader_type, std::make_unique<T>(shader_name)).first->second.get());
        }
    }

};

} // opengl
} // voxel

#endif //VOXEL_ENGINE_SHADERS_H
