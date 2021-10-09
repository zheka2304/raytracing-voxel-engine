#include "lightmap.h"

#include "voxel/common/profiler.h"


namespace voxel {
namespace render {

LightMapTexture::LightMapTexture(i32 width, i32 height) : m_width(width), m_height(height) {
    m_interpolation_frame_last = new opengl::Texture(width, height, GL_RGBA32F, GL_RGBA, GL_FLOAT);
    m_interpolation_frame_current = new opengl::Texture(width, height, GL_RGBA32F, GL_RGBA, GL_FLOAT);
    m_blur_frame_last = new opengl::Texture(width, height, GL_RGBA32F, GL_RGBA, GL_FLOAT);
    m_blur_frame_current = new opengl::Texture(width, height, GL_RGBA32F, GL_RGBA, GL_FLOAT);
}

LightMapTexture::~LightMapTexture() {
    delete(m_interpolation_frame_last);
    delete(m_interpolation_frame_current);
    delete(m_blur_frame_last);
    delete(m_blur_frame_current);
}

opengl::Texture& LightMapTexture::getLightmapTexture() {
    return *m_interpolation_frame_current;
}

opengl::Texture& LightMapTexture::getBlurPassTexture() {
    return *m_blur_frame_current;
}

void LightMapTexture::_swapBlurFrames() {
    std::swap(m_blur_frame_last, m_blur_frame_current);
}

void LightMapTexture::_swapInterpolationFrames() {
    std::swap(m_interpolation_frame_last, m_interpolation_frame_current);
}

void LightMapTexture::_bindTexturesForInterpolatePass(opengl::ShaderManager& shader_manager) {
    VOXEL_ENGINE_SHADER_CONSTANT(i32, interpolation_frame_last, shader_manager, "lightmap.interpolation_frame_last");
    VOXEL_ENGINE_SHADER_CONSTANT(i32, interpolation_frame_current, shader_manager, "lightmap.interpolation_frame_current");
    glBindImageTexture(interpolation_frame_last.get(), m_interpolation_frame_last->getHandle(), 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA32F);
    glBindImageTexture(interpolation_frame_current.get(), m_interpolation_frame_current->getHandle(), 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA32F);
    _bindTexturesForBlurPass(shader_manager);
}

void LightMapTexture::_bindTexturesForBlurPass(opengl::ShaderManager& shader_manager) {
    VOXEL_ENGINE_SHADER_CONSTANT(i32, blur_frame_last, shader_manager, "lightmap.blur_frame_last");
    VOXEL_ENGINE_SHADER_CONSTANT(i32, blur_frame_current, shader_manager, "lightmap.blur_frame_current");
    glBindImageTexture(blur_frame_last.get(), m_blur_frame_last->getHandle(), 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA32F);
    glBindImageTexture(blur_frame_current.get(), m_blur_frame_current->getHandle(), 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA32F);
}

void LightMapTexture::runInterpolationPass(RenderContext& context) {
    opengl::ShaderManager& shader_manager = context.getShaderManager();
    _bindTexturesForInterpolatePass(shader_manager);

    // run shader
    {
        VOXEL_ENGINE_PROFILE_GPU_SCOPE(render_lightmap_interpolate)
        VOXEL_ENGINE_SHADER_REF(opengl::ComputeShader, lightmap_interpolate_pass, shader_manager,"lightmap_interpolate_pass");
        lightmap_interpolate_pass->dispatchForTexture(math::Vec3i(m_width, m_height, 1));
    }

    _swapInterpolationFrames();
}

void LightMapTexture::runBlurPass(RenderContext& context) {
    opengl::ShaderManager& shader_manager = context.getShaderManager();

    // run shader
    {
        VOXEL_ENGINE_PROFILE_GPU_SCOPE(render_lightmap_blur)
        VOXEL_ENGINE_SHADER_REF(opengl::ComputeShader, soft_shadow_blur_pass1, shader_manager, "soft_shadow_blur_pass1");
        VOXEL_ENGINE_SHADER_REF(opengl::ComputeShader, soft_shadow_blur_pass2, shader_manager, "soft_shadow_blur_pass2");

        for (i32 i = 0; i < 2; i++) {
            _bindTexturesForBlurPass(shader_manager);
            soft_shadow_blur_pass1->dispatchForTexture(math::Vec3i(m_width, m_height, 1));
            _swapBlurFrames();
            _bindTexturesForBlurPass(shader_manager);
            soft_shadow_blur_pass2->dispatchForTexture(math::Vec3i(m_width, m_height, 1));
            _swapBlurFrames();
        }
    }

    {
        VOXEL_ENGINE_PROFILE_GPU_SCOPE(render_lightmap_3x3)
        VOXEL_ENGINE_SHADER_REF(opengl::ComputeShader, soft_shadow_3x3_pass, shader_manager, "soft_shadow_3x3_pass");
        for (i32 i = 0; i < 2; i++) {
            _bindTexturesForBlurPass(shader_manager);
            soft_shadow_3x3_pass->dispatchForTexture(math::Vec3i(m_width, m_height, 1));
            _swapBlurFrames();
        }
    }
}

} // render
} // voxel
