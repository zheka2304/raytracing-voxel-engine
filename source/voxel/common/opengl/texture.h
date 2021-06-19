#ifndef VOXEL_ENGINE_TEXTURE_H
#define VOXEL_ENGINE_TEXTURE_H

#include <glad/glad.h>


namespace voxel {
namespace opengl {


class Texture {
    GLuint m_handle;
    int m_width, m_height;
    GLuint m_format;
    GLuint m_internal_format;
    GLuint m_data_type;
public:
    Texture(int width, int height, GLuint internal_format, GLuint format, GLuint data_type, void* data = nullptr);
    Texture(const Texture& other) = delete;
    Texture(Texture&& other);
    ~Texture();

    GLuint getHandle();
    bool isValid();
    void bind(GLuint index = 0, GLuint uniform = -1);
};


} // opengl
} // voxel

#endif //VOXEL_ENGINE_TEXTURE_H
