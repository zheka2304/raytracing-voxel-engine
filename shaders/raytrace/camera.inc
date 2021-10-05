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
    mat3 basis_matrix;
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
    right = vec3(cos(rotation.y), 0.0, -sin(rotation.y));
    up = cross(forward, right);
}

Ray getCameraRay(vec2 screen_pos) {
    vec2 perspective = u_camera_projection.perspective;
    vec2 distance_range = u_camera_projection.distance_range;

    vec3 ray = normalize(screen_pos.x * u_camera_projection.basis_matrix[0] + screen_pos.y * u_camera_projection.basis_matrix[1] + u_camera_projection.basis_matrix[2]);
    vec3 start = u_camera_projection.position + ray * distance_range.x;
    float max_distance = distance_range.y - distance_range.x;

    return Ray(start, ray, max_distance);
}

vec3 getCameraPosition() {
    return u_camera_projection.position;
}

float getDistanceToCamera(vec3 world_pos) {
    return length(world_pos - u_camera_projection.position);
}

vec3 worldPositionToScreen(vec3 world_pos) {
    vec3 pos_to_camera = world_pos - u_camera_projection.position;
    vec3 ray = normalize(pos_to_camera);
    vec3 projected = ray * u_camera_projection.basis_matrix;

    return vec3(projected.xy / projected.z / u_camera_projection.perspective.xy, length(pos_to_camera));
}

#endif  // VOXEL_ENGINE_CAMERA_INC