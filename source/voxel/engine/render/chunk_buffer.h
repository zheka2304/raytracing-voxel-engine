#ifndef VOXEL_ENGINE_CHUNK_BUFFER_H
#define VOXEL_ENGINE_CHUNK_BUFFER_H

#include <set>

#include "voxel/common/base.h"
#include "voxel/common/opengl.h"
#include "voxel/common/define.h"
#include "voxel/common/threading.h"
#include "voxel/common/math/vec.h"
#include "voxel/common/utils/heap.h"
#include "voxel/engine/shared/chunk_position.h"
#include "voxel/engine/render/render_context.h"
#include "voxel/engine/world/chunk.h"


namespace voxel {

class ChunkSource;

namespace render {

class ChunkBuffer;


class FetchedChunksList {
public:
    struct ChunkPosAndWeight {
        ChunkPosition pos;
        u32 weight;

        inline bool operator<(const ChunkPosAndWeight& other) const {
            // invert, so bigger ones go first
            return weight > other.weight;
        }
    };

private:

    std::mutex m_mutex;

    math::Vec3i m_map_offset;
    math::Vec3i m_map_dimensions;

    std::vector<u32> m_raw_data;
    std::vector<ChunkPosAndWeight> m_fetched_chunks;
    std::vector<ChunkPosAndWeight>::iterator m_fetched_chunks_iter = m_fetched_chunks.begin();

public:
    FetchedChunksList() = default;
    FetchedChunksList(const FetchedChunksList&) = delete;
    ~FetchedChunksList() = default;

    void runDataUpdate();
    std::vector<ChunkPosAndWeight>& getChunksToFetch();

    void beginIteration();
    void endIteration();
    bool hasNext();
    ChunkPosAndWeight next();

    friend ChunkBuffer;
};


class ChunkBuffer {
public:
    enum ChunkUploadResult {
        RESULT_SUCCESS,
        RESULT_ALLOCATION_FAILED,
        RESULT_OUT_OF_RANGE
    };

private:
    i32 m_page_size;

    std::vector<ChunkRef> m_chunk_by_page;
    i32 m_allocated_page_count = 0;

    struct UploadedChunk {
        ChunkRef chunk_ref;

        bool allocated;
        i32 first, second;

        i32 index;
        i64 priority;

        inline UploadedChunk(const ChunkRef& ref, i32 first, i32 second, i64 priority) :
            chunk_ref(ref), first(first), second(second), priority(priority), allocated(true) {
        }

        inline UploadedChunk(const ChunkRef& ref, i64 priority) :
            chunk_ref(ref), priority(priority), allocated(false) {
        }

        struct Index {
            inline i32& operator()(UploadedChunk& x) {
                return x.index;
            }
        };

        struct MaxValueFirst {
            inline i64 operator()(UploadedChunk& x) {
                return x.priority;
            }
        };

        struct MinValueFirst {
            inline i64 operator()(UploadedChunk& x) {
                return -x.priority;
            }
        };
    };

    std::unordered_map<ChunkRef, UploadedChunk> m_uploaded_chunks;
    Heap<UploadedChunk, UploadedChunk::Index, UploadedChunk::MinValueFirst> m_allocated_chunk_heap;
    Heap<UploadedChunk, UploadedChunk::Index, UploadedChunk::MaxValueFirst> m_pending_chunk_heap;

    struct UploadRequest {
        ChunkRef chunk_ref;
        i32 offset_page;
        i32 allocated_page_count;
    };
    threading::BlockingQueue<UploadRequest> m_upload_request_queue;

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
    ChunkBuffer(i32 page_count, i32 page_size, math::Vec3i map_buffer_size);
    ChunkBuffer(const ChunkBuffer&) = delete;
    ChunkBuffer(ChunkBuffer&&) = delete;
    ~ChunkBuffer();


    // -- RENDER THREAD --

    // updates fetch & map buffers and binds all buffers
    void prepareAndBind(RenderContext& ctx);

    // runs all queued chunk uploads, if chunk is locked, skips it and queues for next frame
    void runUploadQueue(ChunkSource& chunk_source);

    // gets raw data from fetch buffer into given fetch list
    void getFetchedChunks(FetchedChunksList& fetched_chunks_list);


    // -- TICKING THREAD --

    // uploads or updates uploaded chunk:
    // - if not uploaded, uploads, if allocation fails, places it into pending heap
    // - if uploaded, updates priority
    void uploadChunk(Chunk& chunk, i64 priority);

    // for already uploaded chunk:
    // - update chunk priority
    // - if chunk is not allocated, tries to allocate it
    void updateChunkPriority(Chunk& chunk, i64 priority);

    // completely removes chunk from all heaps and lookup map
    void removeChunk(ChunkRef chunk_ref);

    // iterates over all chunks and updates it for new offset
    void rebuildChunkMap(math::Vec3i offset);


    // -- ANY THREAD --

    f32 getMemoryRatio();

private:
    i32 getMapIndex(ChunkPosition position);
    i32 tryAllocatePageSpan(i32 page_count, ChunkRef chunk_ref);
    i32 allocatePageSpan(i32 page_count, ChunkRef chunk_ref, i64 priority);
};

} // render
} // voxel

#endif //VOXEL_ENGINE_CHUNK_BUFFER_H
