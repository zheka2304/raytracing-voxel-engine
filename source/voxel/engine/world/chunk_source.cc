#include "chunk_source.h"

namespace voxel {

ChunkSource::ChunkSource(
        Shared<ChunkProvider> provider,
        Shared<ChunkStorage> storage,
        Shared<threading::TaskExecutor> executor,
        ChunkSourceState initial_state) :
    m_provider(provider), m_storage(storage), m_executor(executor), m_state(initial_state) {
}

ChunkSource::~ChunkSource() {
}

ChunkSource::ChunkSourceState ChunkSource::getState() {
    return m_state;
}

void ChunkSource::setState(ChunkSourceState state) {
    if (state != m_state) {
        m_state = state;
    }
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
            if (chunk->tryLock()) {
                // if it is still unloading
                if (chunk->getState() == CHUNK_UNLOADING) {
                    runChunkUnload(chunk);
                }
                chunk->unlock();
            }
        });
    } else if (state != CHUNK_FINALIZED) {
        // chunk is in some of loading states and must execute next loading task
        m_executor->queue([=] () -> void {
            if (chunk->tryLock()) {
                handleChunkStateTask(chunk);
                chunk->unlock();
            }
        });
    }
}

void ChunkSource::handleLoadedChunk(Shared<Chunk> chunk) {
    if (m_state == STATE_UNLOADED) {
        chunk->setState(CHUNK_UNLOADING);
    } else if (m_state == STATE_LAZY) {
        chunk->setState(CHUNK_LAZY);
    } else {
        //
    }
}

void ChunkSource::handleLazyChunk(Shared<Chunk> chunk) {
    if (m_state == STATE_UNLOADED) {
        chunk->setState(CHUNK_UNLOADING);
    } else {
        //
    }
}

void ChunkSource::handleChunkStateTask(Shared<Chunk> chunk) {
    ChunkState state = chunk->getState();
    if (state == CHUNK_PENDING) {
        runChunkBuild(chunk);
    } else if (state == CHUNK_BUILT) {
        runChunkProcessing(chunk);
    } else if (state == CHUNK_PROCESSED) {
        runChunkLoad(chunk);
    }
}

void ChunkSource::runChunkBuild(Shared<Chunk> chunk) {
    chunk->setState(CHUNK_BUILT);
}

void ChunkSource::runChunkProcessing(Shared<Chunk> chunk) {
    chunk->setState(CHUNK_PROCESSED);
}

void ChunkSource::runChunkLoad(Shared<Chunk> chunk) {
    chunk->setState(CHUNK_LOADED);
}

void ChunkSource::runChunkUnload(Shared<Chunk> chunk) {
    ThreadLock lock(m_chunks_mutex);
    m_chunks.erase(chunk->getPosition());
    lock.unlock();

    chunk->deleteAllBuffers();
    chunk->setState(CHUNK_FINALIZED);
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

} // voxel