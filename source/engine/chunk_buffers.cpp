#include <algorithm>
#include <iostream>

#include "common/simple-profiler.h"
#include "common/worker_thread.h"
#include "renderer/util.h"
#include "chunk_buffers.h"
#include "voxel_chunk.h"


GPUBufferPool PooledChunkBuffer::bufferPool(1024 * 1024 * 128);

PooledChunkBuffer::PooledChunkBuffer(VoxelChunk* chunk) : chunk(chunk) {

}

PooledChunkBuffer::~PooledChunkBuffer() {

}

int PooledChunkBuffer::getBufferSize() {
    return chunk->voxelBufferLen * sizeof(unsigned int);
}

GLuint PooledChunkBuffer::getStoredContentUuid() {
    return buffer.getStoredContentUuid();
}

GLuint PooledChunkBuffer::ownHandle() {
    // own buffer
    if (!buffer.tryOwn()) {
        buffer = bufferPool.allocate(getBufferSize());
    }
    GLuint handle = buffer.getGlHandle();
    // sync buffer if required
    if (!buffer.isContentUnchanged()) {
        _sync(handle);
    } else if (lastUpdateId != lastSyncId) {
        _sync(handle);
    }
    // return handle
    return handle;
}

void PooledChunkBuffer::releaseHandle() {
    buffer.release();
}

void PooledChunkBuffer::_sync(GLuint handle, int maxLockDelayMilliseconds) {
    std::timed_mutex &chunkMutex = chunk->getContentMutex();
    if (chunkMutex.try_lock_for(std::chrono::milliseconds(maxLockDelayMilliseconds))) {
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, handle);
        glBufferData(GL_SHADER_STORAGE_BUFFER, getBufferSize(), chunk->voxelBuffer, GL_DYNAMIC_COPY);
        chunkMutex.unlock();
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
        buffer.setContentUpdated();
        lastSyncId = lastUpdateId;
    } else {
        // std::cout << "failed to update chunk pooled buffer, lock failed\n";
    }
}

void PooledChunkBuffer::update() {
    lastUpdateId++;
}


GPUBufferPool BakedChunkBuffer::bufferPool(1024 * 1024 * 256);
std::mutex BakedChunkBuffer::cacheByContentIdMapMutex;
std::unordered_map<GLuint, GPUBufferPool::Buffer> BakedChunkBuffer::cacheByContentIdMap;

void BakedChunkBuffer::_dispatchCompute(GLuint chunkBufferHandle, Vec3i offset, Vec3i size) {
    // compile shader once and use it
    static gl::ComputeShader bakeChunkShader("bake_chunk.compute");
    bakeChunkShader.use();

    // 1 KB buffer for uniforms
    static GPUBufferPool uniformBufferPool(16);

    // init uniforms
    int uniforms[4] = { sharedBufferOffset, offset.x, offset.y, offset.z };

    // allocate uniform buffer
    GPUBufferPool::Buffer uniformBuffer = uniformBufferPool.allocate(sizeof(int) * 4);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, uniformBuffer.getGlHandle());
    glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(int) * 4, uniforms, GL_STATIC_DRAW);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

    // pass buffers to compute shader and dispatch
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, chunkBufferHandle);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, uniformBuffer.getGlHandle());
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, sharedBufferHandle);

    // std::cout << "dispatch compute << " << offset.x << ", " << offset.y << ", " << offset.z << " (" << size.x << ", " << size.y << ", " << size.z << ")\n";
    glDispatchCompute(size.x, size.y, size.z);

    // release uniform buffer
    uniformBuffer.release();

}

void BakedChunkBuffer::bake(PooledChunkBuffer& chunkBuffer, GLuint sharedBufferHandle, GLuint sharedBufferOffset, std::vector<Vec3i> const& regions) {
    this->sharedBufferHandle = sharedBufferHandle;
    this->sharedBufferOffset = sharedBufferOffset;
    this->sharedBufferSpanSize = chunkBuffer.getBufferSize();

    GLuint chunkBufferHandle = chunkBuffer.ownHandle();
    GLuint chunkStoredContentUuid = chunkBuffer.getStoredContentUuid();

    ownedContentUuid = 0;

    // TODO: caching is temporary disabled, because all loaded chunks are fitting in render buffer
    /*
    cacheByContentIdMapMutex.lock();
    auto it = cacheByContentIdMap.find(chunkStoredContentUuid);
    if (it != cacheByContentIdMap.end()) {
        GPUBufferPool::Buffer cacheBuffer = it->second;
        cacheByContentIdMapMutex.unlock();

        if (cacheBuffer.tryOwn()) {
            if (cacheBuffer.isContentUnchanged()) {
                glBindBuffer(GL_COPY_READ_BUFFER, cacheBuffer.getGlHandle());
                glBindBuffer(GL_COPY_WRITE_BUFFER, sharedBufferHandle);
                glCopyBufferSubData(GL_COPY_READ_BUFFER, GL_COPY_WRITE_BUFFER, 0, sharedBufferOffset, sharedBufferSpanSize);
                glBindBuffer(GL_COPY_READ_BUFFER, 0);
                glBindBuffer(GL_COPY_WRITE_BUFFER, 0);
                std::cout << "restored from cache\n";
                cacheBuffer.release();
                chunkBuffer.releaseHandle();
                return;
            } else {
                std::cout << "cache invalid\n";
            }
            cacheBuffer.release();
        }
    } else {
        std::cout << "cache not found\n";
        cacheByContentIdMapMutex.unlock();
    } */

    if (regions.empty()) {
        _dispatchCompute(chunkBufferHandle, Vec3i(0, 0, 0), Vec3i(8, 8, 8));
    } else {
        for (auto& offset : regions) {
            _dispatchCompute(chunkBufferHandle, offset, Vec3i(1, 1, 1));
        }
    }

    // update owned content uuid and release handle
    ownedContentUuid = chunkBuffer.getStoredContentUuid();
    chunkBuffer.releaseHandle();
}

void BakedChunkBuffer::release() {
    if (sharedBufferHandle != 0) {
        // TODO: caching is temporary disabled, because all loaded chunks are fitting in render buffer
        /* if (ownedContentUuid != 0) {
            // own cache buffer
            GPUBufferPool::Buffer cacheBuffer;
            cacheByContentIdMapMutex.lock();
            auto it = cacheByContentIdMap.find(ownedContentUuid);
            if (it != cacheByContentIdMap.end()) {
                cacheBuffer = it->second;
                if (!cacheBuffer.tryOwn()) {
                    it->second = cacheBuffer = bufferPool.allocate(sharedBufferSpanSize);
                }
            } else {
                cacheByContentIdMap.emplace(ownedContentUuid, cacheBuffer = bufferPool.allocate(sharedBufferSpanSize));
            }
            cacheByContentIdMapMutex.unlock();

            // copy new data to cache buffer
            glBindBuffer(GL_COPY_READ_BUFFER, sharedBufferHandle);
            glBindBuffer(GL_COPY_WRITE_BUFFER, cacheBuffer.getGlHandle());
            glCopyBufferSubData(GL_COPY_READ_BUFFER, GL_COPY_WRITE_BUFFER, sharedBufferOffset, 0, sharedBufferSpanSize);
            glBindBuffer(GL_COPY_READ_BUFFER, 0);
            glBindBuffer(GL_COPY_WRITE_BUFFER, 0);
            cacheBuffer.setContentUpdated();
            std::cout << "cache created\n";

            // release cache buffer
            cacheBuffer.release();
        } */

        // mark released
        sharedBufferHandle = 0;
        sharedBufferOffset = 0;
        sharedBufferSpanSize = 0;
    }
}

void BakedChunkBuffer::releaseAndDestroyCache() {
    sharedBufferHandle = 0;
    sharedBufferOffset = 0;
    sharedBufferSpanSize = 0;
}

BakedChunkBuffer::~BakedChunkBuffer() {

}