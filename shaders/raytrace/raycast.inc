

#ifndef VOXEL_ENGINE_RAYCAST_INC
#define VOXEL_ENGINE_RAYCAST_INC

#include "common.inc"
#include "raytrace/types.inc"


/*
[4096 bytes per page, no more than 1 chunk per page]
- starting with chunk root, each chunk is 1 or more continuous pages

2 i32 per voxel for voxels (tree leaves)
10 i32 per voxel for tree nodes

Structure of chunk memory span:
- root (3 i32)
- tree nodes span
	Can store both tree nodes and voxels, leaves are tree nodes, that lost all children, in this case it still occupies 10 i32
- voxel (leaf) span
	Reallocation is very heavy, all tree nodes must be iterated to find ones, pointing to leaves and update.
	Leaves, marked as empty, must be cleaned up during memory optimization pass.

Reallocation:
	1. Reallocate buffer to newly required size.
	2. Iterate over all tree nodes, for each tree node, check if it is pointing into old voxel (leaf) span, for each such node, add difference between old and new span offset to pointer
	3. Move voxel (leaf) span using `memmove`

Memory optimization pass:
	1. Allocate new buffer, equal in size to the current one
	2. Do a calculating pass: recursively determine memory for remaining tree nodes and voxels (leaves)
	3. Do copying pass: recursively copy remaining tree nodes and voxels (leaves)
*/

// contains all voxel data, indexed
COMPUTE_SHADER_BUFFER(${raytrace.voxel_buffer}, readonly, u_voxel_buffer_t, u_voxel_buffer, {
    // each voxel tree node contains following:
    // 0) 0-29 bits are voxel color (10 bits per component), then 2 flags: is voxel empty, does voxel have children
    // 1) some additional data
    // 2-9) relative pointers to child voxels for idx 0-7 (if has children), pointers are given relative to bit #0 in voxel, if pointer is 0 - voxel is empty
    //
    // chunk root structure:
    // 0) 0xC0000000
    // 1) reserved
    // 2) pointer to root voxel
    uint data[];
})

// hashmap for chunk lookup (size must be power of 2)
#define VOXEL_CHUNK_LOOKUP_MAP_SIZE ${raytrace.voxel_chunk_map_size}u
#define VOXEL_CHUNK_LOOKUP_MAP_UNIT_CAPACITY ${raytrace.voxel_chunk_map_unit_capacity}u

COMPUTE_SHADER_BUFFER(${raytrace.voxel_chunk_map}, readonly, u_voxel_chunk_map_t, u_voxel_chunk_map, {
    uint data[];
})


uint lookup_chunk(ivec3 position) {
    uvec3 u_pos = uvec3(position);
    return position.x == 0 && position.y == 0 && position.z == 0 ? 1u : uint(-1);

    uint map_index = ((u_pos.x * 93911u) ^ (u_pos.y * 263167u) ^ (u_pos.z * 32993u)) & (VOXEL_CHUNK_LOOKUP_MAP_SIZE - 1u);
    map_index *= VOXEL_CHUNK_LOOKUP_MAP_UNIT_CAPACITY;

    // get first 10 bits for each axis and pack them into int
    uint hash = (u_pos.x & 0x3FFu) | ((u_pos.y & 0x3FFu) << 10u) | ((u_pos.z & 0x3FFu) << 20u) | 0x80000000u;

    for (uint i = 0u; i < VOXEL_CHUNK_LOOKUP_MAP_UNIT_CAPACITY; i++) {
        uint h = u_voxel_chunk_map.data[map_index << 2u];
        if (h == hash) {
            return u_voxel_chunk_map.data[(map_index << 2u) | 1u];
        } else if (h == 0u) {
            return 0u;
        }
        map_index++;
    }

    return 0u;
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
    uint voxel_pointer;
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
    // p(t) = p + t * d
    // d = ray.ray, p = ray.start;
    // t_v(v) = (1 / d) * v + (-p / d)
    // t_v(v) = t_coef * v + t_bias;
    vec3 t_coef = 1.0 / -abs(ray.ray);
    vec3 t_bias = -ray.start / ray.ray;

    int octant_mask = 7;
    if (ray.ray.x > 0.0) { octant_mask ^= 1; chunk_offset.x = -(chunk_offset.x + 0.5) - 0.5; }
    if (ray.ray.y > 0.0) { octant_mask ^= 2; chunk_offset.y = -(chunk_offset.y + 0.5) - 0.5; }
    if (ray.ray.z > 0.0) { octant_mask ^= 4; chunk_offset.z = -(chunk_offset.z + 0.5) - 0.5; }

    Stack stack;
    stack.size = 0;

    Cube cube;
    cube.pos = chunk_offset;
    cube.scale_exp = 1.0;
    cube.scale_i = 23;

    vec3 t_start_v = (cube.pos + cube.scale_exp) * t_coef + t_bias;
    vec3 t_end_v = cube.pos * t_coef + t_bias;
    float t_min = max(0.0, max(t_start_v.x, max(t_start_v.y, t_start_v.z)));
    float t_max = min(t_end_v.x, min(t_end_v.y, t_end_v.z));
    int idx = octant_mask;
    uint voxel_pointer = chunk_index;

    int overflow = 0;
    bool skip_push = false;

    while (overflow++ < 1000) {
        // t value of cube corner
        vec3 t_corner_v = cube.pos * t_coef + t_bias;
        float t_corner_max = min(t_corner_v.x, min(t_corner_v.y, t_corner_v.z));

        uint voxel_header = u_voxel_buffer.data[voxel_pointer];
        if ((voxel_header & 0x40000000u) != 0u) {
        // if ((idx ^ octant_mask) == 3 && stack.size > 0) {
            float k = float(stack.size + 1) / 7.0;
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
                    return debugRaycast(vec3(1.0, 1.0, 0.0));
                }

                if ((voxel_header & 0x80000000u) != 0u) {
                    uint child = u_voxel_buffer.data[voxel_pointer + 2u + uint(idx ^ octant_mask)];
                    if (child != 0u) {
                        stack.nodes[stack.size++] = StackNode(voxel_pointer, t_max, cube, idx);
                        voxel_pointer += child;

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
        }
        skip_push = false;

        // ADVANCE
        int step_mask = 0;
        if (t_corner_v.x <= t_corner_max) { step_mask ^= 1; cube.pos.x -= cube.scale_exp; }
        if (t_corner_v.y <= t_corner_max) { step_mask ^= 2; cube.pos.y -= cube.scale_exp; }
        if (t_corner_v.z <= t_corner_max) { step_mask ^= 4; cube.pos.z -= cube.scale_exp; }
        t_min = t_corner_max;
        idx ^= step_mask;

        if ((idx & step_mask) != 0) {
            if (stack.size <= 0) {
                break;
            }
            // POP
            StackNode node = stack.nodes[--stack.size];
            voxel_pointer = node.voxel_pointer;
            t_max = node.t_max;
            idx = node.idx;
            cube = node.cube;
            skip_push = true;
        }
    }

    return debugRaycast(vec3(1.0, 0.0, 1.0) * float(overflow) / 64.0);
}

vec3 raycastVoxelWorld(Ray ray, bool request_cache_updates) {
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

    for (int i = 0; i < 50; i++) {
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