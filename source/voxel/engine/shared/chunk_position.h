#ifndef VOXEL_ENGINE_CHUNK_POSITION_H
#define VOXEL_ENGINE_CHUNK_POSITION_H

#include "voxel/common/base.h"


namespace voxel {

struct ChunkPosition {
    i32 x, y, z;

    inline constexpr ChunkPosition() : x(0), y(0), z(0) {};
    inline constexpr ChunkPosition(i32 x, i32 y, i32 z) : x(x), y(y), z(z) {};

    inline bool constexpr operator==(const ChunkPosition& other) const {
        return x == other.x && y == other.y && z == other.z;
    }

    inline bool constexpr operator!=(const ChunkPosition& other) const {
        return !this->operator==(other);
    }

    inline static constexpr ChunkPosition invalid() { return ChunkPosition( 0x7fffffff, 0x7fffffff, 0x7fffffff ); }
};

} // voxel

template<>
struct std::hash<voxel::ChunkPosition> {
    std::size_t operator()(const voxel::ChunkPosition& p) const {
        using namespace voxel;
        const i32 k = 1103515245;
        return ((u64(p.x) * k + (u64(p.y << 1) ^ u64(p.z << 2))) * k) ^ ((u64(p.z) * k + (u64(p.x << 1) ^ u64(p.y << 2))) * k);
    }
};
#endif //VOXEL_ENGINE_CHUNK_POSITION_H
