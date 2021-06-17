#include "render_target.h"

#include "voxel/common/shaders.h"


namespace voxel {
namespace render {

void RenderTarget::bind(RenderContext& render_context) {
    opengl::ShaderManager& shader_manager = render_context.getShaderManager();
    VOXEL_ENGINE_SHADER_CONSTANT(int, color_texture, shader_manager, "raytrace.color_texture");
    VOXEL_ENGINE_SHADER_CONSTANT(int, light_texture, shader_manager, "raytrace.light_texture");
    VOXEL_ENGINE_SHADER_CONSTANT(int, depth_texture, shader_manager, "raytrace.depth_texture");
    glBindImageTexture(color_texture.get(), m_color_texture.getHandle(), 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA32F);
    glBindImageTexture(light_texture.get(), m_light_texture.getHandle(), 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA32F);
    glBindImageTexture(depth_texture.get(), m_depth_texture.getHandle(), 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA32F);
}

} // render
} // voxel