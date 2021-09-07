/*
-- FILE HEADER --

BLENDING
Voxel blend mode is 4 bits, representing following modes:
  First 2 bits define blend operation:
  - 0 - translucent - same to opengl SRC_ALPHA, ONE_MINUS_SRC_ALPHA
  - 1 - multiplication - blends color with itself, multiplied by material color, more effect strength = more blend
  - 2 - additive - adds material color, multiplied by blend strength
  Second 2 bits define blend interaction with material width:
  - 0 - none: width does not affect blending
  - 1 - direct: more width = more effect
  - 2 - max axis: same to direct, but width is calculated by maximum axis using ray direction
*/


#ifndef VOXEL_ENGINE_BLENDING_INC
#define VOXEL_ENGINE_BLENDING_INC

vec4 blendVoxelColors(
    vec4 input_color,
    vec4 voxel_color,
    uvec3 material,
    vec3 ray,
    float span_length
) {
    uint blend_mode = (material.y & 3u);
    uint factor_mode = ((material.y >> 2) & 3u);

    // NONE
    float factor = voxel_color.a;

    if (factor_mode > 0u) {
        uint alpha_i = (material.x >> 25u) & 31u;
        float alpha_distance_factor = uintBitsToFloat((127u + 8u - (alpha_i >> 1u)) << 23u) * ((alpha_i & 1u) != 0u ? 1.0 : 0.5);

        // DIRECT
        if (factor_mode == 1u) {
            uint alpha_i = (material.x >> 25u) & 31u;
            factor = min(1.0, span_length / alpha_distance_factor);
        // MAX AXIS
        } else if (factor_mode == 2u) {
            vec3 span_len_v = span_length * ray;
            factor = min(1.0, max(span_len_v.x, max(span_len_v.y, span_len_v.z)) / alpha_distance_factor);
        }
    }

    // TRANSLUCENT
    if (blend_mode == 0u) {
        return mix(input_color, vec4(voxel_color.rgb, 1.0), factor);
    // MULTIPLY
    } else if (blend_mode == 1u) {
        return mix(input_color, input_color * voxel_color, factor);
    // ADD
    } else if (blend_mode == 2u) {
        return mix(input_color, input_color + voxel_color, factor);
    }

    return voxel_color;
}

#endif