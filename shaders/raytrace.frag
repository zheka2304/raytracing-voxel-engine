#version 330 core

varying vec2 uv;

#define PI 3.1415926

// In this structure we split chunk into 2 tiers of regions,
// first tier is 8x8x8, second tier is 4x4x4 of first tier (32x32x32) and
// total chunk size is 4x4x4 of second tier (128x128x128)
// This structure allow us to send data to GPU by chunks and
// run much more efficient raytracing algorithm, that skips empty regions

uniform highp float TIME;
uniform usamplerBuffer CHUNK_BUFFER;
uniform int CHUNK_DATA_OFFSETS_IN_BUFFER[64];
uniform ivec3 CHUNK_OFFSET;
uniform ivec3 CHUNK_COUNT;

uniform vec4 VIEWPORT;
uniform vec3 CAMERA_POSITION;
uniform vec3 CAMERA_RAY;
uniform vec2 CAMERA_NEAR_AND_FAR;

uniform vec3 DIRECT_LIGHT_RAY;
uniform vec4 DIRECT_LIGHT_COLOR; // alpha value is light intensity
uniform vec4 AMBIENT_LIGHT_COLOR; // alpha value is in-shadow direct light multiplier


struct RaytraceData {
    vec3 start; // starting position
    vec3 ray; // ray is normalized
    vec3 rayS; // S value for each coordinate
    vec3 rayL;
    float distance; // total distance
    int voxel_distance; // counter for voxel steps to check for region end
    ivec3 pos; // cell position
    ivec3 pos_step;
};

struct RegTraverseData {
    vec3 add_f;
    ivec3 add_i;
    int region_end;
};


#define BINARY_CHOICE_AB_A(A, B) A
#define BINARY_CHOICE_AB_B(A, B) B

#define RAYTRACE_DDA_STEP(RAYTRACE_DATA, IS_RAY_X_POSITIVE, IS_RAY_Y_POSITIVE, IS_RAY_Z_POSITIVE) \
    { \
        bvec3 _mask = lessThanEqual(RAYTRACE_DATA.rayL, min(RAYTRACE_DATA.rayL.yzx, RAYTRACE_DATA.rayL.zxy)); \
        RAYTRACE_DATA.rayL += vec3(_mask) * RAYTRACE_DATA.rayS; \
        RAYTRACE_DATA.pos += ivec3(_mask) * RAYTRACE_DATA.pos_step; \
        RAYTRACE_DATA.voxel_distance++; \
    }


#define PREPARE_TO_TRAVERSE_REGION(RAYTRACE_DATA, DATA_VAR, REGION_SIZE, REGION_SIZE_BIT_MASK, IS_RAY_X_POSITIVE, IS_RAY_Y_POSITIVE, IS_RAY_Z_POSITIVE) \
    { \
        int COUNT_X = IS_RAY_X_POSITIVE(REGION_SIZE - (RAYTRACE_DATA.pos.x & REGION_SIZE_BIT_MASK), (RAYTRACE_DATA.pos.x & REGION_SIZE_BIT_MASK) + 1);  \
        float _rayL_x = RAYTRACE_DATA.rayL.x + RAYTRACE_DATA.rayS.x * float(COUNT_X - 1); \
        vec2 _add_yz = ceil(max(vec2(0), (_rayL_x - RAYTRACE_DATA.rayL.yz) / RAYTRACE_DATA.rayS.yz)); \
        ivec2 _add_yz_i = ivec2(_add_yz); \
        int _dis_x = _add_yz_i.x + _add_yz_i.y + COUNT_X;  \
        \
        int COUNT_Y = IS_RAY_Y_POSITIVE(REGION_SIZE - (RAYTRACE_DATA.pos.y & REGION_SIZE_BIT_MASK), (RAYTRACE_DATA.pos.y & REGION_SIZE_BIT_MASK) + 1);  \
        float _rayL_y = RAYTRACE_DATA.rayL.y + RAYTRACE_DATA.rayS.y * float(COUNT_Y - 1); \
        vec2 _add_xz = ceil(max(vec2(0), (_rayL_y - RAYTRACE_DATA.rayL.xz) / RAYTRACE_DATA.rayS.xz)); \
        ivec2 _add_xz_i = ivec2(_add_xz); \
        int _dis_y = _add_xz_i.x + _add_xz_i.y + COUNT_Y;  \
        \
        int COUNT_Z = IS_RAY_Z_POSITIVE(REGION_SIZE - (RAYTRACE_DATA.pos.z & REGION_SIZE_BIT_MASK), (RAYTRACE_DATA.pos.z & REGION_SIZE_BIT_MASK) + 1); \
        float _rayL_z = RAYTRACE_DATA.rayL.z + RAYTRACE_DATA.rayS.z * float(COUNT_Z - 1); \
        vec2 _add_xy = ceil(max(vec2(0), (_rayL_z - RAYTRACE_DATA.rayL.xy) / RAYTRACE_DATA.rayS.xy)); \
        ivec2 _add_xy_i = ivec2(_add_xy); \
        int _dis_z = _add_xy_i.x + _add_xy_i.y + COUNT_Z;  \
        \
        if (_dis_x < _dis_y) {  \
            if (_dis_z < _dis_x) { \
                DATA_VAR.add_i = ivec3(_add_xy_i, COUNT_Z); \
                DATA_VAR.region_end = RAYTRACE_DATA.voxel_distance + _dis_z; \
            } else { \
                DATA_VAR.add_i = ivec3(COUNT_X, _add_yz_i); \
                DATA_VAR.region_end = RAYTRACE_DATA.voxel_distance + _dis_x; \
            } \
        } else { \
            if (_dis_z < _dis_y) { \
                DATA_VAR.add_i = ivec3(_add_xy_i, COUNT_Z); \
                DATA_VAR.region_end = RAYTRACE_DATA.voxel_distance + _dis_z; \
            } else { \
                DATA_VAR.add_i = ivec3(_add_xz_i.x, COUNT_Y, _add_xz_i.y); \
                DATA_VAR.region_end = RAYTRACE_DATA.voxel_distance + _dis_y; \
            } \
        } \
    }

#define DO_TRAVERSE_REGION(RAYTRACE_DATA, DATA_VAR, IS_RAY_X_POSITIVE, IS_RAY_Y_POSITIVE, IS_RAY_Z_POSITIVE) \
    { \
        RAYTRACE_DATA.rayL += DATA_VAR.add_i * RAYTRACE_DATA.rayS; \
        RAYTRACE_DATA.pos += DATA_VAR.add_i * RAYTRACE_DATA.pos_step; \
        RAYTRACE_DATA.voxel_distance = DATA_VAR.region_end; \
    }

#define NOT_TRAVERSE_REGION_END(RAYTRACE_DATA, DATA_VAR) RAYTRACE_DATA.voxel_distance < DATA_VAR.region_end

#define EXTRACT_RAYTRACE_DISTANCE_AND_SIDE(RAYTRACE_DATA, RESULT_VAR, SIDE_VAR) \
    { \
        vec3 _lastRayL = RAYTRACE_DATA.rayL - RAYTRACE_DATA.rayS; \
        RESULT_VAR = max(_lastRayL.x, max(_lastRayL.y, _lastRayL.z)); \
        if (_lastRayL.x > _lastRayL.y) { \
            if (_lastRayL.x > _lastRayL.z) { \
                RESULT_VAR = _lastRayL.x; \
                SIDE_VAR = vec3(-RAYTRACE_DATA.pos_step.x, 0, 0); \
            } else { \
                RESULT_VAR = _lastRayL.z; \
                SIDE_VAR = vec3(0, 0, -RAYTRACE_DATA.pos_step.z); \
            } \
        } else { \
            if (_lastRayL.y > _lastRayL.z) { \
                RESULT_VAR = _lastRayL.y; \
                SIDE_VAR = vec3(0, -RAYTRACE_DATA.pos_step.y, 0); \
            } else { \
                RESULT_VAR = _lastRayL.z; \
                SIDE_VAR = vec3(0, 0, -RAYTRACE_DATA.pos_step.z); \
            } \
        } \
    }



#define MAIN_RAYTRACE_FUNC(FUNC_NAME, IS_RAY_X_POSITIVE, IS_RAY_Y_POSITIVE, IS_RAY_Z_POSITIVE) \
uint FUNC_NAME(in RaytraceData raytrace_data, int max_steps, int max_distance, out int steps_made, out float distance_to_voxel, out vec3 voxel_side) { \
    RegTraverseData tier1_region; \
    RegTraverseData tier2_region; \
    RegTraverseData chunk_region; \
    \
    int i = 0; \
    while(i < max_steps) { \
        ivec3 chunk_pos = raytrace_data.pos >> 7; \
        ivec3 chunk_pos_off = chunk_pos - CHUNK_OFFSET; \
        \
        if (raytrace_data.voxel_distance > max_distance) { \
            steps_made = i; \
            distance_to_voxel = max_distance; \
            return 0u; \
        } \
        \
        PREPARE_TO_TRAVERSE_REGION(raytrace_data, chunk_region, 128, 127, IS_RAY_X_POSITIVE, IS_RAY_Y_POSITIVE, IS_RAY_Z_POSITIVE); \
        /* if in bound of chunk, raytrace it */ \
        if (chunk_pos_off.x >= 0 && chunk_pos_off.y >= 0 && chunk_pos_off.z >= 0 && chunk_pos_off.x < CHUNK_COUNT.x && chunk_pos_off.y < CHUNK_COUNT.y && chunk_pos_off.z < CHUNK_COUNT.z) { \
            /* get buffer offset from chunk */ \
            int chunk_buffer_offset = CHUNK_DATA_OFFSETS_IN_BUFFER[chunk_pos_off.x + (chunk_pos_off.z + chunk_pos_off.y * CHUNK_COUNT.z) * CHUNK_COUNT.x]; \
            /* iterate while inside this chunk */ \
            while (i < max_steps) { \
                if (NOT_TRAVERSE_REGION_END(raytrace_data, chunk_region)) { \
                    /* raytrace over tier 2 region */ \
                    ivec3 r2pos = (raytrace_data.pos >> 4) & 7; \
                    int r2offset = chunk_buffer_offset + (r2pos.x | ((r2pos.z | (r2pos.y << 3)) << 3)) * 4161; \
                    \
                    if (raytrace_data.voxel_distance > max_distance) { \
                        steps_made = i; \
                        distance_to_voxel = max_distance; \
                        return 0u; \
                    } \
                    \
                    PREPARE_TO_TRAVERSE_REGION(raytrace_data, tier2_region, 16, 15, IS_RAY_X_POSITIVE, IS_RAY_Y_POSITIVE, IS_RAY_Z_POSITIVE); \
                    if (texelFetch(CHUNK_BUFFER, r2offset).r != 0u) { \
                        /* region is non-empty, raytrace over tier 1 */ \
                        while (NOT_TRAVERSE_REGION_END(raytrace_data, tier2_region)) { \
                            /* raytrace over tier 1 region */ \
                            ivec3 r1pos = (raytrace_data.pos >> 2) & 3; \
                            int r1offset = r2offset + 1 + (r1pos.x | ((r1pos.z | (r1pos.y << 2)) << 2)) * 65; \
                            \
                            PREPARE_TO_TRAVERSE_REGION(raytrace_data, tier1_region, 4, 3, IS_RAY_X_POSITIVE, IS_RAY_Y_POSITIVE, IS_RAY_Z_POSITIVE); \
                            if (texelFetch(CHUNK_BUFFER, r1offset).r != 0u) { \
                                /* region is non-empty, raytrace over voxels */ \
                                while (NOT_TRAVERSE_REGION_END(raytrace_data, tier1_region)) { \
                                    ivec3 voxel_pos = raytrace_data.pos & 3; \
                                    int voxel_index = r1offset + 1 + (voxel_pos.x | ((voxel_pos.z | (voxel_pos.y << 2)) << 2)); \
                                    uint voxel = texelFetch(CHUNK_BUFFER, voxel_index).r; \
                                    if (voxel != 0u) { \
                                        /* voxel found, end iteration */ \
                                        steps_made = i; \
                                        EXTRACT_RAYTRACE_DISTANCE_AND_SIDE(raytrace_data, distance_to_voxel, voxel_side); \
                                        return voxel; \
                                    } \
                                    /* make voxel step */ \
                                    RAYTRACE_DDA_STEP(raytrace_data, IS_RAY_X_POSITIVE, IS_RAY_Y_POSITIVE, IS_RAY_Z_POSITIVE); i++; \
                                } \
                            } else { \
                                /* region is empty, skip (make tier 1 step) */ \
                                DO_TRAVERSE_REGION(raytrace_data, tier1_region, IS_RAY_X_POSITIVE, IS_RAY_Y_POSITIVE, IS_RAY_Z_POSITIVE); i++; \
                            } \
                        } \
                    } else { \
                        /* region is empty, skip (make tier 2 step) */ \
                        DO_TRAVERSE_REGION(raytrace_data, tier2_region, IS_RAY_X_POSITIVE, IS_RAY_Y_POSITIVE, IS_RAY_Z_POSITIVE); i++; \
                    } \
                } else { \
                    i++; \
                    break; \
                } \
            } \
        } else { \
            DO_TRAVERSE_REGION(raytrace_data, chunk_region, IS_RAY_X_POSITIVE, IS_RAY_Y_POSITIVE, IS_RAY_Z_POSITIVE); i++; \
        } \
    } \
    EXTRACT_RAYTRACE_DISTANCE_AND_SIDE(raytrace_data, distance_to_voxel, voxel_side); \
    steps_made = max_steps; \
    return 0u; \
}

MAIN_RAYTRACE_FUNC(raytrace_next_px_py_pz, BINARY_CHOICE_AB_A, BINARY_CHOICE_AB_A, BINARY_CHOICE_AB_A)
MAIN_RAYTRACE_FUNC(raytrace_next_nx_py_pz, BINARY_CHOICE_AB_B, BINARY_CHOICE_AB_A, BINARY_CHOICE_AB_A)
MAIN_RAYTRACE_FUNC(raytrace_next_px_ny_pz, BINARY_CHOICE_AB_A, BINARY_CHOICE_AB_B, BINARY_CHOICE_AB_A)
MAIN_RAYTRACE_FUNC(raytrace_next_nx_ny_pz, BINARY_CHOICE_AB_B, BINARY_CHOICE_AB_B, BINARY_CHOICE_AB_A)
MAIN_RAYTRACE_FUNC(raytrace_next_px_py_nz, BINARY_CHOICE_AB_A, BINARY_CHOICE_AB_A, BINARY_CHOICE_AB_B)
MAIN_RAYTRACE_FUNC(raytrace_next_nx_py_nz, BINARY_CHOICE_AB_B, BINARY_CHOICE_AB_A, BINARY_CHOICE_AB_B)
MAIN_RAYTRACE_FUNC(raytrace_next_px_ny_nz, BINARY_CHOICE_AB_A, BINARY_CHOICE_AB_B, BINARY_CHOICE_AB_B)
MAIN_RAYTRACE_FUNC(raytrace_next_nx_ny_nz, BINARY_CHOICE_AB_B, BINARY_CHOICE_AB_B, BINARY_CHOICE_AB_B)


#define RAYTRACE_FUNC_CALL(RESULT_VAR, RAYTRACE_DATA, PARAMS) \
    if (RAYTRACE_DATA.ray.x > 0) { \
        if (RAYTRACE_DATA.ray.y > 0) { \
            if (RAYTRACE_DATA.ray.z > 0) { \
                RESULT_VAR = raytrace_next_px_py_pz PARAMS; \
            } else { \
                RESULT_VAR = raytrace_next_px_py_nz PARAMS; \
            } \
        } else { \
            if (RAYTRACE_DATA.ray.z > 0) { \
                RESULT_VAR = raytrace_next_px_ny_pz PARAMS; \
            } else { \
                RESULT_VAR = raytrace_next_px_ny_nz PARAMS; \
            } \
        } \
    } else { \
        if (RAYTRACE_DATA.ray.y > 0) { \
            if (RAYTRACE_DATA.ray.z > 0) { \
                RESULT_VAR = raytrace_next_nx_py_pz PARAMS; \
            } else { \
                RESULT_VAR = raytrace_next_nx_py_nz PARAMS; \
            } \
        } else { \
            if (RAYTRACE_DATA.ray.z > 0) { \
                RESULT_VAR = raytrace_next_nx_ny_pz PARAMS; \
            } else { \
                RESULT_VAR = raytrace_next_nx_ny_nz PARAMS; \
            } \
        } \
    }


uint raytrace_direct(vec3 start, vec3 ray, int max_steps, float max_distance, out int steps_made, out float distance_to_voxel, out vec3 voxel_side) {
    RaytraceData raytrace_data;
    raytrace_data.start = start;
    raytrace_data.ray = ray = normalize(ray);

    vec3 r2 = ray * ray;
    raytrace_data.rayS = vec3(
        sqrt(1.0 + r2.y / r2.x + r2.z / r2.x),
        sqrt(1.0 + r2.x / r2.y + r2.z / r2.y),
        sqrt(1.0 + r2.x / r2.z + r2.y / r2.z)
    );

    raytrace_data.pos = ivec3(floor(start));
    raytrace_data.pos_step = ivec3(ray.x > 0 ? 1 : -1, ray.y > 0 ? 1 : -1, ray.z > 0 ? 1 : -1);
    raytrace_data.distance = 0.0;

    raytrace_data.rayL = vec3(
            (ray.x > 0 ? (float(raytrace_data.pos.x + 1) - start.x) : (start.x - float(raytrace_data.pos.x))) * raytrace_data.rayS.x,
            (ray.y > 0 ? (float(raytrace_data.pos.y + 1) - start.y) : (start.y - float(raytrace_data.pos.y))) * raytrace_data.rayS.y,
            (ray.z > 0 ? (float(raytrace_data.pos.z + 1) - start.z) : (start.z - float(raytrace_data.pos.z))) * raytrace_data.rayS.z
        );

    uint result;

    RAYTRACE_FUNC_CALL(result, raytrace_data, (raytrace_data, max_steps, int(max_distance * (abs(ray.x) + abs(ray.y) + abs(ray.z))), steps_made, distance_to_voxel, voxel_side));
    return result;
}

void raytrace(
        vec3 start,
        vec3 ray,
        int max_steps,
        float max_distance,
        float max_distance_light,
        out vec3 out_color,
        out float out_depth,
        out vec3 out_light
    ) {
    ray = normalize(ray);

    int steps_made;
    float distance_to_voxel;
    vec3 voxel_normal;
    uint result = raytrace_direct(start, ray, max_steps, max_distance, steps_made, distance_to_voxel, voxel_normal);
    out_depth = distance_to_voxel;
    out_color = vec3(distance_to_voxel / 150.0);
    out_light = vec3(1.0);

    if (result == 0u) {
        out_color = vec3(0.5, 0.7, 1.0);
        out_light = vec3(1.0);
        return;
    }


    if (DIRECT_LIGHT_COLOR.a > 0.01) {
        uint packed_normal = result >> 8;
        float n_yaw = float(packed_normal >> 5u) / 63.0 * PI * 2.0 - PI;
        float n_pitch = float(packed_normal & 31u) / 31.0 * PI - PI / 2.0;
        float n_pitch_cos = cos(n_pitch);
        vec3 normal = vec3(cos(n_yaw) * n_pitch_cos, sin(n_pitch), sin(n_yaw) * n_pitch_cos);
        // vec3 normal = normalize(vec3(float(int(packed_normal >> 12) - 32), float(int((packed_normal >> 6) & 63u) - 32), float(int(packed_normal & 63u) - 32)));

        vec3 ray_end = start + ray * distance_to_voxel + voxel_normal * 0.1;
        vec3 light_dir = normalize(DIRECT_LIGHT_RAY);

        normal = mix(normal, voxel_normal, 0.5);

        float direct_light_intensity = raytrace_direct(ray_end, -light_dir, max_steps - steps_made, max_distance_light, steps_made, distance_to_voxel, voxel_normal) == 0u ? 1.0 : AMBIENT_LIGHT_COLOR.a;
        direct_light_intensity *= DIRECT_LIGHT_COLOR.a * (dot(-light_dir, normal) * 0.5 + 0.5);
        out_light = mix(AMBIENT_LIGHT_COLOR.rgb, DIRECT_LIGHT_COLOR.rgb, direct_light_intensity);

        // out_light = vec3(1.0);
        // out_color = normal * 0.5 + 0.5;// vec3(n_yaw / (2 * PI) + 0.5, n_pitch / (2 * PI) + 0.5, 0.0);
        // return;
    } else {
        out_light = AMBIENT_LIGHT_COLOR.rgb;
    }

    out_color = vec3(1.0);
}

void main() {
    vec3 start = CAMERA_POSITION;

    vec3 forward = normalize(CAMERA_RAY);
    vec3 right = vec3(forward.z, 0.0, -forward.x);
    vec3 up = cross(forward, right);
    vec2 xy = (uv - 0.5) * VIEWPORT.zw + VIEWPORT.xy;
    start += (xy.x * normalize(right) + xy.y * normalize(up));

    float max_depth = CAMERA_NEAR_AND_FAR.y - CAMERA_NEAR_AND_FAR.x;

    vec3 out_color;
    float out_depth;
    vec3 out_light;
    raytrace(
        start + forward * CAMERA_NEAR_AND_FAR.x, // start
        CAMERA_RAY,                              // ray
        200,                                     // max steps
        max_depth,                               // max distance
        100,                                     // max light distance
        out_color,                               // result color
        out_depth,                               // result depth
        out_light                                // result light color
    );
    gl_FragData[0] = vec4(out_color, 1.0);
    gl_FragData[1] = vec4(out_light, 1.0);
    gl_FragData[2] = vec4(vec3(out_depth / max_depth), 1.0);
    gl_FragDepth = out_depth / max_depth;
}