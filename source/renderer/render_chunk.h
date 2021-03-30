
#ifndef VOXEL_ENGINE_RENDER_CHUNK_H
#define VOXEL_ENGINE_RENDER_CHUNK_H

#include <unordered_map>
#include <mutex>
#include <list>
#include <atomic>
#include <memory>
#include <functional>

#include "common/vec.h"

#include "util.h"


class RenderChunk;
class VoxelRenderEngine;


class ChunkPos {
public:
    int x, y, z;
    ChunkPos(int x, int y, int z);
    explicit ChunkPos(Vec3 v);
    ChunkPos();
    bool operator==(ChunkPos const& other) const;
};

namespace std {
    template <>
    struct hash<ChunkPos> {
        std::size_t operator()(const ChunkPos& k) const {
            return k.x ^ (k.y << 1) ^ (k.z << 2);
        }
    };

}


class VoxelChunk {
public:
    static const int DEFAULT_CHUNK_SIZE = 128;

    // 8x8x8 of sub regions of tier 1
    static const int SUB_REGION_SIZE_2_BITS = 2;
    static const int SUB_REGION_SIZE_2 = 1 << SUB_REGION_SIZE_2_BITS;
    static const int SUB_REGION_SIZE_2_MASK = SUB_REGION_SIZE_2 - 1;
    // 4x4x4 of voxels
    static const int SUB_REGION_SIZE_1_BITS = 2;
    static const int SUB_REGION_SIZE_1 = 1 << SUB_REGION_SIZE_1_BITS;
    static const int SUB_REGION_SIZE_1_MASK = SUB_REGION_SIZE_1 - 1;
    // total size of tier 2 sub region in voxels
    static const int SUB_REGION_TOTAL_SIZE = SUB_REGION_SIZE_1 * SUB_REGION_SIZE_2;
    // total elements in buffer for tier 2 sub region
    static const int SUB_REGION_BUFFER_SIZE_1 = 1 + SUB_REGION_SIZE_1 * SUB_REGION_SIZE_1 * SUB_REGION_SIZE_1;
    // total elements in buffer for tier 2 sub region
    static const int SUB_REGION_BUFFER_SIZE_2 = 1 + SUB_REGION_SIZE_2 * SUB_REGION_SIZE_2 * SUB_REGION_SIZE_2 * SUB_REGION_BUFFER_SIZE_1;

    // count of tier 2 regions per chunk axis
    static const int DEFAULT_T2_REGIONS_PER_CHUNK_AXIS = VoxelChunk::DEFAULT_CHUNK_SIZE / VoxelChunk::SUB_REGION_TOTAL_SIZE;
    // render buffer size of default chunk
    static const int DEFAULT_CHUNK_BUFFER_SIZE = VoxelChunk::SUB_REGION_BUFFER_SIZE_2 * DEFAULT_T2_REGIONS_PER_CHUNK_AXIS * DEFAULT_T2_REGIONS_PER_CHUNK_AXIS * DEFAULT_T2_REGIONS_PER_CHUNK_AXIS;


    int sizeX, sizeY, sizeZ;
    int rSizeX, rSizeY, rSizeZ;

    // buffer, that contains pure voxel data
    // default direct access: x + (z + y * sizeZ) * sizeX
    unsigned int* voxelBuffer = nullptr;
    int voxelBufferLen = 0;
    // buffer, containing data for material and normals
    // access via 2 sub regions used in raytracing and physics, but slower for single voxel access
    // region access:
    // - tier 2 offset: t2offset = rx2 + (rz2 + ry2 * rSizeZ) * rSizeX
    // - tier 1 offset: t1offset = t2offset + 1 + rx1 + ((rz1 + (ry1 << SUB_REGION_SIZE_2_BITS)) << SUB_REGION_SIZE_2_BITS)
    // - voxel: voxel = t1offset + 1 + vx + ((vz + (vz << SUB_REGION_SIZE_1_BITS)) << SUB_REGION_SIZE_1_BITS)
    unsigned int* renderBuffer = nullptr;
    int renderBufferLen = 0;

    RenderChunk* renderChunk = nullptr;

    ChunkPos position;
private:
    VoxelChunk(int sizeX, int sizeY, int sizeZ);
public:
    VoxelChunk();
    VoxelChunk(VoxelChunk const& other);

    void setPos(ChunkPos const& pos);

    void rebuildRenderBuffer();
    unsigned int calcNormal(int x, int y, int z);
    bool rebuildTier2Region(int rx2, int ry2, int rz2);
    bool rebuildTier1Region(int rx1, int ry1, int rz1, int offset = -1);
    ~VoxelChunk();

    void attachRenderChunk(RenderChunk* renderChunk);
};


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



class ChunkSource {
public:
    virtual VoxelChunk* getChunkAt(ChunkPos pos) = 0;
};


class DebugChunkSource : public ChunkSource {
public:
    std::unordered_map<ChunkPos, VoxelChunk*> chunkMap;
    std::function<VoxelChunk*(ChunkPos)> chunkProviderFunc;

    explicit DebugChunkSource(std::function<VoxelChunk*(ChunkPos)> providerFunc);
    VoxelChunk* getChunkAt(ChunkPos pos) override;
    ~DebugChunkSource();
};


class Camera {
public:
    Vec3 position = Vec3(0, 0, 0);
    float yaw = 0, pitch = 0;
    float viewport[4] = { 0 };

    void setPosition(Vec3 const& position);
    void setRotation(float yaw, float pitch);
    void setViewport(float x, float y, float w, float h);

    // iterates over all visible chunk positions
    virtual void addAllVisiblePositions(std::unordered_map<ChunkPos, int>& visibilityMap);
    virtual void sendParametersToShader(gl::Shader& shader);

};

class OrthographicCamera : public Camera {
public:
    float near = 0, far = 100;

private:
    void _addAllVisiblePositions(std::unordered_map<ChunkPos, int>& visibilityMap, int level, float viewExpand, bool useEmplace);
public:
    void addAllVisiblePositions(std::unordered_map<ChunkPos, int>& visibilityMap) override;

};



class VoxelRenderEngine {
    static const int MAX_RENDER_CHUNK_INSTANCES = 64;

    std::mutex mutex;

    //
    int totalRenderChunkInstances = 0;
    // pool is used to reuse old render chunks objects instead of destroying them
    std::list<RenderChunk*> renderChunkPool;

    // chunk by position map
    std::unordered_map<ChunkPos, RenderChunk*> renderChunkMap;

    // chunk source for world access
    std::shared_ptr<ChunkSource> chunkSource;

    // camera, that calculates visible chunks
    std::shared_ptr<Camera> camera;

    // all chunks are stored in single buffer because sampler uniform arrays are not always supported
    gl::BufferTexture* chunkBuffer;

    // maximum chunks in buffer
    int chunkBufferSize;
    bool chunkBufferUsage[MAX_RENDER_CHUNK_INSTANCES] = { false };

public:
    VoxelRenderEngine(std::shared_ptr<ChunkSource> chunkSource, std::shared_ptr<Camera> camera);

    RenderChunk* getNewRenderChunk(int maxLevelToReuse = RenderChunk::VISIBILITY_LEVEL_NOT_VISIBLE);

    // refresh visible chunks from camera, update all visibility levels, pool unused chunks
    // for all visible chunks, run queued updates
    void updateVisibleChunks();

    // for all visible chunks, map all textures, pass to uniforms
    // passed shader should contain following uniforms:
    // - CHUNK_OFFSET - ivec3 offset of chunk region to be rendered
    // - CHUNK_COUNT - ivec3 of chunk count on each axis, total count should not be greater then 32
    // - CHUNK_BUFFERS - array of usamplerBuffer with size of 32
    void prepareForRender(gl::Shader& shader);

    GLuint getChunkBufferHandle();
};


#endif //VOXEL_ENGINE_RENDER_CHUNK_H
