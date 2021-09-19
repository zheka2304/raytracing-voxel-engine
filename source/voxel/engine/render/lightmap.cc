#include "lightmap.h"


namespace voxel {
namespace render {

LightMapTexture::LightMapTexture(int width, int height) :
    m_width(width), m_height(height),
    m_blur_pass_texture(width, height, GL_RGBA32F, GL_RGBA, GL_FLOAT) {
    m_last_frame = new opengl::Texture(width, height, GL_RGBA32F, GL_RGBA, GL_FLOAT);
    m_current_frame = new opengl::Texture(width, height, GL_RGBA32F, GL_RGBA, GL_FLOAT);
}

LightMapTexture::~LightMapTexture() {
    delete(m_last_frame);
    delete(m_current_frame);
}

opengl::Texture& LightMapTexture::getLightmapTexture() {
    return *m_current_frame;
}

opengl::Texture& LightMapTexture::getBlurPassTexture() {
    return m_blur_pass_texture;
}

void LightMapTexture::_bindTexturesForLightmapPass(opengl::ShaderManager& shader_manager) {
    VOXEL_ENGINE_SHADER_CONSTANT(int, last_frame, shader_manager, "lightmap.last_frame");
    VOXEL_ENGINE_SHADER_CONSTANT(int, current_frame, shader_manager, "lightmap.current_frame");
    VOXEL_ENGINE_SHADER_CONSTANT(int, blur_pass_texture, shader_manager, "lightmap.blur_pass_texture");
    glBindImageTexture(last_frame.get(), m_last_frame->getHandle(), 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA32F);
    glBindImageTexture(current_frame.get(), m_current_frame->getHandle(), 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA32F);
    glBindImageTexture(blur_pass_texture.get(), m_blur_pass_texture.getHandle(), 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA32F);
}

void LightMapTexture::runInterpolationPass(RenderContext& context) {
    opengl::ShaderManager& shader_manager = context.getShaderManager();
    _bindTexturesForLightmapPass(shader_manager);

    // run shader
    VOXEL_ENGINE_SHADER_REF(opengl::ComputeShader, lightmap_interpolate_pass, shader_manager, "lightmap_interpolate_pass");
    lightmap_interpolate_pass->dispatchForTexture(math::Vec3i(m_width, m_height, 1));

    std::swap(m_current_frame, m_last_frame);
}

void LightMapTexture::runBlurPass(RenderContext& context) {
    opengl::ShaderManager& shader_manager = context.getShaderManager();
    _bindTexturesForLightmapPass(shader_manager);

    // run shader
    VOXEL_ENGINE_SHADER_REF(opengl::ComputeShader, soft_shadow_blur_pass1, shader_manager, "soft_shadow_blur_pass1");
    VOXEL_ENGINE_SHADER_REF(opengl::ComputeShader, soft_shadow_blur_pass2, shader_manager, "soft_shadow_blur_pass2");
    soft_shadow_blur_pass1->dispatchForTexture(math::Vec3i(m_width, m_height, 1));
    soft_shadow_blur_pass2->dispatchForTexture(math::Vec3i(m_width, m_height, 1));
    soft_shadow_blur_pass1->dispatchForTexture(math::Vec3i(m_width, m_height, 1));
    soft_shadow_blur_pass2->dispatchForTexture(math::Vec3i(m_width, m_height, 1));

    VOXEL_ENGINE_SHADER_REF(opengl::ComputeShader, soft_shadow_3x3_pass1, shader_manager, "soft_shadow_3x3_pass1");
    VOXEL_ENGINE_SHADER_REF(opengl::ComputeShader, soft_shadow_3x3_pass2, shader_manager, "soft_shadow_3x3_pass2");
    soft_shadow_3x3_pass1->dispatchForTexture(math::Vec3i(m_width, m_height, 1));
    soft_shadow_3x3_pass2->dispatchForTexture(math::Vec3i(m_width, m_height, 1));

}

} // render
} // voxel
