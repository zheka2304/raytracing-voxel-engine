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
    
    opengl::ShaderManager& shader_manager = context.getShaderManager();
    m_projection_uniform.bindUniform(shader_manager);
}

} // render
} // voxel
