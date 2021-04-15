#include <iostream>

#include "common/vec.h"
#include "engine/voxel_chunk.h"
#include "render_chunk.h"
#include "render_engine.h"


RenderChunk::RenderChunk(VoxelRenderEngine* renderEngine) : renderEngine(renderEngine) {

}

void RenderChunk::setPos(int x, int y, int z) {
    this->x = x;
    this->y = y;
    this->z = z;
}

void RenderChunk::setChunkBufferOffset(size_t offset) {
    chunkBufferOffset = offset;
}

void RenderChunk::_attach(VoxelChunk* newChunk) {
    chunkMutex.lock();
    if (chunk != newChunk) {
        if (chunk != nullptr) {
            // TODO: when caching will be implemented, this should be executed on gpu worker thread (also check if chunk was destroyed)
            chunk->bakedBuffer.release();

            chunk->renderChunkMutex.lock();
            chunk->renderChunk = nullptr;
            chunk->renderChunkMutex.unlock();
        }

        chunk = newChunk;
        if (chunk != nullptr) {
            chunk->renderChunkMutex.lock();
            chunk->renderChunk = this;
            chunk->renderChunkMutex.unlock();
        }

        if (chunk != nullptr) {
            fullUpdateQueued = true;
        } else {
            fullUpdateQueued = false;
        }

        // at the end we clear per-region update queue
        regionUpdateQueueMutex.lock();
        regionUpdateQueue.clear();
        regionUpdateQueueMutex.unlock();
    }
    chunkMutex.unlock();
}

void RenderChunk::queueRegionUpdate(int x, int y, int z) {
    if (fullUpdateQueued) {
        return;
    }
    std::unique_lock<std::mutex> queueLock(regionUpdateQueueMutex);
    regionUpdateQueue.emplace(x | ((y | (z << 6)) << 6), Vec3i(x, y, z));
}

void RenderChunk::queueFullUpdate() {
    fullUpdateQueued = true;
}

bool RenderChunk::hasAnyQueuedUpdates() {
    if (fullUpdateQueued) {
        return true;
    }
    std::unique_lock<std::mutex> queueLock(regionUpdateQueueMutex);
    return !regionUpdateQueue.empty();
}

int RenderChunk::runAllUpdates() {
    // lock before check
    regionUpdateQueueMutex.lock();
    if (fullUpdateQueued || regionUpdateQueue.size() > MAX_PER_REGION_UPDATES) {
        // clear all other queued updates
        fullUpdateQueued = false;
        regionUpdateQueue.clear();
        regionUpdateQueueMutex.unlock();

        // run baking
        std::unique_lock<std::mutex> chunkLock(chunkMutex);
        if (chunk != nullptr) {
            chunk->bakedBuffer.bake(chunk->pooledBuffer, renderEngine->getChunkBufferHandle(), chunkBufferOffset,std::vector<Vec3i>({}));
            return MAX_PER_REGION_UPDATES;
        } else {
            return 0;
        }
    } else if (!regionUpdateQueue.empty()) {
        // move queue to local variable
        std::unordered_map<int, Vec3i> queue = std::move(regionUpdateQueue);
        regionUpdateQueueMutex.unlock();

        // create vector to hold changed region positions
        std::vector<Vec3i> regions;
        for (auto& hashAndPos : queue) {
            regions.emplace_back(hashAndPos.second);
        }

        // lock in chunk and bake regions
        std::unique_lock<std::mutex> chunkLock(chunkMutex);
        if (chunk != nullptr) {
            chunk->bakedBuffer.bake(chunk->pooledBuffer, renderEngine->getChunkBufferHandle(), chunkBufferOffset, regions);
            return regions.size();
        } else {
            return 0;
        }
    } else {
        regionUpdateQueueMutex.unlock();
    }
    return 0;
}


RenderChunk::~RenderChunk() {
    if (this->chunk != nullptr) {
        this->chunk->attachRenderChunk(nullptr);
    }
}


