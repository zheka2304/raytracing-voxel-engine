#ifndef VOXEL_ENGINE_CHUNK_POSITION_H
#define VOXEL_ENGINE_CHUNK_POSITION_H

#include "voxel/common/base.h"


namespace voxel {

struct ChunkPosition {
    i32 x, y, z;

    inline ChunkPosition() = default;
    inline ChunkPosition(i32 x, i32 y, i32 z) : x(x), y(y), z(z) {};

    inline bool operator==(const ChunkPosition& other) const {
        return x == other.x && y == other.y && z == other.z;
    }
};

} // voxel

namespace std {
template<>
struct hash<voxel::ChunkPosition> {
    std::size_t operator()(const voxel::ChunkPosition& p) const {
        using namespace voxel;
        const i32 k = 1103515245;
        return ((u64(p.x) * k + (u64(p.y << 1) ^ u64(p.z << 2))) * k) ^ ((u64(p.z) * k + (u64(p.x << 1) ^ u64(p.y << 2))) * k);
    }
};
} // std
#endif //VOXEL_ENGINE_CHUNK_POSITION_H
