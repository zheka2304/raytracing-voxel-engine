

#ifndef VOXEL_ENGINE_RAYCAST_INC
#define VOXEL_ENGINE_RAYCAST_INC

#include "common.inc"
#include "raytrace/types.inc"


// contains all voxel data, indexed
COMPUTE_SHADER_BUFFER(${raytrace.voxel_buffer}, writeonly, u_voxel_buffer_t, u_voxel_buffer, {
    uint data[];
})

// hashmap for chunk lookup (size must be power of 2)
#define VOXEL_CHUNK_LOOKUP_MAP_SIZE ${raytrace.voxel_chunk_map_size}
#define VOXEL_CHUNK_LOOKUP_MAP_UNIT_CAPACITY ${raytrace.voxel_chunk_map_unit_capacity}

COMPUTE_SHADER_BUFFER(${raytrace.voxel_chunk_map}, writeonly, u_voxel_chunk_map_t, u_voxel_chunk_map, {
    uint data[];
})


uint lookup_chunk(ivec3 position) {
    uint hash = ((uint(position.x) * 93911u) ^ (uint(position.y) * 263167u) ^ (uint(position.z) * 32993u)) & uint(VOXEL_CHUNK_LOOKUP_MAP_SIZE - 1);
    return hash == 0u ? 0u : uint(-1);
}

struct ChunkRaycastData {
    ivec3 pos;
    ivec3 pos_step;
    vec3 rayL;
    vec3 rayS;
};

struct Cube {
    vec3 pos;
    float scale_exp;
    int scale_i;
};

struct StackNode {
    int parent;
    float t_max;
    Cube cube;
    int idx;
};

struct Stack {
    int size;
    StackNode nodes[32];
};


RaycastResult debugRaycast(vec3 color) {
    RaycastResult r;
    r.color = color;
    r.voxel = 1u;
    return r;
}

RaycastResult debugRaycastNoHit() {
    RaycastResult r;
    r.color = vec3(0.0);
    r.voxel = 0u;
    return r;
}

RaycastResult raycastVoxelChunk(Ray ray, vec3 chunk_offset, uint chunk_index) {
    if (ray.ray.x >= 0.0 || ray.ray.y >= 0.0 || ray.ray.z >= 0.0) return debugRaycast(vec3(.5, 0.0, 0.0));

    // ray.start = vec3(0.0);

    // p(t) = p + t * d
    // d = ray.ray, p = ray.start;
    // t_v(v) = (1 / d) * v + (-p / d)
    // t_v(v) = t_coef * v + t_bias;
    vec3 t_coef = 1.0 / -abs(ray.ray);
    vec3 t_bias = -ray.start * t_coef;

    int octant_mask = 7;
    if (ray.ray.x > 0.0) { octant_mask ^= 1; /* t_bias.x = 3.0 * t_coef.x - t_bias.x; */ }
    if (ray.ray.y > 0.0) { octant_mask ^= 2; /* t_bias.y = 3.0 * t_coef.y - t_bias.y; */ }
    if (ray.ray.z > 0.0) { octant_mask ^= 4; /* t_bias.z = 3.0 * t_coef.z - t_bias.z; */ }

    Stack stack;
    stack.size = 0;

    Cube cube;
    cube.pos = chunk_offset; //vec3(-10.0);
    cube.scale_exp = 1.0;
    cube.scale_i = 23;

    vec3 t_end_v = cube.pos * t_coef + t_bias;
    float t, t_min = 0.0, t_max = min(t_end_v.x, min(t_end_v.y, t_end_v.z));
    int idx = 0;

    // return vec3(t_min, t_max, 0.0) / 10.0;

    int overflow = 0;
    bool skip_push = false;

    while (overflow++ < 1000) {
        // t value of cube corner
        vec3 t_corner_v = cube.pos * t_coef + t_bias;
        float t_corner_max = min(t_corner_v.x, min(t_corner_v.y, t_corner_v.z));

        if ((idx ^ octant_mask) == 3) {
            float k = float(overflow) / 5.0;
            vec3 t_side_v = (cube.pos + cube.scale_exp) * t_coef + t_bias;
            float t_side_min = max(t_side_v.x, max(t_side_v.y, t_side_v.z));
            if (t_side_v.x >= t_side_min) return debugRaycast(vec3(1.0, 0.0, 0.0) * k);
            if (t_side_v.y >= t_side_min) return debugRaycast(vec3(0.0, 1.0, 0.0) * k);
            if (t_side_v.z >= t_side_min) return debugRaycast(vec3(0.0, 0.0, 1.0) * k);
        }

        if (!skip_push && t_min < t_max) {
            float t_voxel_max = min(t_max, t_corner_max);
            float half_scale = cube.scale_exp * 0.5f;
            vec3 t_center_v = (cube.pos + cube.scale_exp * .5) * t_coef + t_bias; //half_scale * t_coef + t_corner_v;
            float t_center_max = min(t_center_v.x, min(t_center_v.y, t_center_v.z));

            if (t_min < t_voxel_max) {
                // PUSH
                if (stack.size > 20) {
                    return debugRaycast(vec3(0.0, 1.0, 0.0));
                }

                if (stack.size < 50 && (overflow == 1 || (idx ^ octant_mask) == 5 || (idx ^ octant_mask) == 6)) {
                    stack.nodes[stack.size++] = StackNode(0, t_max, cube, idx);

                    idx = 0;
                    cube.scale_i--;
                    cube.scale_exp = half_scale;

                    if (t_center_v.x > t_min) { idx ^= 1; cube.pos.x += cube.scale_exp; }
                    if (t_center_v.y > t_min) { idx ^= 2; cube.pos.y += cube.scale_exp; }
                    if (t_center_v.z > t_min) { idx ^= 4; cube.pos.z += cube.scale_exp; }

                    t_max = t_voxel_max;
                    continue;
                }
            }
        }
        skip_push = false;

        // ADVANCE
        int step_mask = 0;
        if (t_corner_v.x <= t_corner_max) { step_mask ^= 1; cube.pos.x -= cube.scale_exp; }
        if (t_corner_v.y <= t_corner_max) { step_mask ^= 2; cube.pos.y -= cube.scale_exp; }
        if (t_corner_v.z <= t_corner_max) { step_mask ^= 4; cube.pos.z -= cube.scale_exp; }

        t_min = t_corner_max;
        idx ^= step_mask;

        // POP
        if ((idx & step_mask) != 0) {
            if (stack.size <= 0) {
                break;
            }
            // POP
            StackNode node = stack.nodes[--stack.size];
            t_max = node.t_max;
            idx = node.idx;
            cube = node.cube;
            skip_push = true;
        }
    }

    return debugRaycastNoHit(); //debugRaycast(vec3(1.0, 0.0, 1.0) * float(overflow) / 64.0);
}

vec3 raycastVoxelWorld(Ray ray, bool request_cache_updates) {
    // return raycastVoxelChunk(ray, vec3(-1.0), 0u).color;

    ChunkRaycastData chunk_raycast;

    vec3 r2 = ray.ray * ray.ray;
    chunk_raycast.rayS = vec3(
        sqrt(1.0 + r2.y / r2.x + r2.z / r2.x),
        sqrt(1.0 + r2.x / r2.y + r2.z / r2.y),
        sqrt(1.0 + r2.x / r2.z + r2.y / r2.z)
    );

    chunk_raycast.pos = ivec3(floor(ray.start));
    chunk_raycast.pos_step = ivec3(ray.ray.x > 0.0 ? 1 : -1, ray.ray.y > 0.0 ? 1 : -1, ray.ray.z > 0.0 ? 1 : -1);

    chunk_raycast.rayL = vec3(
            (ray.ray.x > 0.0 ? (float(chunk_raycast.pos.x + 1) - ray.start.x) : (ray.start.x - float(chunk_raycast.pos.x))) * chunk_raycast.rayS.x,
            (ray.ray.y > 0.0 ? (float(chunk_raycast.pos.y + 1) - ray.start.y) : (ray.start.y - float(chunk_raycast.pos.y))) * chunk_raycast.rayS.y,
            (ray.ray.z > 0.0 ? (float(chunk_raycast.pos.z + 1) - ray.start.z) : (ray.start.z - float(chunk_raycast.pos.z))) * chunk_raycast.rayS.z
        );

    for (int i = 0; i < 100; i++) {
        if (lookup_chunk(chunk_raycast.pos) != uint(-1)) {
            RaycastResult chunk_result = raycastVoxelChunk(ray, vec3(chunk_raycast.pos), 0u);
            if (chunk_result.voxel != 0u) {
                return chunk_result.color;
            }
        }

        bvec3 mask = lessThanEqual(chunk_raycast.rayL, min(chunk_raycast.rayL.yzx, chunk_raycast.rayL.zxy));
        chunk_raycast.rayL += vec3(mask) * chunk_raycast.rayS;
        chunk_raycast.pos += ivec3(mask) * chunk_raycast.pos_step;
    }

    return vec3(1.0);

}

#endif  // VOXEL_ENGINE_RAYCAST_INC