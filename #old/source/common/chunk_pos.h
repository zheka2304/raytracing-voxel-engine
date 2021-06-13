
#ifndef VOXEL_ENGINE_CHUNK_POS_H
#define VOXEL_ENGINE_CHUNK_POS_H

#include <unordered_map>
#include "common/vec.h"


class ChunkPos {
public:
    static const int CHUNK_SIZE = 128;

    int x, y, z;
    ChunkPos();
    ChunkPos(int x, int y, int z);
    ChunkPos(ChunkPos const& other);
    explicit ChunkPos(Vec3 v);
    bool operator==(ChunkPos const& other) const;
};

namespace std {
    template <>
    struct hash<ChunkPos> {
        std::size_t operator()(const ChunkPos& k) const {
            return k.x ^ (k.y << 1) ^ (k.z << 2);
        }
    };
}

#endif //VOXEL_ENGINE_CHUNK_POS_H
