#include "render_target.h"


namespace voxel {
namespace render {

RenderTarget::RenderTarget(int width, int height) :
        m_width(width), m_height(height),
        m_color_texture(m_width, m_height, GL_RGBA32F, GL_RGBA, GL_FLOAT),
        m_depth_texture(m_width, m_height, GL_RGBA32F, GL_RGBA, GL_FLOAT),
        m_lightmap(m_width, m_height),
        m_spatial_buffer(m_width, m_height),
        m_result_render_to_texture(m_width, m_height, 1) {
}

void RenderTarget::bindForCompute(RenderContext& render_context) {
    opengl::ShaderManager& shader_manager = render_context.getShaderManager();
    VOXEL_ENGINE_SHADER_CONSTANT(int, color_texture, shader_manager, "raytrace.color_texture");
    VOXEL_ENGINE_SHADER_CONSTANT(int, light_texture, shader_manager, "raytrace.light_texture");
    VOXEL_ENGINE_SHADER_CONSTANT(int, depth_texture, shader_manager, "raytrace.depth_texture");
    glBindImageTexture(color_texture.get(), m_color_texture.getHandle(), 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA32F);
    glBindImageTexture(depth_texture.get(), m_depth_texture.getHandle(), 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA32F);
    glBindImageTexture(light_texture.get(), m_lightmap.getLightmapTexture().getHandle(), 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA32F);

    m_spatial_buffer.bind(render_context);
}

void RenderTarget::bindForPostProcessing(opengl::GraphicsShader& post_processing_shader) {
    VOXEL_ENGINE_SHADER_UNIFORM(color_texture, post_processing_shader, "IN_TEXTURE_COLOR");
    VOXEL_ENGINE_SHADER_UNIFORM(light_texture, post_processing_shader, "IN_TEXTURE_LIGHT");
    VOXEL_ENGINE_SHADER_UNIFORM(depth_texture, post_processing_shader, "IN_TEXTURE_DEPTH");
    m_color_texture.bind(1, color_texture);
    m_depth_texture.bind(3, depth_texture);
    m_lightmap.getBlurPassTexture().bind(2, light_texture);
}

int RenderTarget::getWidth() {
    return m_width;
}

int RenderTarget::getHeight() {
    return m_height;
}

opengl::Texture& RenderTarget::getColorTexture() {
    return m_color_texture;
}

opengl::Texture& RenderTarget::getDepthTexture() {
    return m_depth_texture;
}

LightMapTexture& RenderTarget::getLightmap() {
    return m_lightmap;
}

SpatialRenderBuffer& RenderTarget::getSpatialBuffer() {
    return m_spatial_buffer;
}

opengl::RenderToTexture& RenderTarget::getRenderToTexture() {
    return m_result_render_to_texture;
}

opengl::Texture& RenderTarget::getResultTexture() {
    return m_result_render_to_texture.getOutputTexture();
}

} // render
} // voxel