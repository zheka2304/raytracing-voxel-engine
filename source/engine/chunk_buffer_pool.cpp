#include <algorithm>
#include <iostream>

#include "common/simple-profiler.h"
#include "common/worker_thread.h"
#include "renderer/util.h"
#include "chunk_buffer_pool.h"
#include "voxel_chunk.h"


GPUBufferPool PooledChunkBuffer::bufferPool;

PooledChunkBuffer::PooledChunkBuffer(VoxelChunk* chunk) : chunk(chunk) {

}

PooledChunkBuffer::~PooledChunkBuffer() {

}

int PooledChunkBuffer::getBufferSize() {
    return chunk->voxelBufferLen * sizeof(unsigned int);
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

void PooledChunkBuffer::_sync(GLuint handle) {
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, handle);
    glBufferData(GL_SHADER_STORAGE_BUFFER, getBufferSize(), chunk->voxelBuffer, GL_DYNAMIC_DRAW);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
    buffer.setContentUpdated();
    lastSyncId = lastUpdateId;
}

void PooledChunkBuffer::update() {
    lastUpdateId++;
}



void BakedChunkBuffer::bake(PooledChunkBuffer& chunkBuffer, GLuint sharedBufferHandle, GLuint sharedBufferOffset) {
    this->sharedBufferHandle = sharedBufferHandle;
    this->sharedBufferOffset = sharedBufferOffset;

    GLuint chunkBufferHandle = chunkBuffer.ownHandle();
    /*
    // update buffer size
    this->bufferSize = chunkBuffer.getBufferSize();

    // if cache is valid
    if (cacheBuffer != nullptr && chunkBuffer.getSyncUuid() == cacheSyncUuid) {
        // use cache
        bakeSyncUuid = cacheSyncUuid;
        // copy cache to buffer
        glBindBuffer(GL_TEXTURE_BUFFER, sharedBufferHandle);
        glBufferSubData(GL_TEXTURE_BUFFER, sharedBufferOffset, cacheBufferSize, cacheBuffer);
        glBindBuffer(GL_TEXTURE_BUFFER, 0);
    } else { */
        // use new
        // bakeSyncUuid = chunkBuffer.getSyncUuid();
        // run baking for chunk

        static WorkerThread bakingThread;
        static gl::ComputeShader bakeChunkShader("bake_chunk.compute");
        bakeChunkShader.use();

        int offset = sharedBufferOffset;
        GLuint offsetBuffer;
        glGenBuffers(1, &offsetBuffer);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, offsetBuffer);
        glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(int), &offset, GL_STATIC_DRAW);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, chunkBufferHandle);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, offsetBuffer);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, sharedBufferHandle);

        glDispatchCompute(8, 8, 8);

        chunkBuffer.releaseHandle();
}

void BakedChunkBuffer::release() {
    if (sharedBufferHandle != 0) {
        // required to update cache
        if (cacheBuffer == nullptr || cacheSyncUuid != bakeSyncUuid) {
            // allocate cache if required
            /*
            if (cacheBufferSize != bufferSize) {
                delete(cacheBuffer);
                cacheBuffer = nullptr;
            }
            if (cacheBuffer == nullptr) {
                cacheBufferSize = bufferSize;
                cacheBuffer = new unsigned int[cacheBufferSize / sizeof(unsigned int)];
            }

            // get cache from buffer
            cacheSyncUuid = bakeSyncUuid;
            glBindBuffer(GL_TEXTURE_BUFFER, sharedBufferHandle);
            glGetBufferSubData(GL_TEXTURE_BUFFER, sharedBufferOffset, cacheBufferSize, cacheBuffer);
            glBindBuffer(GL_TEXTURE_BUFFER, 0); */
        }
        // mark released
        sharedBufferHandle = 0;
        sharedBufferOffset = 0;
    }
}

void BakedChunkBuffer::releaseAndDestroyCache() {
    sharedBufferHandle = 0;
    sharedBufferOffset = 0;
    delete(cacheBuffer);
    cacheBuffer = nullptr;
}

bool BakedChunkBuffer::owns() {
    return sharedBufferHandle != 0;
}

BakedChunkBuffer::~BakedChunkBuffer() {
    if (cacheBuffer != nullptr) {
        delete (cacheBuffer);
    }
}