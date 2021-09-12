#include "texture.h"


namespace voxel {
namespace opengl {


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

void Texture::setFiltering(GLenum min_filter, GLenum mag_filter) {
    glBindTexture(GL_TEXTURE_2D, m_handle);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, min_filter);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, mag_filter);
    glBindTexture(GL_TEXTURE_2D, 0);
}

GLuint Texture::getHandle() {
    return m_handle;
}

bool Texture::isValid() {
    return m_handle != 0;
}

void Texture::bind(GLuint index, GLuint uniform) {
    glActiveTexture(GL_TEXTURE0 + index);
    glBindTexture(GL_TEXTURE_2D, m_handle);
    if (uniform != -1) {
        glUniform1i(uniform, index);
    }
}


} // opengl
} // voxel