#include "chunk_source.h"

#include <algorithm>
#include <iostream>
#include <functional>
#include "voxel/common/utils/time.h"
#include "voxel/engine/world/chunk_provider.h"
#include "voxel/engine/world/chunk_storage.h"


namespace voxel {

void ChunkSourceListener::onChunkSourceTick(ChunkSource& chunk_source) {}
void ChunkSourceListener::onChunkUpdated(ChunkSource& chunk_source, Shared<Chunk> chunk) {}


ChunkSource::LoadingRegion::LoadingRegion(ChunkSource* chunk_source, math::Vec3i position, i32 min_size, i32 max_size) :
        m_chunk_source(chunk_source), m_position(position), m_min_size(min_size), m_max_size(max_size) {
}

math::Vec3i ChunkSource::LoadingRegion::getPosition() {
    return m_position;
}

void ChunkSource::LoadingRegion::setPosition(math::Vec3i position) {
    m_position = position;
}

i32 ChunkSource::LoadingRegion::getMinSize() {
    return m_min_size;
}

i32 ChunkSource::LoadingRegion::getMaxSize() {
    return m_max_size;
}

ChunkSource::ChunkSource(
        Shared<ChunkProvider> provider,
        Shared<ChunkStorage> storage,
        Shared<threading::TaskExecutor> executor,
        Settings settings,
        ChunkSourceState initial_state) :
        m_provider(provider), m_storage(storage), m_executor(executor), m_settings(settings), m_state(initial_state),
        m_chunk_task_executor([this] () -> ChunkTask { return m_chunk_task_queue.pop(); }, [this] (ChunkTask task) -> void { runChunkTask(task); }, 4){
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

void ChunkSource::setGpuMemoryRatio(f32 ratio) {
    m_gpu_memory_ratio = ratio;
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
        if (chunk->getState() == CHUNK_LOADED) {
            f32 distance_to_loading_region = getMinDistanceToLoadingRegion(chunk->getPosition(), true);
            if (chunk->getTimeSinceLastFetch() > 2000 && distance_to_loading_region < 0.0) {
                chunk->setState(CHUNK_LAZY);
                fireEventChunkUpdated(chunk);
            }
        } else if (chunk->getState() == CHUNK_LAZY) {
        } else {
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
    // TODO: bring back
    /* i32 updates_count = std::min(m_settings.loaded_chunk_updates, i32(m_updates_queue.getDeque().size()));
    for (i32 i = 0; i < updates_count; i++) {
        auto popped = m_updates_queue.tryPop();
        if (popped.has_value()) {
            Shared<Chunk> chunk = popped->lock();
            if (chunk && updateChunk(chunk)) {
                m_updates_queue.push(popped.value());
            }
        }
    } */
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
            ChunkState state = chunk->getState();
            if (state == CHUNK_LOADED) {
            } else if (state == CHUNK_LAZY) {
                tryLoadLazyChunk(chunk);
            } else {
                if (chunk->tryLock()) {
                    handleChangingChunkStateTask(chunk);
                    chunk->unlock();
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
        return Shared<Chunk>(nullptr);
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

Shared<ChunkSource::LoadingRegion> ChunkSource::addLoadingRegion(math::Vec3i position, i32 min_size, i32 max_size) {
    ThreadLock lock(m_loaded_regions_mutex);
    Shared<ChunkSource::LoadingRegion> loading_region = CreateShared<LoadingRegion>(this, position, min_size, max_size);
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

f32 ChunkSource::getMinDistanceToLoadingRegion(ChunkPosition position, bool relative) {
    ThreadLock lock(m_loaded_regions_mutex);

    math::Vec3i pos(position.x, position.y, position.z);
    f32 distance = -1;
    for (auto& region : m_loaded_regions) {
        f32 d = math::len(region->m_position - pos);
        if (d < region->m_max_size) {
            if (relative) {
                d = std::max(0.0f, d - region->m_min_size) / (region->m_max_size - region->m_min_size);
            }
            if (d < distance || distance < 0) {
                distance = d;
            }
        }
    }

    return distance;
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