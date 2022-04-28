#include <iostream>

#include "chunk_lock.h"
#include "voxel/engine/world/chunk.h"
#include "voxel/engine/world/chunk_source.h"


namespace voxel {

class Chunk;

ChunkLock::ChunkLock(const Shared<ChunkSource>& chunk_source, const std::vector<ChunkRef>& chunks) : m_chunk_source(chunk_source), m_chunks(chunks) {
}

ChunkLock::ChunkLock(const Shared<ChunkSource>& chunk_source, std::vector<ChunkRef>&& chunks) : m_chunk_source(chunk_source), m_chunks(std::move(chunks)) {
}

ChunkLock::~ChunkLock() {
    VOXEL_ENGINE_ASSERT(m_owned_chunks.empty());
}

bool ChunkLock::tryLock() {
    ChunkSource& chunk_source = *m_chunk_source;
    m_owned_chunks.clear();
    m_owned_chunks.reserve(m_chunks.size());

    for (i32 i = 0; i < m_chunks.size(); i++) {
        auto& chunk_ref = m_chunks[i];
        chunk_source.accessChunk<chunk_access_policy_map_only>(chunk_ref, [&](Chunk& chunk) {
            if (chunk.tryLock()) {
                m_owned_chunks.emplace_back(std::addressof(chunk));
            } else {
                for (Chunk* owned : m_owned_chunks) {
                    owned->unlock();
                }
                m_owned_chunks.clear();
            };
        });
    }

    return true;
}

bool ChunkLock::isLockOwned() {
    return m_owned_chunks.size() > 0;
}

void ChunkLock::unlock() {
    for (auto& chunk : m_owned_chunks) {
        chunk->unlock();
    }
}

} // voxel