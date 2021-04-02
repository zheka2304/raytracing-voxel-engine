#version 330 core

varying vec2 uv;

#define PI 3.1415926

// settings

#define HIGHP_RAYTRACING

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
    float distance; // total distance
    ivec3 pos; // cell position
};

#define RAYTRACE_DDA_STEP_OVER(RAYTRACE_DATA, END_DIS) { \
        RAYTRACE_DATA.distance += (END_DIS); \
        vec3 _end_pos = RAYTRACE_DATA.distance * RAYTRACE_DATA.ray + RAYTRACE_DATA.start; \
        RAYTRACE_DATA.pos = ivec3(floor(_end_pos)); \
    }

#define RAYTRACE_DDA_STEP_OVER_AUTO(RAYTRACE_DATA, REGION_SIZE) { \
        float _end_dis = 1e-1 + get_end_region_distance(raytrace_data.start + raytrace_data.distance * raytrace_data.ray, raytrace_data.ray, raytrace_data.rayS, REGION_SIZE); \
        RAYTRACE_DDA_STEP_OVER(raytrace_data, _end_dis); \
    }


// returns distance from position to the end of given region
float get_end_region_distance(vec3 start, vec3 ray, vec3 rayS, float region_size) {
    vec3 region_offset = floor(start / region_size) * region_size;
    vec3 dis = vec3(
        (ray.x > 0 ? (region_offset.x + region_size - start.x) : (start.x - region_offset.x)) * rayS.x,
        (ray.y > 0 ? (region_offset.y + region_size - start.y) : (start.y - region_offset.y)) * rayS.y,
        (ray.z > 0 ? (region_offset.z + region_size - start.z) : (start.z - region_offset.z)) * rayS.z
    );
    return min(dis.x, min(dis.y, dis.z)) + 5e-6;
}

uint raytrace_next(RaytraceData raytrace_data, int max_steps, float max_distance, out int steps_made, out float distance_to_voxel) {
    float end_region_distance_chunk = get_end_region_distance(raytrace_data.start, raytrace_data.ray, raytrace_data.rayS, 128.0);
    float end_region_distance_tier2 = get_end_region_distance(raytrace_data.start, raytrace_data.ray, raytrace_data.rayS, 16.0);
    float end_region_distance_tier1 = get_end_region_distance(raytrace_data.start, raytrace_data.ray, raytrace_data.rayS, 4.0);

    int i = 0;
    // chunk level
    while(i < max_steps) {
        ivec3 chunk_pos = raytrace_data.pos >> 7;
        ivec3 chunk_pos_off = chunk_pos - CHUNK_OFFSET;

        if (raytrace_data.distance > max_distance) {
            steps_made = i;
            distance_to_voxel = max_distance;
            return 0u;
        }

        // if in bound of chunk, raytrace it
        if (chunk_pos_off.x >= 0 && chunk_pos_off.y >= 0 && chunk_pos_off.z >= 0 && chunk_pos_off.x < CHUNK_COUNT.x && chunk_pos_off.y < CHUNK_COUNT.y && chunk_pos_off.z < CHUNK_COUNT.z) {
            // get buffer offset from chunk
            int chunk_buffer_offset = CHUNK_DATA_OFFSETS_IN_BUFFER[chunk_pos_off.x + (chunk_pos_off.z + chunk_pos_off.y * CHUNK_COUNT.z) * CHUNK_COUNT.z];
            // iterate while inside this chunk
            while (i < max_steps) {
                if (raytrace_data.distance < end_region_distance_chunk) {
                    // raytrace over tier 2 region
                    ivec3 r2pos = (raytrace_data.pos >> 4) & 7;
                    int r2offset = chunk_buffer_offset + (r2pos.x | ((r2pos.z | (r2pos.y << 3)) << 3)) * 4161; //

                    if (raytrace_data.distance > max_distance) {
                        steps_made = i;
                        distance_to_voxel = max_distance;
                        return 0u;
                    }

                    if (texelFetch(CHUNK_BUFFER, r2offset).r != 0u) {
                        // region is non-empty, raytrace over tier 1
                        while (i < max_steps) {
                            // if in bound of tier 2 region
                            if (raytrace_data.distance < end_region_distance_tier2) {
                                // raytrace over tier 1 region
                                ivec3 r1pos = (raytrace_data.pos >> 2) & 3;
                                int r1offset = r2offset + 1 + (r1pos.x | ((r1pos.z | (r1pos.y << 2)) << 2)) * 65; //
                                if (texelFetch(CHUNK_BUFFER, r1offset).r != 0u) {
                                    // region is non-empty, raytrace over voxels
                                    while (i < max_steps) {
                                        if (raytrace_data.distance < end_region_distance_tier1) {
                                            ivec3 voxel_pos = raytrace_data.pos & 3;
                                            int voxel_index = r1offset + 1 + (voxel_pos.x | ((voxel_pos.z | (voxel_pos.y << 2)) << 2));
                                            uint voxel = texelFetch(CHUNK_BUFFER, voxel_index).r;
                                            if (voxel != 0u) {
                                                // voxel found, end iteration
                                                steps_made = i;
                                                distance_to_voxel = raytrace_data.distance;
                                                return voxel;
                                            }
                                            // make voxel step
                                            RAYTRACE_DDA_STEP_OVER_AUTO(raytrace_data, 1); i++;
                                        } else {
                                            i++;
                                            break;
                                        }
                                    }
                                    // return 2;
                                } else {
                                    // region is empty, skip (make tier 1 step)
                                    RAYTRACE_DDA_STEP_OVER_AUTO(raytrace_data, 4); i++;
                                }
                                // both if branches will reach next tier 1 region, so
                                // init for next tier 1 region
                                end_region_distance_tier1 = raytrace_data.distance + get_end_region_distance(raytrace_data.start + raytrace_data.distance * raytrace_data.ray, raytrace_data.ray, raytrace_data.rayS, 4.0);
                            } else {
                                i++;
                                break;
                            }
                        }
                    } else {
                        // region is empty, skip (make tier 2 step)
                        RAYTRACE_DDA_STEP_OVER_AUTO(raytrace_data, 16); i++;
                    }
                    // both if branches will reach next tier 2 region, so
                    // init for next tier 2 region
                    end_region_distance_tier2 = raytrace_data.distance + get_end_region_distance(raytrace_data.start + raytrace_data.distance * raytrace_data.ray, raytrace_data.ray, raytrace_data.rayS, 16.0);
                } else {
                    i++;
                    break;
                }
            }
        } else {
            RAYTRACE_DDA_STEP_OVER_AUTO(raytrace_data, 128); i++;
        }

        // find next chunk bound
        end_region_distance_chunk = raytrace_data.distance + get_end_region_distance(raytrace_data.start + raytrace_data.distance * raytrace_data.ray, raytrace_data.ray, raytrace_data.rayS, 128.0);
    }
    distance_to_voxel = raytrace_data.distance;
    steps_made = max_steps;
    return 0u;
}

uint raytrace_direct(vec3 start, vec3 ray, int max_steps, float max_distance, out int steps_made, out float distance_to_voxel) {
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
    raytrace_data.distance = 0.0;

    return raytrace_next(raytrace_data, max_steps, max_distance, steps_made, distance_to_voxel);
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
    uint result = raytrace_direct(start, ray, max_steps, max_distance, steps_made, distance_to_voxel);
    // return steps_made > 50 ? vec3(1.0, 0.0, 0.0) : vec3(steps_made / 50.0);
    out_depth = distance_to_voxel;

    if (result == 0u) {
        out_color = vec3(0.5, 0.7, 1.0);
        out_light = vec3(1.0);
        return;
    }

    vec3 ray_end = floor(start + ray * (distance_to_voxel - 1.0)) + 0.5;
    vec3 light_dir = normalize(DIRECT_LIGHT_RAY);

    if (DIRECT_LIGHT_COLOR.a > 0.01) {
        uint packed_normal = result >> 8;
        uint nx = (packed_normal >> 16) & 0xFFu;
        uint ny = (packed_normal >> 8) & 0xFFu;
        uint nz = (packed_normal >> 0) & 0xFFu;
        vec3 normal = vec3(float(nx), float(ny), float(nz)) / 127.0 - 1.0;

        float direct_light_intensity = raytrace_direct(ray_end, -light_dir, max_steps - steps_made, max_distance_light, steps_made, distance_to_voxel) == 0u ? 1.0 : AMBIENT_LIGHT_COLOR.a;
        direct_light_intensity *= DIRECT_LIGHT_COLOR.a * (dot(light_dir, normal) * 0.5 + 0.5);
        out_light = mix(AMBIENT_LIGHT_COLOR.rgb, DIRECT_LIGHT_COLOR.rgb, direct_light_intensity);
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
        75,                                      // max light distance
        out_color,                               // result color
        out_depth,                               // result depth
        out_light                                // result light color
    );
    gl_FragData[0] = vec4(out_color, 1.0);
    gl_FragData[1] = vec4(out_light, 1.0);
    gl_FragData[2] = vec4(vec3(out_depth / max_depth), 1.0);
    gl_FragDepth = out_depth / max_depth;
}