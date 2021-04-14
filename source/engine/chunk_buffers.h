
#ifndef VOXEL_ENGINE_CHUNK_BUFFERS_H
#define VOXEL_ENGINE_CHUNK_BUFFERS_H

#include <glad/glad.h>
#include <atomic>
#include <mutex>
#include <list>
#include <unordered_map>
#include <vector>

#include "common/vec.h"
#include "gpu_cache.h"

class VoxelChunk;


// Implements GPU cache for non-baked chunk data, allows maximum allocation of certain size
// this cache is passed to baking compute shader
class PooledChunkBuffer {
private:
    VoxelChunk* chunk;
    GPUBufferPool::Buffer buffer;

    int lastSyncId = 1;
    int lastUpdateId = 0;

    static GPUBufferPool bufferPool;
public:
    explicit PooledChunkBuffer(VoxelChunk* chunk);
    int getBufferSize();
    GLuint getStoredContentUuid();
    GLuint ownHandle();
    void releaseHandle();
    void update();
    ~PooledChunkBuffer();

private:
    void _sync(GLuint handle);
};


// Holds baked chunk data, either in shared GPU buffer or on CPU memory,
// if GPU buffer span was replaced by another chunk
class BakedChunkBuffer {
private:
    GLuint sharedBufferHandle = 0;
    int sharedBufferOffset, sharedBufferSpanSize;

    // currently owned chunk buffer stored content uuid
    GLuint ownedContentUuid = 0;

    // lookup map by stored chunk content uuid
    static std::mutex cacheByContentIdMapMutex;
    static std::unordered_map<GLuint, GPUBufferPool::Buffer> cacheByContentIdMap;

    // cache buffer pool
    static GPUBufferPool bufferPool;

public:
    // own shared buffer span, use cache or run baking compute shader, release cache if out of cache memory
    void bake(PooledChunkBuffer& chunkBuffer, GLuint sharedBufferHandle, GLuint sharedBufferOffset, std::vector<Vec3i> const& regions);
    // releases shared buffer, creates cache, if able to do it
    void release();
    void releaseAndDestroyCache();
    ~BakedChunkBuffer();

private:
    void _dispatchCompute(GLuint chunkBufferHandle, Vec3i offset, Vec3i size);
};


#endif //VOXEL_ENGINE_CHUNK_BUFFERS_H
