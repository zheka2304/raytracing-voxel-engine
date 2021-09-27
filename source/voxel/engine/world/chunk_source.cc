#include "chunk_source.h"

#include <algorithm>
#include <iostream>
#include "voxel/engine/world/chunk_provider.h"
#include "voxel/engine/world/chunk_storage.h"


namespace voxel {

void ChunkSourceListener::onChunkSourceTick(ChunkSource& chunk_source) {}
void ChunkSourceListener::onChunkUpdated(ChunkSource& chunk_source, Shared<Chunk> chunk) {}


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

void ChunkSource::addListener(ChunkSourceListener* listener) {
    if (std::find(m_listeners.begin(), m_listeners.end(), listener) == m_listeners.end()) {
        m_listeners.emplace_back(listener);
    }
}

void ChunkSource::removeListener(ChunkSourceListener* listener) {
    auto it = std::find(m_listeners.begin(), m_listeners.end(), listener);
    if (it != m_listeners.end()) {
        m_listeners.erase(it);
    }
}

void ChunkSource::tryCreateNewChunk(ChunkPosition position) {
    {
        ThreadLock lock(m_chunks_mutex);
        if (m_chunks.find(position) != m_chunks.end()) {
            return;
        }
    }

    Shared<Chunk> chunk = m_provider->createChunk(*this, position);
    if (!chunk) {
        return;
    }
    chunk->setState(CHUNK_PENDING);

    {
        ThreadLock lock(m_chunks_mutex);
        if (m_chunks.find(position) == m_chunks.end()) {
            chunk->fetch();
            m_chunks.emplace(position, chunk);
        }
    }
}

/*
void ChunkSource::handleChunk(Shared<Chunk> chunk) {
    ChunkState state = chunk->getState();
    if (state == CHUNK_LOADED) {
        // chunk is loaded and should be checked to put into lazy state
        handleLoadedChunk(chunk);
    } else if (state == CHUNK_LAZY) {
        // chunk is in lazy state, it can be instantly changed to loaded or start unloading sequence
        handleLazyChunk(chunk);
    } else if (state != CHUNK_FINALIZED) {
        // chunk is in some of loading states and must execute next loading task
        m_executor->queue([=] () -> void {
            if (chunk->tryLock()) {
                handleChangingChunkStateTask(chunk);
                chunk->unlock();
            }
        }, true);
    }
} */

void ChunkSource::handleLoadedChunk(Shared<Chunk> chunk) {
    if (m_state == STATE_UNLOADED) {
        chunk->setState(CHUNK_STORING);
        fireEventChunkUpdated(chunk);
    } else if (m_state == STATE_LAZY) {
        chunk->setState(CHUNK_LAZY);
        fireEventChunkUpdated(chunk);
    } else {
        //
    }
}

void ChunkSource::handleLazyChunk(Shared<Chunk> chunk) {
    if (m_state == STATE_UNLOADED) {
        chunk->setState(CHUNK_STORING);
    } else {
        //
    }
}

void ChunkSource::handleChangingChunkStateTask(Shared<Chunk> chunk) {
    ChunkState state = chunk->getState();
    if (state == CHUNK_PENDING) {
        if (m_storage->tryLoadChunk(*this, chunk)) {
            chunk->setState(CHUNK_PROCESSED);
            runChunkLoad(chunk);
        } else {
            runChunkBuild(chunk);
        }
    } else if (state == CHUNK_BUILT) {
        runChunkProcessing(chunk);
    } else if (state == CHUNK_PROCESSED) {
        runChunkLoad(chunk);
    } else if (state == CHUNK_STORING) {
        if (m_storage->tryStoreChunk(*this, chunk)) {
            chunk->setState(CHUNK_UNLOADING);
        } else {
            chunk->setState(CHUNK_LAZY);
        }
    } else if (state == CHUNK_LAZY) {
        handleLazyChunk(chunk);
    } else if (state == CHUNK_UNLOADING) {
        runChunkUnload(chunk);
    }
}

void ChunkSource::runChunkBuild(Shared<Chunk> chunk) {
    if (m_provider->buildChunk(*this, chunk)) {
        chunk->setState(CHUNK_BUILT);
    }
}

void ChunkSource::runChunkProcessing(Shared<Chunk> chunk) {
    if (m_provider->processChunk(*this, chunk)) {
        chunk->setState(CHUNK_PROCESSED);
    }
}

void ChunkSource::runChunkLoad(Shared<Chunk> chunk) {
    chunk->setState(CHUNK_LOADED);
    fireEventChunkUpdated(chunk);
}

void ChunkSource::runChunkUnload(Shared<Chunk> chunk) {
    ThreadLock lock(m_chunks_mutex);
    m_chunks.erase(chunk->getPosition());
    lock.unlock();

    chunk->deleteAllBuffers();
    chunk->setState(CHUNK_FINALIZED);
}


void ChunkSource::onTick() {
    fireEventTick();
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

bool ChunkSource::fetchChunkAt(ChunkPosition position) {
    Shared<Chunk> chunk = getChunkAt(position);
    if (chunk) {
        chunk->fetch();
        auto chunk_state = chunk->getState();
        if (chunk_state == CHUNK_LOADED) {
            return true;
        } else if (canAddTasks()) {
            m_executor->queue([=] () -> void {
                if (chunk->tryLock()) {
                    handleChangingChunkStateTask(chunk);
                    chunk->unlock();
                }
            });
            return true;
        }
    } else if (canAddTasks()) {
        if (m_provider->canFetchChunk(*this, position)) {
            m_executor->queue([=] () -> void {
                tryCreateNewChunk(position);
            });
        }
        return true;
    }
    return false;
}

bool ChunkSource::canAddTasks() {
    return m_executor->getProcessingThreadCount() > m_executor->getEstimatedRemainingTasks();
}

void ChunkSource::fireEventTick() {
    for (auto listener : m_listeners) {
        listener->onChunkSourceTick(*this);
    }
}

void ChunkSource::fireEventChunkUpdated(const Shared<Chunk>& chunk) {
    for (auto listener : m_listeners) {
        listener->onChunkUpdated(*this, chunk);
    }
}

} // voxel