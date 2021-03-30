
#ifndef VOXEL_ENGINE_RENDER_CHUNK_H
#define VOXEL_ENGINE_RENDER_CHUNK_H

#include <mutex>
#include <atomic>
#include <unordered_map>

#include "common/vec.h"
#include "util.h"


class VoxelRenderEngine;
class VoxelChunk;

class RenderChunk {
public:
    static const int FULL_CHUNK_UPDATE = 64;

    static const int VISIBILITY_LEVEL_NOT_VISIBLE = 0;
    static const int VISIBILITY_LEVEL_NEAR_VIEW = 1;
    static const int VISIBILITY_LEVEL_VISIBLE = 2;

    static const int MAX_REGIONS_PER_CHUNK_SIZE_BITS = 5;
    static const int MAX_REGIONS_PER_CHUNK_SIZE = 1 << MAX_REGIONS_PER_CHUNK_SIZE_BITS;
    static const int REGION_UPDATE_BUFFER_SIZE = MAX_REGIONS_PER_CHUNK_SIZE * MAX_REGIONS_PER_CHUNK_SIZE * MAX_REGIONS_PER_CHUNK_SIZE;
private:
    // mutex used when working with update queue and reattaching
    std::mutex chunkMutex;

    // reload queue contains region indices to update
    std::unordered_map<int, int> regionUpdateQueue;
    // separately queueing full update is an option

public:
    std::atomic<bool> fullUpdateQueued = false;
    VoxelRenderEngine* renderEngine;
    int chunkBufferOffset = -1;

    VoxelChunk* chunk = nullptr;
    int x = 0, y = 0, z = 0;

    // used by render engine
    int visibilityLevel = VISIBILITY_LEVEL_NOT_VISIBLE;

    explicit RenderChunk(VoxelRenderEngine* renderEngine);
    void setPos(int x, int y, int z);
    void setChunkBufferOffset(int offset);
    void _attach(VoxelChunk* chunk);
    int runAllUpdates(int maxRegionUpdates = -1);
    ~RenderChunk();
};


#endif //VOXEL_ENGINE_RENDER_CHUNK_H
