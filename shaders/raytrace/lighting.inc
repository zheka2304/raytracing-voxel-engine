/*
-- FILE HEADER --


*/


#ifndef VOXEL_ENGINE_LIGHTING_INC
#define VOXEL_ENGINE_LIGHTING_INC

#define NORMAL_FADEOUT_DISTANCE float(${lighting.normal_fadeout_distance})
#define NORMAL_FADEOUT_RATE float(${lighting.normal_fadeout_rate})


//
vec3 getVoxelNormalFromMaterial(uint material_z) {
    uint normal_bits = (material_z & 7u);
    vec3 normal = (normal_bits & 1u) == 0u ? vec3(-1.0) : vec3(1.0);
    normal_bits &= 6u;
    if (normal_bits == 2u) normal *= vec3(1.0, 0.0, 0.0);
    else if (normal_bits == 4u) normal *= vec3(0.0, 1.0, 0.0);
    else if (normal_bits == 6u) normal *= vec3(0.0, 0.0, 1.0);
    return normal;
}

//
vec3 getMatNormalFromMaterial(uint material_y) {
    uint normal_bits_yaw = (material_y >> 8u) & 0x3Fu;
    uint normal_bits_pitch = (material_y >> 14u) & 0x1Fu;
    uint normal_bits_length = (material_y >> 19u) & 0xFu;
    float yaw = float(normal_bits_yaw) * (2.0 * 3.14159 / 63.0);
    float pitch = (float(normal_bits_pitch) / 31.0 - .5) * 3.14159;
    float len = float(normal_bits_length) / 16.0;

    float cos_pitch = cos(pitch);
    return vec3(sin(yaw) * cos_pitch, sin(pitch), cos(yaw) * cos_pitch) * len;
}

//
vec3 getVoxelNormal(
    uvec3 material,
    vec2 span
) {
    // voxel side normal
    vec3 voxel_normal = getVoxelNormalFromMaterial(material.z);

    // non-normalized material normal vector
    vec3 mat_normal = getMatNormalFromMaterial(material.y);

    // normal length from 0 to 1 represents factor of blending material normal with voxel normal
    // - far voxels always use material normal, except it is zero length (bits 0x777)
    // - nearby voxels will use more material normal, if it length is higher
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


#endif