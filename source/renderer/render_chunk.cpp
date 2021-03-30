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

void RenderChunk::setChunkBufferOffset(int offset) {
    chunkBufferOffset = offset;
}

void RenderChunk::_attach(VoxelChunk* newChunk) {
    chunkMutex.lock();
    if (chunk != newChunk) {
        chunk = newChunk;
        if (chunk != nullptr) {
            fullUpdateQueued = true;
        } else {
            fullUpdateQueued = false;
        }
        // at the end we clear per-region update queue
        regionUpdateQueue.clear();
    }
    chunkMutex.unlock();
}

int RenderChunk::runAllUpdates(int maxRegionUpdates) {
    if (fullUpdateQueued) {
        // clear all other queued updates
        chunkMutex.lock();
        regionUpdateQueue.clear();
        chunkMutex.unlock();
        fullUpdateQueued = false;

        // TODO: chunk data must be somehow locked during this update
        glBindBuffer(GL_TEXTURE_BUFFER, renderEngine->getChunkBufferHandle());
        glBufferSubData(GL_TEXTURE_BUFFER,
                        chunkBufferOffset * sizeof(unsigned int),
                        chunk->renderBufferLen * sizeof(unsigned int),
                        chunk->renderBuffer);
        glBindBuffer(GL_TEXTURE_BUFFER, 0);

        return FULL_CHUNK_UPDATE;
    } else {
        int update_count = 0;

        glBindBuffer(GL_TEXTURE_BUFFER, renderEngine->getChunkBufferHandle());
        while(maxRegionUpdates < 0 || update_count < maxRegionUpdates) {
            // lock and pop first element of the map
            chunkMutex.lock();
            auto it = regionUpdateQueue.begin();
            if (it == regionUpdateQueue.end()) {
                chunkMutex.unlock();
                break;
            }
            std::pair<int, int> reg_pair = *it;
            regionUpdateQueue.erase(it);
            chunkMutex.unlock();

            // as the next region is acquired, update it
            // TODO: chunk data must be somehow locked during this update
            glBufferSubData(GL_TEXTURE_BUFFER,
                            (chunkBufferOffset + reg_pair.first) * sizeof(unsigned int),
                            reg_pair.second * sizeof(unsigned int),
                            chunk->renderBuffer + reg_pair.first);
            update_count++;
        }
        glBindBuffer(GL_TEXTURE_BUFFER, 0);

        return update_count;
    }
}


RenderChunk::~RenderChunk() {
    if (this->chunk != nullptr) {
        this->chunk->attachRenderChunk(nullptr);
    }
}


