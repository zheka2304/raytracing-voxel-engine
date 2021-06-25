

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

    for (int i = 0; i < 200; i++) {
        if (lookup_chunk(chunk_raycast.pos) != uint(-1)) {
            return vec3(0.0);
        }

        bvec3 mask = lessThanEqual(chunk_raycast.rayL, min(chunk_raycast.rayL.yzx, chunk_raycast.rayL.zxy));
        chunk_raycast.rayL += vec3(mask) * chunk_raycast.rayS;
        chunk_raycast.pos += ivec3(mask) * chunk_raycast.pos_step;
    }

    return vec3(1.0);
}

#endif  // VOXEL_ENGINE_RAYCAST_INC