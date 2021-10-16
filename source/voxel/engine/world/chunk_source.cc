#include "chunk_source.h"

#include <iostream>
#include <functional>
#include "voxel/common/profiler.h"
#include "voxel/engine/world/chunk_provider.h"
#include "voxel/engine/world/chunk_storage.h"


namespace voxel {

void ChunkSourceListener::onChunkSourceTick(ChunkSource& chunk_source) {}
void ChunkSourceListener::onChunkUpdated(ChunkSource& chunk_source, Shared<Chunk> chunk) {}


ChunkSource::LoadingRegion::LoadingRegion(ChunkSource* chunk_source, math::Vec3i position, i32 loading_level) :
        m_chunk_source(chunk_source), m_position(position), m_loading_level(loading_level) {
}

math::Vec3i ChunkSource::LoadingRegion::getPosition() {
    return m_position;
}

void ChunkSource::LoadingRegion::setPosition(math::Vec3i position) {
    m_position = position;
}

i32 ChunkSource::LoadingRegion::getLoadingLevel() {
    return m_loading_level;
}

ChunkSource::ChunkSource(
        Shared<ChunkProvider> provider,
        Shared<ChunkStorage> storage,
        Settings settings,
        ChunkSourceState initial_state) :
        m_provider(provider), m_storage(storage), m_settings(settings), m_state(initial_state),
        m_chunk_task_executor(
                [this] () -> ChunkTask { return m_chunk_task_queue.pop(); },
                [this] (ChunkTask task) -> void { runChunkTask(task); },
                m_settings.worker_threads) {
}

ChunkSource::~ChunkSource() {
    {
        ThreadLock lock(m_loaded_regions_mutex);
        for (auto& region : m_loaded_regions) {
            region->m_chunk_source = nullptr;
        }
    }
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

void ChunkSource::tryLoadLazyChunk(Shared<Chunk> chunk) {
    chunk->setState(CHUNK_LOADED);
    chunk->fetch();
    fireEventChunkUpdated(chunk);
}

void ChunkSource::startUpdatingChunk(Shared<Chunk> chunk) {
    m_updates_queue.push(Weak<Chunk>(chunk));
}

bool ChunkSource::updateChunk(Shared<Chunk> chunk) {
    bool continue_updating = true;
    if (chunk->tryLock()) {
        auto state = chunk->getState();
        if (state == CHUNK_LOADED) {
            if (chunk->getTimeSinceLastFetch() > m_settings.chunk_unload_timeout &&
                getLoadingLevelForPosition(chunk->getPosition()) < LoadingRegion::LEVEL_LOAD) {
                chunk->setState(CHUNK_LAZY);
                fireEventChunkUpdated(chunk);
            }
        } else if (state == CHUNK_LAZY) {
            if (chunk->getTimeSinceLastFetch() > m_settings.chunk_unload_timeout &&
                getLoadingLevelForPosition(chunk->getPosition()) < LoadingRegion::LEVEL_LAZY) {
                chunk->setState(CHUNK_STORING);
                fireEventChunkUpdated(chunk);
            }
        } else if (state == CHUNK_STORING) {
            if (m_storage->tryStoreChunk(*this, chunk)) {
                chunk->setState(CHUNK_UNLOADING);
            } else {
                chunk->setState(CHUNK_LAZY);
            }
        } else if (state == CHUNK_UNLOADING) {
            runChunkUnload(chunk);
            continue_updating = false;
        }
        chunk->unlock();
    }
    return continue_updating;
}

void ChunkSource::handleLazyChunk(Shared<Chunk> chunk) {
    if (m_state == STATE_UNLOADED) {
        chunk->setState(CHUNK_STORING);
    } else {
        if (chunk->getTimeSinceLastFetch() < 1000) {
            tryLoadLazyChunk(chunk);
        }
    }
}

void ChunkSource::handleChunkLoading(Shared<Chunk> chunk) {
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
    } else if (state == CHUNK_LAZY) {
        handleLazyChunk(chunk);
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
    chunk->fetch();
    startUpdatingChunk(chunk);
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
    VOXEL_ENGINE_PROFILE_SCOPE(chunk_source_tick)
    i32 updates_count = std::min(m_settings.loaded_chunk_updates, i32(m_updates_queue.getDeque().size()));
    for (i32 i = 0; i < updates_count; i++) {
        auto popped = m_updates_queue.tryPop();
        if (popped.has_value()) {
            Shared<Chunk> chunk = popped->lock();
            if (chunk && updateChunk(chunk)) {
                m_updates_queue.push(popped.value());
            }
        }
    }
    fireEventTick();
}

void ChunkSource::runChunkTask(ChunkTask task) {
    switch (task.type) {
        case TASK_CREATE: {
            tryCreateNewChunk(task.position);
            break;
        }
        case TASK_LOAD: {
            Shared<Chunk> chunk = getChunkAt(task.position);
            if (chunk) {
                ChunkState state = chunk->getState();
                if (state == CHUNK_LOADED) {
                } else if (state == CHUNK_LAZY) {
                    tryLoadLazyChunk(chunk);
                } else {
                    if (chunk->tryLock()) {
                        handleChunkLoading(chunk);
                        chunk->unlock();
                    }
                }
            }
            break;
        }
    }

}

Shared<Chunk> ChunkSource::getChunkAt(ChunkPosition position) {
    ThreadLock lock(m_chunks_mutex);
    auto it = m_chunks.find(position);
    if (it != m_chunks.end()) {
        return it->second;
    } else {
        return Shared<Chunk>();
    }
}

Shared<Chunk> ChunkSource::fetchChunkAt(ChunkPosition position, i64 priority) {
    Shared<Chunk> chunk = getChunkAt(position);
    if (chunk) {
        chunk->fetch();
        auto chunk_state = chunk->getState();
        if (chunk_state == CHUNK_LOADED) {
        } else if (chunk_state == CHUNK_LAZY) {
            tryLoadLazyChunk(chunk);
        } else {
            m_chunk_task_queue.push({ position, TASK_LOAD, priority });
        }
    } else if (m_provider->canFetchChunk(*this, position)) {
        m_chunk_task_queue.push({ position, TASK_CREATE, priority });
    }
    return chunk;
}

Shared<ChunkSource::LoadingRegion> ChunkSource::addLoadingRegion(math::Vec3i position, i32 loading_level) {
    ThreadLock lock(m_loaded_regions_mutex);
    Shared<ChunkSource::LoadingRegion> loading_region = CreateShared<LoadingRegion>(this, position, loading_level);
    m_loaded_regions.emplace_back(loading_region);
    return loading_region;
}

void ChunkSource::removeLoadingRegion(Shared<LoadingRegion> loading_region) {
    ThreadLock lock(m_loaded_regions_mutex);
    loading_region->m_chunk_source = nullptr;
    auto it = std::find(m_loaded_regions.begin(), m_loaded_regions.end(), loading_region);
    if (it != m_loaded_regions.end()) {
        m_loaded_regions.erase(it);
    }
}

i32 ChunkSource::getLoadingLevelForPosition(ChunkPosition position) {
    math::Vec3i pos0(position.x, position.y, position.z);
    i32 level = 0;

    ThreadLock lock(m_loaded_regions_mutex);
    for (auto& region : m_loaded_regions) {
        math::Vec3i delta = pos0 - region->getPosition();
        level = std::max(level, region->getLoadingLevel() - std::max(abs(delta.x), std::max(abs(delta.y), abs(delta.z))));
    }
    return level;
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