#ifndef VOXEL_ENGINE_SHADERS_H
#define VOXEL_ENGINE_SHADERS_H

#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <typeindex>
#include <variant>

#include <glad/glad.h>
#include "voxel/common/logger.h"
#include "voxel/common/math/vec.h"
#include "voxel/common/utils/thread_guard.h"


namespace voxel {
namespace opengl {

class ShaderManager;

class Shader {
private:
    ShaderManager* m_shader_manager = nullptr;
    std::string m_shader_name;

public:
    Shader(std::string const& shader_name);
    // shaders can be moved, but not copied, as any other opengl entity with integer handle
    Shader(const Shader& other) = delete;
    Shader(Shader&& other);
    virtual ~Shader();

    std::string getName();
    ShaderManager& getShaderManager();
    virtual bool isValid();

private:
    void setShaderManager(ShaderManager* shader_manager);
    friend ShaderManager;
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
    void dispatch(int size_x = 1, int size_y = 1, int size_z = 1);
    void dispatch(math::Vec3i size);
    void dispatchForTexture(math::Vec3i texture_size, math::Vec3i compute_group_size);
    void dispatchForTexture(math::Vec3i texture_size);
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
    void bind();
    void unbind();
    GLuint getUniform(const char* name);
};


class ShaderManager {
public:
    // represents constant, that is inserted in shader code
    class Constant {
        std::string m_name;
        std::variant<int, float, std::string> m_value;

    public:
        explicit Constant(const std::string& name, const std::variant<int, float, std::string>& value);
        Constant(const Constant& other) = default;
        Constant(Constant&& other) = default;

        std::string getName();
        std::string toString();

        template<typename T>
        inline T getValue() {
            return std::get<T>(m_value);
        }
    };

private:
    std::string m_shader_directory;
    Logger m_logger;

    std::unordered_map<std::string, std::pair<std::type_index, std::unique_ptr<Shader>>> m_shader_map;
    std::unordered_map<std::type_index, std::unique_ptr<Shader>> m_shader_fallback_map;

    std::unordered_map<std::string, Constant> m_constants_map;

public:
    ShaderManager(const std::string& shader_directory, const Logger& logger);
    ShaderManager(const ShaderManager& other) = delete;
    ShaderManager(ShaderManager&& other) = delete;
    ~ShaderManager();

private:
    std::string loadRawSource(const std::string& source_name);
    std::string resolveIncludesInShaderSource(const std::string& source_name, const std::string& source);
    std::string resolveUniqueConstantsInShaderSource(const std::string& source_name, const std::string& source);
    std::string addDefinesToShaderSource(const std::string& source, const std::vector<std::string>& defines);

public:
    Logger& getLogger();

    std::string loadAndParseShaderSource(const std::string& source_name, const std::vector<std::string>& defines);
    void loadDeclarationsJson(const std::string& list_name);

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
        shader_ptr->setShaderManager(this);
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

    Constant& getConstant(const std::string& name);
    Constant& getOrCreateConstant(const std::string& name);

    template<typename T>
    Constant& setConstant(const std::string& name, T value) {
        auto found = m_constants_map.find(name);
        if (found != m_constants_map.end()) {
            m_logger.message(Logger::flag_error, "ShaderManager", "constant %s is already defined and cannot be changed", name.data());
            return found->second;
        }

        // emplace new constant and return reference to it
        return m_constants_map.emplace(name, Constant(name, value)).first->second;
    }
};


/*
 * Reference to shader, that is usually used in combination with with macro VOXEL_ENGINE_SHADER_REF.
 */
template<typename T>
class ShaderRef {
    bool m_initialized = false;
    T* m_shader = nullptr;

public:
    inline bool isInitialized() {
        return m_initialized;
    }

    inline void initialize(ShaderManager& shader_manager, const std::string& shader_name) {
        m_shader = &shader_manager.getShader<T>(shader_name);
        m_initialized = true;
    }

    inline T* operator-> () {
        return m_shader;
    }

    inline T& operator* () {
        return *m_shader;
    }
};

/*
 * Reference to shader constant, that is usually used in combination with with macro VOXEL_ENGINE_SHADER_CONSTANT.
 */
template<typename T>
class ShaderConstantRef {
    bool m_initialized = false;
    T m_value;

public:
    inline bool isInitialized() {
        return m_initialized;
    }

    inline void initialize(ShaderManager& shader_manager, const std::string& shader_constant_name) {
        m_value = shader_manager.getConstant(shader_constant_name).getValue<T>();
        m_initialized = true;
    }

    inline T get() {
        return m_value;
    }
};


} // opengl
} // voxel


// this macro is used for fast access to shaders
// it is caching values for each thread separately, because there is normally one shader manager per context

#define VOXEL_ENGINE_SHADER_REF(T, variable_name, shader_manager, shader_name) \
    static thread_local voxel::opengl::ShaderRef<T> variable_name;             \
    if (!variable_name.isInitialized()) {                                      \
        variable_name.initialize(shader_manager, shader_name);                 \
    }


// this macro is used for fast access to shader constants,
// it is caching values for each thread separately, because there is normally one shader manager per context, for
// more complex scenario avoid using it
// note: for cleaner code, do not use it for strings, because this will make it not trivially destructible

#define VOXEL_ENGINE_SHADER_CONSTANT(T, variable_name, shader_manager, shader_constant_name) \
    static thread_local voxel::opengl::ShaderConstantRef<T> variable_name;                   \
    if (!variable_name.isInitialized()) {                                                    \
        variable_name.initialize(shader_manager, shader_constant_name);                      \
    }


// simple macro for caching uniform handles, must always receive same shader, because it is not checking it
#define VOXEL_ENGINE_SHADER_UNIFORM(variable_name, shader, uniform_name) \
    static thread_local GLuint variable_name = -1;                       \
    if (variable_name == -1) {                                           \
        variable_name = (shader).getUniform(uniform_name);               \
    }

#endif //VOXEL_ENGINE_SHADERS_H
