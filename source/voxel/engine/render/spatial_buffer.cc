#include "spatial_buffer.h"


namespace voxel {
namespace render {

SpatialRenderBuffer::SpatialRenderBuffer(int width, int height) :
    m_width(width), m_height(height),
    m_spatial_buffer("common.spatial_buffer"),
    m_spatial_buffer_spinlock("common.spatial_buffer_spinlock") {
    m_last_frame = new opengl::Texture(m_width, m_height, GL_RGBA32F, GL_RGBA, GL_FLOAT);
    m_current_frame = new opengl::Texture(m_width, m_height, GL_RGBA32F, GL_RGBA, GL_FLOAT);
    m_spatial_buffer.preallocate(width * height * 4 * 8, GL_DYNAMIC_READ);
    m_spatial_buffer_spinlock.preallocate(width * height, GL_DYNAMIC_READ);
    m_spatial_buffer_spinlock.clear(GL_R32UI, GL_RED, GL_UNSIGNED_INT);
}

SpatialRenderBuffer::~SpatialRenderBuffer() {
    delete(m_last_frame);
    delete(m_current_frame);
}

void SpatialRenderBuffer::bind(RenderContext& context) {
    opengl::ShaderManager& shader_manager = context.getShaderManager();
    VOXEL_ENGINE_SHADER_CONSTANT(int, current_frame_texture, shader_manager, "common.spatial_buffer_current_frame");
    glBindImageTexture(current_frame_texture.get(), m_current_frame->getHandle(), 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA32F);
    m_spatial_buffer.bind(context.getShaderManager());
    m_spatial_buffer_spinlock.bind(context.getShaderManager());
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

    // run main postprocess shader (interpolate + discard)
    VOXEL_ENGINE_SHADER_REF(opengl::ComputeShader, spatial_buffer_postprocess_interpolate, shader_manager, "spatial_buffer_postprocess_interpolate");
    spatial_buffer_postprocess_interpolate->dispatchForTexture(math::Vec3i(m_width, m_height, 1));

    // optionally, you can run add margin pass, that adds 1 pixel margin to remaining zero-stability regions
    // VOXEL_ENGINE_SHADER_REF(opengl::ComputeShader, spatial_buffer_postprocess_margin, shader_manager, "spatial_buffer_postprocess_margin");
    // spatial_buffer_postprocess_margin->dispatchForTexture(math::Vec3i(m_width, m_height, 1));

    // swap buffers
    std::swap(m_last_frame, m_current_frame);
}


} // render
} // voxel