#include "camera.h"

namespace voxel {
namespace render {

Camera::Camera() :
    m_uniform_projection([=] () {
        return new opengl::ShaderUniformBuffer<CameraProjection>("raytrace.camera_projection", &m_projection);
    }) {
}

void Camera::render(RenderContext& context, RenderTarget& target) {
    m_uniform_projection->bindUniform(context.getShaderManager());
}

} // render
} // voxel
