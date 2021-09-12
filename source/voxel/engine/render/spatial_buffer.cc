#include "spatial_buffer.h"


namespace voxel {
namespace render {

SpatialRenderBuffer::SpatialRenderBuffer(int width, int height) :
    m_width(width), m_height(height),
    m_spatial_buffer(m_width, m_height, GL_RGBA32F, GL_RGBA, GL_FLOAT) {
    m_last_frame = new opengl::Texture(m_width, m_height, GL_RGBA32F, GL_RGBA, GL_FLOAT);
    m_current_frame = new opengl::Texture(m_width, m_height, GL_RGBA32F, GL_RGBA, GL_FLOAT);
}

SpatialRenderBuffer::~SpatialRenderBuffer() {
    delete(m_last_frame);
    delete(m_current_frame);
}

void SpatialRenderBuffer::bind(RenderContext& context) {
    opengl::ShaderManager& shader_manager = context.getShaderManager();
    VOXEL_ENGINE_SHADER_CONSTANT(int, lightmap_texture, context.getShaderManager(), "common.spatial_buffer");
    glBindImageTexture(lightmap_texture.get(), m_spatial_buffer.getHandle(), 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA32F);
    VOXEL_ENGINE_SHADER_CONSTANT(int, current_frame_texture, shader_manager, "common.spatial_buffer_current_frame");
    glBindImageTexture(current_frame_texture.get(), m_current_frame->getHandle(), 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA32F);
}

void SpatialRenderBuffer::runSwap(RenderContext& context) {
    opengl::ShaderManager& shader_manager = context.getShaderManager();

    // bind all textures
    VOXEL_ENGINE_SHADER_CONSTANT(int, last_frame_texture, shader_manager, "spatial_buffer.last_frame");
    glBindImageTexture(last_frame_texture.get(), m_last_frame->getHandle(), 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA32F);
    bind(context);

    // reset buffer
    VOXEL_ENGINE_SHADER_REF(opengl::ComputeShader, spatial_buffer_reset, shader_manager, "spatial_buffer_reset");
    spatial_buffer_reset->dispatchForTexture(math::Vec3i(m_width, m_height, 1));

    // run main shader
    VOXEL_ENGINE_SHADER_REF(opengl::ComputeShader, spatial_buffer_pass, shader_manager, "spatial_buffer_pass");
    spatial_buffer_pass->dispatchForTexture(math::Vec3i(m_width, m_height, 1));

    // run postprocess shader
    VOXEL_ENGINE_SHADER_REF(opengl::ComputeShader, spatial_buffer_postprocess, shader_manager, "spatial_buffer_postprocess");
    spatial_buffer_postprocess->dispatchForTexture(math::Vec3i(m_width, m_height, 1));

    // swap buffers
    std::swap(m_last_frame, m_current_frame);
}


} // render
} // voxel