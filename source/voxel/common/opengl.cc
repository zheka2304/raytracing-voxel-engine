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

} // opengl
} // voxel
