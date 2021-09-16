/*
-- FILE HEADER --


*/


#ifndef VOXEL_ENGINE_LIGHTING_INC
#define VOXEL_ENGINE_LIGHTING_INC

#include "common.inc"
#include "raytrace/material.inc"
#include "raytrace/raycast.inc"

#define NORMAL_FADEOUT_DISTANCE float(${lighting.normal_fadeout_distance})
#define SHADOW_NORMAL_FADEOUT_DISTANCE float(${lighting.shadow_normal_fadeout_distance})
#define NORMAL_FADEOUT_RATE float(${lighting.normal_fadeout_rate})
#define SHADOW_NORMAL_FADEOUT_RATE float(${lighting.shadow_normal_fadeout_rate})

struct LightSource {
    vec3 light_direction;
    float radius;
    float brightness;
};

Ray getLightRayForPosition(LightSource light_source, vec3 position, bool randomize, float rand_seed) {
    if (randomize) {
        Ray ray = Ray(position, -normalize(light_source.light_direction), 100.0);
        vec3 right, up;
        right = normalize(cross(ray.ray, vec3(ray.ray.x, 0, ray.ray.z)));
        up = normalize(cross(ray.ray, right));

        float rand_x = fast_random(vec4(position + 1.0, getTime() + rand_seed)) * 2.0 - 1.0;
        float rand_y = fast_random(vec4(position - 1.0, getTime() + rand_seed)) * 2.0 - 1.0;
        ray.ray = normalize(ray.ray + right * rand_x * light_source.radius + up * rand_y * light_source.radius);
        return ray;
    } else {
        return Ray(position, -normalize(light_source.light_direction), 100.0);
    }
}


LightSource getSkyLightSource() {
    return LightSource(vec3(-2.0, -.5, -1.0), 0.15, 1.0);
}

//
vec3 getVoxelNormal(uvec3 material, vec2 span) {
    // voxel side normal
    vec3 voxel_normal = getVoxelNormalFromMaterial(material.z);

    // non-normalized material normal vector
    vec3 mat_normal = getMatNormalFromMaterial(material.y);

    // normal length from 0 to 1 represents factor of blending material normal with voxel normal
    // - far voxels always use material normal, except it is zero length (bits 0x777)
    // - nearby voxels will use more material normal, if it length is higher, when it is 1, it will always use material normal
    float mat_normal_len = length(mat_normal);

    if (mat_normal_len > 0.0) {
        // get scale_i from material and calculate voxel size
        uint scale_i = (material.z >> 3u) & 31u;
        float voxel_size = uintBitsToFloat((scale_i + 127u - 23u) << 23u);

        float distance_factor = max(0.0, 1.0 - (span.x / (NORMAL_FADEOUT_DISTANCE * voxel_size)) * 0.5);
        float blend = pow(distance_factor, NORMAL_FADEOUT_RATE) * (1.0 - mat_normal_len);
        return mix(normalize(mat_normal), voxel_normal, blend);
    } else {
        return voxel_normal;
    }
}

vec3 getVoxelNormalForSoftShadows(uvec3 material, vec2 span) {
    // voxel side normal
    vec3 voxel_normal = getVoxelNormalFromMaterial(material.z);

    // non-normalized material normal vector
    vec3 mat_normal = getMatNormalFromMaterial(material.y);

    // normal length from 0 to 1 represents factor of blending material normal with voxel normal
    // - far voxels always use material normal, except it is zero length (bits 0x777)
    // - nearby voxels will use more material normal, if it length is higher, when it is 1, it will always use material normal
    float mat_normal_len = length(mat_normal);

    if (mat_normal_len > 0.0) {
        // get scale_i from material and calculate voxel size
        uint scale_i = (material.z >> 3u) & 31u;
        float voxel_size = uintBitsToFloat((scale_i + 127u - 23u) << 23u);

        float distance_factor = max(0.0, 1.0 - (span.x / (SHADOW_NORMAL_FADEOUT_DISTANCE * voxel_size)) * 0.5);
        float blend = pow(distance_factor, SHADOW_NORMAL_FADEOUT_RATE);
        return mix(normalize(mat_normal), voxel_normal, blend);
    } else {
        return voxel_normal;
    }
}

float getShadowValueFromRay(Ray light_ray, RaycastResultSpan span) {
    // check, if voxel is opaque and light is blocked by voxel itself
    if ((span.mat.xy & uvec2(0x3E000000u, 0x000000Fu)) == uvec2(31u << 25u, 0u)) {
        vec3 voxel_normal = getVoxelNormalFromMaterial(span.mat.z);
        if (dot(voxel_normal, light_ray.ray) < 0.0) {
            return 0.0;
        }
    }

    RaycastResult shadow_result = raycastVoxelWorld(light_ray, 10, 1, true);
    return shadow_result.count > 0 ? 0.0 : 1.0;
}

float getShadowValueFromLightSource(LightSource light_source, vec3 position, RaycastResultSpan span, int ray_count) {
    float value = 0.0;
    for (int i = 0; i < ray_count; i++) {
        value += getShadowValueFromRay(getLightRayForPosition(light_source, position, true, float(i)), span);
    }
    return value / float(ray_count);
}


#endif