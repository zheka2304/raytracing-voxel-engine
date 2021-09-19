#include "buffer.h"


namespace voxel {
namespace opengl {


Buffer::Buffer(GLuint buffer_type) {
    m_buffer_type = buffer_type;
    glCreateBuffers(1, &m_handle);
}

Buffer::Buffer(Buffer&& other) {
    m_buffer_type = other.m_buffer_type;
    m_handle = other.m_handle;
    m_allocated_size = other.m_allocated_size;
    other.m_handle = 0;
    other.m_allocated_size = 0;
}

Buffer::~Buffer() {
    if (m_handle != 0) {
        glDeleteBuffers(1, &m_handle);
    }
}

GLuint Buffer::getType() {
    return m_buffer_type;
}

GLuint Buffer::getHandle() {
    return m_handle;
}

GLuint Buffer::getAllocatedSize() {
    return m_allocated_size;
}

bool Buffer::isValid() {
    return m_handle != 0;
}

void Buffer::setData(size_t size, void* data, GLuint access_type, bool unbind) {
    glBindBuffer(m_buffer_type, m_handle);
    glBufferData(m_buffer_type, size, data, access_type);
    if (unbind) {
        glBindBuffer(m_buffer_type, 0);
    }
    m_allocated_size = size;
}

void Buffer::preallocate(size_t size, GLuint access_type) {
    setData(size, nullptr, access_type);
}

void Buffer::clear(GLuint internal_format, GLuint format, GLuint type, void* data) {
    if (m_allocated_size > 0) {
        GLuint zero = 0;
        if (data == nullptr) data = &zero;
        glBindBuffer(m_buffer_type, m_handle);
        glClearBufferData(m_buffer_type, internal_format, format, type, data);
        glBindBuffer(m_buffer_type, 0);
    }
}

void Buffer::bindBuffer() {
    if (m_handle != 0) {
        glBindBuffer(m_buffer_type, m_handle);
    }
}

void Buffer::unbindBuffer() {
    if (m_handle != 0) {
        glBindBuffer(m_buffer_type, 0);
    }
}


ShaderStorageBuffer::ShaderStorageBuffer(const std::string& constant_name) : Buffer(GL_SHADER_STORAGE_BUFFER),
                                                                             m_constant_name(constant_name) {
}

void ShaderStorageBuffer::bind(ShaderManager& shader_manager) {
    if (!isValid()) {
        return;
    }
    if (!m_constant_ref.isInitialized()) {
        m_constant_ref.initialize(shader_manager, m_constant_name);
    }
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, m_constant_ref.get(), getHandle());
}

std::string ShaderStorageBuffer::getConstantName() {
    return m_constant_name;
}


} // opengl
} // voxel