#include "camera.h"

#include <math.h>
#include "voxel/common/utils/time.h"


namespace voxel {
namespace render {


void CameraProjection::setFov(float fov, float ratio) {
    if (fov == 0 || ratio == 0) {
        m_perspective = math::Vec2f(0, 0);
        return;
    }

    float tg = tan(fov / 2.0f);
    m_perspective.x = tg;
    m_perspective.y = tg * ratio;
}

void CameraProjection::setOrtho(float w, float h) {
    m_ortho_range = math::Vec2f(w / 2.0f, h / 2.0f);
}

void CameraProjection::setNearAndFar(float near, float far) {
    m_distance_range = math::Vec2f(near, far);
}


Camera::Camera() :
        m_projection_uniform("raytrace.camera_projection"),
        m_time_uniform("common.time_uniform"),
        m_screen_size_uniform("common.screen_size") {
}

CameraProjection& Camera::getProjection() {
    return *m_projection_uniform;
}

void Camera::render(RenderContext& context, RenderTarget& target) {
    m_thread_guard.guard();

    // shader references
    opengl::ShaderManager& shader_manager = context.getShaderManager();
    VOXEL_ENGINE_SHADER_REF(opengl::ComputeShader, raytrace_screen_pass_shader, shader_manager, "raytrace_screen_pass");
    VOXEL_ENGINE_SHADER_REF(opengl::GraphicsShader, raytrace_combine_pass_shader, shader_manager, "raytrace_combine_pass");

    // update camera uniforms
    *m_time_uniform = utils::getTimeSinceStart();
    *m_screen_size_uniform = math::Vec2i(target.getWidth(), target.getHeight());

    // bind camera uniforms
    m_time_uniform.bindUniform(shader_manager);
    m_projection_uniform.bindUniform(shader_manager);
    m_screen_size_uniform.bindUniform(shader_manager);

    // run compute shader pass
    target.bindForCompute(context);
    raytrace_screen_pass_shader->dispatchForTexture(math::Vec3i(target.getWidth(), target.getHeight(), 1));

    // swap spatial buffer
    target.getSpatialBuffer().runSwap(context);

    // post process lightmap
    target.getLightmap().runInterpolationPass(context);
    target.getLightmap().runBlurPass(context);

    // run final post processing pass
    raytrace_combine_pass_shader->bind();
    target.bindForPostProcessing(*raytrace_combine_pass_shader);
    target.getRenderToTexture().render();
    raytrace_combine_pass_shader->unbind();
}

} // render
} // voxel
