#ifndef VOXEL_ENGINE_CHUNK_H
#define VOXEL_ENGINE_CHUNK_H

#include "voxel/common/types.h"
#include "voxel/engine/shared/voxel.h"


namespace voxel {
namespace world {

struct VoxelPosition {
    u8 scale;
    u32 x, y, z;
};

struct ChunkPosition {
    i32 x, y, z;
};

class Chunk {
private:
    static const i8 HEADER_SIZE = 3;
    static const i8 VOXEL_SIZE = 2;
    static const i8 TREE_NODE_SIZE = 10;

private:
    ChunkPosition m_position;

    u32* m_buffer = nullptr;
    i32 m_buffer_tree_offset = 0;
    i32 m_buffer_voxels_offset = 0;
    i32 m_buffer_voxel_span = 0;
    i32 m_buffer_size = 0;

public:
    Chunk(ChunkPosition position);
    Chunk(const Chunk&) = delete;
    Chunk(const Chunk&&) = delete;
    ~Chunk();

    const ChunkPosition& getPosition() const;
    const u32* getBuffer() const;
    const i32 getBufferSize() const;

private:
    u32 _getAllocatedNodeSpanSize();
    u32 _getAllocatedVoxelSpanSize();
    u32 _allocateNewNode(u32 color, u32 material);
    u32 _allocateNewVoxel(u32 color, u32 material);

public:
    void setVoxel(VoxelPosition position, Voxel voxel);
    void preallocate(i32 tree_nodes, i32 voxels);
    void preallocate(i32 voxels);
};

} // world
} // voxel

#endif //VOXEL_ENGINE_CHUNK_H
