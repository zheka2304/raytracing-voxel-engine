#include "chunk_source.h"

namespace voxel {
namespace world {

ChunkSource::ChunkSource(Shared<ChunkProvider> provider, Shared<ChunkStorage> storage, Shared<threading::TaskExecutor> executor) :
    m_provider(provider), m_storage(storage), m_executor(executor) {
}

ChunkSource::~ChunkSource() {
}

void ChunkSource::handleChunk(Shared<Chunk> chunk) {
    ChunkState state = chunk->getState();
    if (state == CHUNK_LOADED) {
        // chunk is loaded and should be checked to put into lazy state
        handleLoadedChunk(chunk);
    } else if (state == CHUNK_LAZY) {
        // chunk is in lazy state, it can be instantly changed to loaded or start unloading sequence
        handleLazyChunk(chunk);
    } else if (state == CHUNK_UNLOADING) {
        // chunk will not change its state and waiting for unload task, queue it for this chunk
        m_executor->queue([=] () -> void {
            runChunkUnload(chunk);
        });
    } else if (state != CHUNK_FINALIZED) {
        // chunk is in some of loading states and must execute next loading task
        m_executor->queue([=] () -> void {
            handleChunkStateTask(chunk);
        });
    }
}

void ChunkSource::handleLoadedChunk(Shared<Chunk> chunk) {

}

void ChunkSource::handleLazyChunk(Shared<Chunk> chunk) {

}

void ChunkSource::handleChunkStateTask(Shared<Chunk> chunk) {
    if (chunk->tryLock()) {
        ChunkState state = chunk->getState();
        if (state == CHUNK_PENDING) {
            runChunkBuild(chunk);
        } else if (state == CHUNK_BUILT) {
            runChunkProcessing(chunk);
        } else if (state == CHUNK_PROCESSED) {
            runChunkLoad(chunk);
        }
        chunk->unlock();
    }
}

void ChunkSource::runChunkBuild(Shared<Chunk> chunk) {
    if (chunk->tryLock()) {
        chunk->setState(CHUNK_BUILT);
        chunk->unlock();
    }
}

void ChunkSource::runChunkProcessing(Shared<Chunk> chunk) {
    if (chunk->tryLock()) {
        chunk->setState(CHUNK_PROCESSED);
        chunk->unlock();
    }

}

void ChunkSource::runChunkLoad(Shared<Chunk> chunk) {
    if (chunk->tryLock()) {
        chunk->setState(CHUNK_LOADED);
        chunk->unlock();
    }
}

void ChunkSource::runChunkUnload(Shared<Chunk> chunk) {
    if (chunk->tryLock()) {
        ThreadLock lock(m_chunks_mutex);
        m_chunks.erase(chunk->getPosition());
        lock.unlock();

        chunk->deleteAllBuffers();
        chunk->setState(CHUNK_FINALIZED);
        chunk->unlock();
    }
}

void ChunkSource::onTick() {
    if (m_executor->getEstimatedRemainingTasks() < m_chunks.size()) {
        ThreadLock lock(m_chunks_mutex);
        for (auto& p : m_chunks) {
            Shared<Chunk> chunk = p.second;
            if (chunk->tryLock()) {
                handleChunk(chunk);
                chunk->unlock();
            }
        }
    }
}

Shared<Chunk> ChunkSource::getChunkAt(ChunkPosition position) {
    ThreadLock lock(m_chunks_mutex);
    auto it = m_chunks.find(position);
    if (it != m_chunks.end()) {
        return it->second;
    } else {
        return Shared<Chunk>(nullptr);
    }
}

void ChunkSource::fetchChunkAt(ChunkPosition position) {
    Shared<Chunk> chunk = getChunkAt(position);
    if (chunk) {
        chunk->fetch();
    }
}

} // world
} // voxel