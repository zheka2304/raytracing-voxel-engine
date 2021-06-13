#ifndef VOXEL_ENGINE_OPENGL_H
#define VOXEL_ENGINE_OPENGL_H

#include <glad/glad.h>

namespace voxel {
namespace opengl {

// Universal class for opengl buffers
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
    Buffer(Buffer const&) = delete;
    Buffer(Buffer&&);
    ~Buffer();

    GLuint getType();
    GLuint getHandle();
    GLuint getAllocatedSize();
    bool isValid();

    void setData(size_t size, void* data, GLuint access_type);
    void preallocate(size_t size, GLuint access_type);
};


} // opengl
} // voxel

#endif //VOXEL_ENGINE_OPENGL_H
