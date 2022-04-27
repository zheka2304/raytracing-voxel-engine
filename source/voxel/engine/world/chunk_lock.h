#ifndef VOXEL_ENGINE_CHUNK_LOCK_H
#define VOXEL_ENGINE_CHUNK_LOCK_H

#include <mutex>
#include <vector>
#include "voxel/common/base.h"
#include "voxel/engine/world/chunk.h"

namespace voxel {

class ChunkSource;

class ChunkLock {
    Shared<ChunkSource> m_chunk_source;
    std::vector<ChunkRef> m_chunks;
    std::vector<Chunk*> m_owned_chunks;

public:
    ChunkLock(const Shared<ChunkSource>& chunk_source, const std::vector<ChunkRef>& chunks);
    ChunkLock(const Shared<ChunkSource>& chunk_source, std::vector<ChunkRef>&& chunks);
    ~ChunkLock();

    bool tryLock();
    bool isLockOwned();
    void unlock();

    inline const auto& owned_chunks() { return m_owned_chunks; }
};

} // voxel

#endif //VOXEL_ENGINE_CHUNK_LOCK_H
