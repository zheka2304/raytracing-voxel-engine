#ifndef VOXEL_ENGINE_CAMERA_INC
#define VOXEL_ENGINE_CAMERA_INC

#include "common.inc"
#include "raytrace/types.inc"


COMPUTE_SHADER_BUFFER(${raytrace.camera_projection}, readonly, u_camera_projection_t, u_camera_projection, {
    vec3 position;
    vec3 rotation;
    vec2 ortho_range;
    vec2 perspective;
    vec2 distance_range;
})

// TODO: roll rotation is not supported yet
void getAllCameraVectors(out vec3 forward, out vec3 right, out vec3 up) {
    vec3 rotation = u_camera_projection.rotation;
    float cos_pitch = cos(rotation.x);
    forward = vec3(
        cos_pitch * sin(rotation.y),
        sin(rotation.x),
        cos_pitch * cos(rotation.y)
    );
    right = vec3(forward.z, 0.0, -forward.x);
    up = cross(forward, right);
}

Ray getCameraRay(vec2 screen_pos) {
    vec3 forward, right, up;
    getAllCameraVectors(forward, right, up);

    vec2 ortho_range = u_camera_projection.ortho_range;
    vec2 perspective = u_camera_projection.perspective;
    vec2 distance_range = u_camera_projection.distance_range;

    vec3 ray = normalize(forward + screen_pos.x * perspective.x * right + screen_pos.y * perspective.y * up);
    vec3 start = u_camera_projection.position + screen_pos.x * ortho_range.x * right + screen_pos.y * ortho_range.y * up + ray * distance_range.x;
    float max_distance = distance_range.y - distance_range.x;

    return Ray(start, ray, max_distance);
}

#endif  // VOXEL_ENGINE_CAMERA_INC