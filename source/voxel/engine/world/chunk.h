#ifndef VOXEL_ENGINE_CHUNK_H
#define VOXEL_ENGINE_CHUNK_H

#include <mutex>
#include <atomic>

#include "voxel/common/base.h"
#include "voxel/engine/shared/voxel.h"
#include "voxel/engine/shared/chunk_position.h"
#include "voxel/engine/shared/voxel_position.h"


namespace voxel {

enum ChunkState {
    // Chunk is just initialized and must be generated & processed
    CHUNK_PENDING,

    // Chunk base voxels are generated or loaded
    CHUNK_BUILT,

    // Chunk has passed its post-processing stage
    CHUNK_PROCESSED,

    // Chunk is loaded and ready to tick and render
    CHUNK_LOADED,

    // Chunk is in lazy state, it is not ticking or rendering, yet it is in memory and can quickly change to LOADED state
    CHUNK_LAZY,

    //
    CHUNK_STORING,

    // Chunk is waiting for unloading task to complete, such chunks cannot change state and can only be removed
    CHUNK_UNLOADING,

    // Chunk is unloaded, its buffer is deleted, it is detached from chunk source and waiting to be destroyed
    CHUNK_FINALIZED
};

class Chunk {
private:
    static const i8 HEADER_SIZE = 3;
    static const i8 VOXEL_SIZE = 2;
    static const i8 TREE_NODE_SIZE = 10;

private:
    ChunkPosition m_position;
    std::atomic<ChunkState> m_state = CHUNK_PENDING;
    std::mutex m_lock;
    std::atomic<u64> m_last_fetched;

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

    ChunkState getState() const;
    void setState(ChunkState state);
    u64 getLastFetched() const;
    u64 getTimeSinceLastFetch() const;
    void fetch();

    std::mutex& getLock();
    bool tryLock();
    void unlock();

private:
    u32 _getAllocatedNodeSpanSize();
    u32 _getAllocatedVoxelSpanSize();
    u32 _allocateNewNode(u32 color, u32 material);
    u32 _allocateNewVoxel(u32 color, u32 material);

public:

    void setVoxel(VoxelPosition position, Voxel voxel);
    void preallocate(i32 tree_nodes, i32 voxels);
    void preallocate(i32 voxels);
    void deleteAllBuffers();
};

struct ChunkRef {
    inline ChunkRef() : m_pos(ChunkPosition::invalid()) {};
    inline ChunkRef(const Chunk& chunk) : m_pos(chunk.getPosition()) {};
    inline ChunkRef(const ChunkPosition& pos) : m_pos(pos) {};

    inline const ChunkPosition& position() const { return m_pos; }
    inline bool operator==(const ChunkRef& other) const { return m_pos == other.m_pos; }
private:
    ChunkPosition m_pos;
};

} // voxel

namespace std {
std::string to_string(voxel::ChunkState state);
} // std

template<>
struct std::hash<voxel::ChunkRef> {
    std::size_t operator()(const voxel::ChunkRef& r) const {
        return std::hash<voxel::ChunkPosition>()(r.position());
    }
};

#endif //VOXEL_ENGINE_CHUNK_H
