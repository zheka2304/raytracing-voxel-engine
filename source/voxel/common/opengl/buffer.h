#ifndef VOXEL_ENGINE_BUFFER_H
#define VOXEL_ENGINE_BUFFER_H

#include <glad/glad.h>
#include "voxel/common/opengl/shaders.h"


namespace voxel {
namespace opengl {

// Universal base class for opengl buffers
class Buffer {
private:
    // buffer handle, 0 if buffer is invalid
    GLuint m_handle;

    // buffer type, defined in the constructor
    GLuint m_buffer_type;

    // holds currently allocated size
    size_t m_allocated_size = 0;

public:
    Buffer(GLuint buffer_type);
    Buffer(const Buffer&) = delete;
    Buffer(Buffer&&);
    ~Buffer();

    GLuint getType();
    GLuint getHandle();
    GLuint getAllocatedSize();
    bool isValid();

    void setData(size_t size, void* data, GLuint access_type, bool unbind = true);
    void preallocate(size_t size, GLuint access_type);

    void bindBuffer();
    void unbindBuffer();
};


// represents buffer, that is bound to specified shader constant name
// only one implementation should exist per shader manager
class ShaderStorageBuffer : public Buffer {
    std::string m_constant_name;
    ShaderConstantRef<int> m_constant_ref;

public:
    ShaderStorageBuffer(const std::string& constant_name);
    void bind(ShaderManager& shader_manager);
    std::string getConstantName();
};


template<typename T>
class ShaderUniformBuffer : public ShaderStorageBuffer {
    T m_data;
    T* m_data_ptr = nullptr;

public:
    ShaderUniformBuffer(const std::string& constant_name) : ShaderStorageBuffer(constant_name) {
    }

    ShaderUniformBuffer(const std::string& constant_name, const T& data): ShaderStorageBuffer(constant_name), m_data(data) {
    }

    ShaderUniformBuffer(const std::string& constant_name, T* data): ShaderStorageBuffer(constant_name), m_data_ptr(data) {
    }

    void bindUniform(ShaderManager& shader_manager) {
        setData(sizeof(T), m_data_ptr ? m_data_ptr : &m_data, GL_STATIC_READ);
        ShaderStorageBuffer::bind(shader_manager);
    }

    inline T* operator->() {
        return m_data_ptr ? m_data_ptr : &m_data;
    }

    inline T& operator*() {
        if (m_data_ptr) {
            return *m_data_ptr;
        } else {
            return m_data;
        }
    }
};

} // opengl
} // voxel

#endif //VOXEL_ENGINE_BUFFER_H
