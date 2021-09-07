/*
-- FILE HEADER --


*/

#ifndef VOXEL_ENGINE_MATERIAL_INC
#define VOXEL_ENGINE_MATERIAL_INC

vec4 getVoxelColorFromMaterial(uint material_x) {
    return vec4(
        vec3(material_x & 0xFFu, (material_x >> 8u) & 0xFFu, (material_x >> 16u) & 0xFFu) / 255.0,
        float((material_x >> 25u) & 31u) / 31.0
    );
}

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
    float yaw = float(normal_bits_yaw) * (2.0 * 3.14159 / 63.0) - 3.14159;
    float pitch = (float(normal_bits_pitch) / 31.0 - .5) * 3.14159;
    float len = float(normal_bits_length) / 16.0;

    float cos_pitch = cos(pitch);
    return vec3(sin(yaw) * cos_pitch, sin(pitch), cos(yaw) * cos_pitch) * len;
}


#endif