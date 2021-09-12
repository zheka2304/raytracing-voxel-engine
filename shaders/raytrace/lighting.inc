/*
-- FILE HEADER --


*/


#ifndef VOXEL_ENGINE_LIGHTING_INC
#define VOXEL_ENGINE_LIGHTING_INC

#include "common.inc"
#include "raytrace/material.inc"
#include "raytrace/raycast.inc"

#define LIGHTMAP_DOWNSAMPLE int(${lighting.lightmap_downsample})
#define LIGHTMAP_DOWNSAMPLE_HALF (int(${lighting.lightmap_downsample}) / 2)
#define NORMAL_FADEOUT_DISTANCE float(${lighting.normal_fadeout_distance})
#define NORMAL_FADEOUT_RATE float(${lighting.normal_fadeout_rate})

Ray getSkyLightRayForPosition(vec3 position) {
    return Ray(position, normalize(vec3(-2.0, -0.5, -1.0)), 100.0);
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

        float distance_factor = max(0.0, 1.0 - (span.x / (NORMAL_FADEOUT_DISTANCE * voxel_size)) * 0.5);
        float blend = pow(distance_factor, NORMAL_FADEOUT_RATE);
        return mix(normalize(mat_normal), voxel_normal, blend);
    } else {
        return voxel_normal;
    }
}

//
// r - combined light value (from shadow
// g - light from normal only
// b - shadow distance, if shadow is not present, it contains POSITIVE_INF
vec3 getShadingDataForLightSource(Ray inv_light_ray, RaycastResultSpan span) {
    // calculate light value from normal
    vec3 normal = getVoxelNormal(span.mat, span.span);
    float normal_shade = dot(inv_light_ray.ray, normal) * .5 + .5;

    // check, if voxel is opaque and light is blocked by voxel itself
    if ((span.mat.xy & uvec2(0x3E000000u, 0x000000Fu)) == uvec2(31u << 25u, 0u)) {
        vec3 voxel_normal = getVoxelNormalFromMaterial(span.mat.z);
        if (dot(voxel_normal, inv_light_ray.ray) < 0.0) {
            return vec3(/* combined value */ 0.0, /* light from normal */ normal_shade,  /* distance */ 0.0);
        }
    }

    // check for shadow
    RaycastResult shadow_result = raycastVoxelWorld(inv_light_ray, 10, 1, true);
    if (shadow_result.count > 0) {
        return vec3(/* combined value */ 0.0, /* light from normal */ normal_shade,  /* distance */ shadow_result.distance);
    }

    return vec3(/* combined value */ normal_shade, /* light from normal */ normal_shade,  /* distance */ POSITIVE_INF);
}


#endif