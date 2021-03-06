#include "camera.h"

#include <math.h>
#include "voxel/common/utils/time.h"
#include "voxel/common/profiler.h"


namespace voxel {
namespace render {


void CameraProjection::setFov(f32 fov, f32 ratio) {
    if (fov == 0 || ratio == 0) {
        perspective = math::Vec2f(0, 0);
        return;
    }

    f32 tg = tan(fov / 2.0f);
    perspective.x = tg;
    perspective.y = tg * ratio;
}

void CameraProjection::setOrtho(f32 w, f32 h) {
    ortho_range = math::Vec2f(w / 2.0f, h / 2.0f);
}

void CameraProjection::setNearAndFar(f32 near, f32 far) {
    distance_range = math::Vec2f(near, far);
}

void CameraProjection::updateMatrix() {
    f32 cos_pitch = cos(rotation.x);
    math::Vec3f forward = math::Vec3f(
            cos_pitch * sin(rotation.y),
            sin(rotation.x),
            cos_pitch * cos(rotation.y)
    );
    math::Vec3f right = math::Vec3f(cos(rotation.y), 0.0, -sin(rotation.y));
    math::Vec3f up = math::cross(forward, right);
    basis_matrix[0].x = right.x;
    basis_matrix[0].y = right.y;
    basis_matrix[0].z = right.z;
    basis_matrix[1].x = up.x;
    basis_matrix[1].y = up.y;
    basis_matrix[1].z = up.z;
    basis_matrix[2].x = forward.x;
    basis_matrix[2].y = forward.y;
    basis_matrix[2].z = forward.z;
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
    VOXEL_ENGINE_SHADER_REF(opengl::GraphicsShader, raytrace_output_pass_shader, shader_manager, "raytrace_output_pass");

    // update camera uniforms
    *m_time_uniform = utils::getTimeSinceStart();
    *m_screen_size_uniform = math::Vec2i(target.getWidth(), target.getHeight());
    m_projection_uniform->updateMatrix();

    // bind camera uniforms
    m_time_uniform.bindUniform(shader_manager);
    m_projection_uniform.bindUniform(shader_manager);
    m_screen_size_uniform.bindUniform(shader_manager);

    // run compute shader pass
    {
        VOXEL_ENGINE_PROFILE_GPU_SCOPE(render_raytrace_pass)
        target.bindForCompute(context);
        raytrace_screen_pass_shader->dispatchForTexture(math::Vec3i(target.getWidth(), target.getHeight(), 1));
    }

    // swap spatial buffer
    {
        VOXEL_ENGINE_PROFILE_GPU_SCOPE(render_spatial_buffer)
        target.getSpatialBuffer().runSwap(context);
    }

    // post process lightmap
    {
        VOXEL_ENGINE_PROFILE_GPU_SCOPE(render_lightmap)
        target.getLightmap().runInterpolationPass(context);
        target.getLightmap().runBlurPass(context);
    }

    // run final post processing & output pass
    {
        VOXEL_ENGINE_PROFILE_GPU_SCOPE(render_postprocess_output)
        raytrace_output_pass_shader->bind();
        target.bindForPostProcessing(*raytrace_output_pass_shader);
        target.getRenderToTexture().render();
        raytrace_output_pass_shader->unbind();
    }
}

} // render
} // voxel
