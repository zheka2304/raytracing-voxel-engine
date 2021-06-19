#include "render_to_texture.h"

#include <iostream>


namespace voxel {
namespace opengl {

FullScreenQuad::FullScreenQuad(float z) {
    m_mesh.getVertices() = {
            {{-1, -1, z}, {0, 0}},
            {{1, -1, z}, {1, 0}},
            {{-1, 1, z}, {0, 1}},
            {{1, 1, z}, {1, 1}},
    };
    m_mesh.getIndices() = { 0, 1, 3, 0, 3, 2};
    m_mesh.invalidate();
    m_mesh.rebuild();
}

void FullScreenQuad::render() {
    m_mesh.render();
}


RenderToTexture::RenderToTexture(int width, int height, int output_texture_count) : m_width(width), m_height(height) {
    // create and bind framebuffer
    glGenFramebuffers(1, &m_framebuffer_handle);
    glBindFramebuffer(GL_FRAMEBUFFER, m_framebuffer_handle);

    // create and bind depth renderbuffer
    glGenRenderbuffers(1, &m_depth_buffer_handle);
    glBindRenderbuffer(GL_RENDERBUFFER, m_depth_buffer_handle);
    glad_glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, width, height);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, m_depth_buffer_handle);

    // create output textures
    for (int i = 0; i < output_texture_count; i++) {
        m_output_textures.emplace_back(Texture(width, height, GL_RGB, GL_RGB, GL_FLOAT));
    }

    // attach textures to framebuffer
    GLenum attachment = GL_COLOR_ATTACHMENT0;
    std::vector<GLenum> draw_buffers;
    for (Texture& texture : m_output_textures) {
        glFramebufferTexture(GL_FRAMEBUFFER, attachment, texture.getHandle(), 0);
        draw_buffers.emplace_back(attachment);
        attachment++;
    }
    glDrawBuffers(draw_buffers.size(), &draw_buffers[0]);

    // unbind
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glBindRenderbuffer(GL_RENDERBUFFER, 0);
}

RenderToTexture::RenderToTexture(RenderToTexture&& other) :
    m_output_textures(std::move(other.m_output_textures)),
    m_fullscreen_quad(std::move(other.m_fullscreen_quad)) {

    m_framebuffer_handle = other.m_framebuffer_handle;
    m_depth_buffer_handle = other.m_depth_buffer_handle;
    other.m_framebuffer_handle = 0;
    other.m_depth_buffer_handle = 0;

    m_width = other.m_width;
    m_height = other.m_height;
    m_is_rendering = other.m_is_rendering;
    m_fallback_viewport = other.m_fallback_viewport;
    m_background_color = other.m_background_color;
}

RenderToTexture::~RenderToTexture() {
    if (m_framebuffer_handle != 0) {
        glDeleteFramebuffers(1, &m_framebuffer_handle);
    }
    if (m_depth_buffer_handle != 0) {
        glDeleteRenderbuffers(1, &m_depth_buffer_handle);
    }
}


void RenderToTexture::begin() {
    if (m_is_rendering) {
        std::cerr << "RenderToTexture::begin() called in invalid state: already rendering\n";
        return;
    }
    if (m_framebuffer_handle == 0 || m_depth_buffer_handle == 0) {
        std::cerr << "RenderToTexture::begin() called in invalid state: framebuffer or depth buffer is not initialized or was moved\n";
        return;
    }
    m_is_rendering = true;
    glBindFramebuffer(GL_FRAMEBUFFER, m_framebuffer_handle);
    glClearColor(m_background_color.r, m_background_color.g, m_background_color.b, m_background_color.a);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glGetIntegerv(GL_VIEWPORT, reinterpret_cast<GLint*>(&m_fallback_viewport));
    glViewport(0, 0, m_width, m_height);
}

void RenderToTexture::end() {
    if (!m_is_rendering) {
        std::cerr << "RenderToTexture::end() called in invalid state: not rendering\n";
        return;
    }
    m_is_rendering = false;
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(m_fallback_viewport.x, m_fallback_viewport.y, m_fallback_viewport.w, m_fallback_viewport.h);
}

void RenderToTexture::render() {
    begin();
    m_fullscreen_quad.render();
    end();
}

Texture& RenderToTexture::getOutputTexture(int index) {
    return m_output_textures[index];
}

std::vector<Texture>& RenderToTexture::getOutputTextures() {
    return m_output_textures;
}

} // opengl
} // voxel