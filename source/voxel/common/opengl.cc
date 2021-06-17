#include "opengl.h"


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

void Buffer::setData(size_t size, void* data, GLuint access_type) {
    glBindBuffer(m_buffer_type, m_handle);
    glBufferData(m_buffer_type, size, data, access_type);
    glBindBuffer(m_buffer_type, 0);
    m_allocated_size = size;
}

void Buffer::preallocate(size_t size, GLuint access_type) {
    setData(size, nullptr, access_type);
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


Texture::Texture(int width, int height, GLuint internal_format, GLuint format, GLuint data_type, void* data) {
    m_width = width;
    m_height = height;
    m_internal_format = internal_format;
    m_format = format;
    m_data_type = data_type;

    glGenTextures(1, &m_handle);
    glBindTexture(GL_TEXTURE_2D, m_handle);
    glTexImage2D(GL_TEXTURE_2D, 0, m_internal_format, m_width, m_height, 0, m_format, m_data_type, data);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glBindTexture(GL_TEXTURE_2D, 0);
}

Texture::Texture(Texture&& other) {
    m_handle = other.m_handle;
    m_width = other.m_width;
    m_height = other.m_height;
    m_internal_format = other.m_internal_format;
    m_format = other.m_format;
    m_data_type = other.m_data_type;
    other.m_handle = 0;
}

Texture::~Texture() {
    if (m_handle != 0) {
        glDeleteTextures(1, &m_handle);
    }
}

GLuint Texture::getHandle() {
    return m_handle;
}

bool Texture::isValid() {
    return m_handle != 0;
}


} // opengl
} // voxel
