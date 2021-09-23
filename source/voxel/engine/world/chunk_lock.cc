#include <iostream>

#include "chunk_lock.h"
#include "voxel/engine/world/chunk.h"


namespace voxel {
namespace world {

class Chunk;

ChunkLock::ChunkLock(const std::vector<Shared<Chunk>>& chunks, bool owns_lock) : m_chunks(chunks), m_owns_lock(owns_lock) {
}

ChunkLock::ChunkLock(std::vector<Shared<Chunk>>&& chunks, bool owns_lock) : m_chunks(std::move(chunks)), m_owns_lock(owns_lock) {
}

ChunkLock::~ChunkLock() {
    if (m_owns_lock) {
        std::cerr << "ChunkLock was not released before destruction, this must not occur";
    }
}

bool ChunkLock::tryLock() {
    if (m_owns_lock) {
        return false;
    }

    i32 chunk_count = m_chunks.size();
    for (i32 i = 0; i < chunk_count; i++) {
        auto& chunk = m_chunks[i];
        if (!chunk->tryLock()) {
            for (i32 j = 0; j < i; i++) {
                m_chunks[i]->unlock();
            }
            return false;
        }
    }
    m_owns_lock = true;
    return true;
}

bool ChunkLock::isLockOwned() {
    return m_owns_lock;
}

void ChunkLock::unlock() {
    if (m_owns_lock) {
        for (auto& chunk : m_chunks) {
            chunk->unlock();
        }
    }
}

} // world
} // voxel