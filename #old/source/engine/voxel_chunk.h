
#ifndef VOXEL_ENGINE_VOXEL_CHUNK_H
#define VOXEL_ENGINE_VOXEL_CHUNK_H

#include "common/types.h"
#include "common/chunk_pos.h"
#include "chunk_buffers.h"


class RenderChunk;
class ChunkSource;

class VoxelChunk {
public:
    enum ChunkState : int {
        STATE_INITIALIZED = 0,
        STATE_POPULATED = 1,
        STATE_BAKED = 2
    };

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
    static const int DEFAULT_T2_REGIONS_PER_CHUNK_AXIS = ChunkPos::CHUNK_SIZE / VoxelChunk::SUB_REGION_TOTAL_SIZE;
    // render buffer size of default chunk
    static const int DEFAULT_CHUNK_BUFFER_SIZE = VoxelChunk::SUB_REGION_BUFFER_SIZE_2 * DEFAULT_T2_REGIONS_PER_CHUNK_AXIS * DEFAULT_T2_REGIONS_PER_CHUNK_AXIS * DEFAULT_T2_REGIONS_PER_CHUNK_AXIS;


    int sizeX, sizeY, sizeZ;
    int rSizeX, rSizeY, rSizeZ;

    // buffer, that contains pure voxel data
    // default direct access: x + (z + y * sizeZ) * sizeX
    std::timed_mutex contentMutex;
    voxel_data_t* voxelBuffer = nullptr;
    int voxelBufferLen = 0;

    // gpu cache for chunk data
    PooledChunkBuffer pooledBuffer;

    // buffer, containing data for material and normals
    // access via 2 sub regions used in raytracing and physics, but slower for single voxel access
    // region access:
    // - tier 2 offset: t2offset = rx2 + (rz2 + ry2 * rSizeZ) * rSizeX
    // - tier 1 offset: t1offset = t2offset + 1 + rx1 + ((rz1 + (ry1 << SUB_REGION_SIZE_2_BITS)) << SUB_REGION_SIZE_2_BITS)
    // - voxel: voxel = t1offset + 1 + vx + ((vz + (vz << SUB_REGION_SIZE_1_BITS)) << SUB_REGION_SIZE_1_BITS)
    BakedChunkBuffer bakedBuffer;

    // current attached RenderChunk
    std::mutex renderChunkMutex;
    RenderChunk* renderChunk = nullptr;

    // ChunkSource related fields

    // absolute position
    ChunkPos position;
    // loading state
    ChunkState state = STATE_INITIALIZED;
    // owning chunk source
    ChunkSource* chunkSource = nullptr;
private:
    VoxelChunk(int sizeX, int sizeY, int sizeZ);
public:
    VoxelChunk();
    VoxelChunk(VoxelChunk const& other);
    bool copyFrom(VoxelChunk const& other);

    void setChunkSource(ChunkSource* source);
    void setPos(ChunkPos const& pos);
    void setState(ChunkState newState);
    bool isAvailableForRender();

    std::timed_mutex& getContentMutex();
    void queueFullUpdate();
    void queueRegionUpdate(int x, int y, int z);

    ~VoxelChunk();
    void attachRenderChunk(RenderChunk* renderChunk);

private:
    inline voxel_data_t getVoxelAt(int x, int y, int z) {
        if ((unsigned(x) >> 7) == 0 && (unsigned(y) >> 7) == 0 && (unsigned(z) >> 7) == 0) {
            return voxelBuffer[x | ((z | (y << 7)) << 7)];
        } else {
            return 0;
        }
    }
    unsigned int calcNormal(int x, int y, int z);
};

#endif //VOXEL_ENGINE_VOXEL_CHUNK_H
