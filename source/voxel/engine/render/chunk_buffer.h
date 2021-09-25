#ifndef VOXEL_ENGINE_CHUNK_BUFFER_H
#define VOXEL_ENGINE_CHUNK_BUFFER_H

#include "voxel/common/base.h"
#include "voxel/common/opengl.h"
#include "voxel/common/define.h"
#include "voxel/common/threading.h"
#include "voxel/common/math/vec.h"
#include "voxel/engine/shared/chunk_position.h"
#include "voxel/engine/render/render_context.h"


namespace voxel {

class Chunk;

namespace render {

class ChunkBuffer;

class FetchedChunksList {
    std::mutex m_mutex;

    math::Vec3i m_map_offset;
    math::Vec3i m_map_dimensions;

    std::vector<u32> m_raw_data;
    std::vector<ChunkPosition> m_fetched_chunks;

public:
    FetchedChunksList() = default;
    FetchedChunksList(const FetchedChunksList&) = delete;
    ~FetchedChunksList() = default;

    void runDataUpdate(i32 threshold);
    std::vector<ChunkPosition>& getChunksToFetch();

    friend ChunkBuffer;
};

class ChunkBuffer {
public:
    const u32 PAGE_SIZE = 65536;

    enum ChunkUploadResult {
        RESULT_SUCCESS,
        RESULT_ALLOCATION_FAILED,
        RESULT_OUT_OF_RANGE
    };

private:
    struct ChunkPageSpan {
        Weak<Chunk> chunk;
        i32 first;
        i32 second;
    };

    std::vector<u64> m_chunk_by_page;
    std::unordered_map<u64, ChunkPageSpan> m_pages_by_chunk;
    std::mutex m_page_map_lock;

    i32 m_data_buffer_size;
    i32 m_fetch_buffer_size;
    i32 m_map_buffer_size;

    i32* m_map_buffer;
    math::Vec3i m_map_buffer_offset = math::Vec3i(0);
    math::Vec3i m_map_buffer_dimensions;

    opengl::ShaderStorageBuffer m_data_shader_buffer;
    opengl::ShaderStorageBuffer m_map_shader_buffer;
    opengl::ShaderStorageBuffer m_fetch_shader_buffer;

public:
    ChunkBuffer(i32 page_count, math::Vec3i map_buffer_size);
    ChunkBuffer(const ChunkBuffer&) = delete;
    ChunkBuffer(ChunkBuffer&&) = delete;
    ~ChunkBuffer();

    void rebuildChunkMap(math::Vec3i offset);
    void prepareAndBind(RenderContext& ctx);

    ChunkUploadResult uploadChunk(Shared<Chunk> chunk);
    void removeChunk(Shared<Chunk> chunk);
    void getFetchedChunks(FetchedChunksList& fetched_chunks_list);

private:
    i32 getMapIndex(ChunkPosition position);
    i32 allocatePageSpan(i32 page_count, u64 chunk);
};

} // render
} // voxel

#endif //VOXEL_ENGINE_CHUNK_BUFFER_H
