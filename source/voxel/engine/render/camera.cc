#include "camera.h"


namespace voxel {
namespace render {

Camera::Camera() :
        m_projection_uniform("raytrace.camera_projection") {
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
    VOXEL_ENGINE_SHADER_CONSTANT(int, screen_pass_work_group_size, shader_manager, "raytrace.screen_pass_work_group_size");

    // bind camera uniforms
    m_projection_uniform.bindUniform(shader_manager);

    // run compute shader pass
    target.bindForCompute(context);
    raytrace_screen_pass_shader->dispatch(target.getComputeDispatchSize(screen_pass_work_group_size.get()));

    // run post processing pass
    raytrace_combine_pass_shader->bind();
    target.bindForPostProcessing(*raytrace_combine_pass_shader);
    target.getRenderToTexture().render();
    raytrace_combine_pass_shader->unbind();
}

} // render
} // voxel
