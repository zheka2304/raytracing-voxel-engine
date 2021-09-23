#ifndef VOXEL_ENGINE_CHUNK_LOCK_H
#define VOXEL_ENGINE_CHUNK_LOCK_H

#include <mutex>
#include <vector>
#include "voxel/common/base.h"


namespace voxel {
namespace world {

class Chunk;

class ChunkLock {
    std::vector<Shared<Chunk>> m_chunks;
    bool m_owns_lock = false;

public:
    ChunkLock(const std::vector<Shared<Chunk>>& chunks, bool owns_lock = false);
    ChunkLock(std::vector<Shared<Chunk>>&& chunks, bool owns_lock = false);
    ~ChunkLock();

    bool tryLock();
    bool isLockOwned();
    void unlock();
};

} // world
} // voxel

#endif //VOXEL_ENGINE_CHUNK_LOCK_H
