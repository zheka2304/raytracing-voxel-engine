#ifndef VOXEL_ENGINE_CAMERA_H
#define VOXEL_ENGINE_CAMERA_H

#include <functional>

#include "voxel/common/define.h"
#include "voxel/common/math/vec.h"
#include "voxel/common/opengl.h"
#include "voxel/engine/render/render_context.h"
#include "voxel/engine/render/render_target.h"


namespace voxel {
namespace render {

// universal representation of the projection, passed to the raytrace shader
// this structure can be directly passed as uniform buffer, as it is aligned in proper way
struct CameraProjection {
    // position of the camera in chunk units
    GLSL_BUFFER_ALIGN(16) vec_math::Vec3 m_position;

    // rotation euler angles in radians
    GLSL_BUFFER_ALIGN(16) vec_math::Vec3 m_rotation;

    // orthographic projection parameters: (x, y) is size of the camera view in chunk units
    GLSL_BUFFER_ALIGN(16) vec_math::Vec2 m_ortho_range;

    // perspective parameters: (x, y) is coefficients of ray direction displacement from the position relative to screen center,
    // where relative position x and y are in range (-1; 1), this values are usually calculated from vertical and horizontal FOV
    GLSL_BUFFER_ALIGN(16) vec_math::Vec2 m_perspective;

    // x is near plane and y is far plane
    GLSL_BUFFER_ALIGN(16) vec_math::Vec2 m_distance_range;
};

class Camera {
    // holds all data about camera position and projection parameters
    CameraProjection m_projection;

    opengl::OnDemand<opengl::ShaderUniformBuffer<CameraProjection>> m_uniform_projection;
public:
    Camera();

    // executes one complete world render call into given render target
    // 1) prepares for camera passes - binds world cache buffer,
    // 2) runs all passes (raytracing passes first, post processing passes next)
    // 3) calls glFinish() to wait all passes to complete
    // 4) requests cache updates from render context
    void render(RenderContext& context, RenderTarget& target);
};

} // render
} // voxel

#endif //VOXEL_ENGINE_CAMERA_H
