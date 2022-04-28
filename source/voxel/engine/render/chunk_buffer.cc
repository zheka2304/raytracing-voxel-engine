#include "chunk_buffer.h"

#include <cstring>
#include <iostream>
#include "voxel/engine/world/chunk.h"
#include "voxel/engine/world/chunk_source.h"


namespace voxel {
namespace render {


void FetchedChunksList::runDataUpdate() {
    m_fetched_chunks.clear();

    ThreadLock lock(m_mutex);
    math::Vec3i offset = m_map_offset;
    math::Vec3i size = m_map_dimensions;

    i32 i = 0;
    for (u32 request_count : m_raw_data) {
        if (request_count > 0) {
            i32 index = i;
            i32 x = index % size.x; index /= size.x;
            i32 y = index % size.y; index /= size.y;
            i32 z = index % size.z;
            m_fetched_chunks.push_back({ ChunkPosition(x + offset.x, y + offset.y, z + offset.z), request_count });
        }
        i++;
    }

    beginIteration();
}

std::vector<FetchedChunksList::ChunkPosAndWeight>& FetchedChunksList::getChunksToFetch() {
    return m_fetched_chunks;
}

void FetchedChunksList::beginIteration() {
    m_iteration_index = 0;
}

void FetchedChunksList::endIteration() {
    m_iteration_index = m_fetched_chunks.size();
}

FetchedChunksList::ChunkPosAndWeight FetchedChunksList::next() {
    return m_fetched_chunks[m_iteration_index++];
}

bool FetchedChunksList::hasNext() {
    return m_iteration_index != m_fetched_chunks.size();
}


ChunkBuffer::ChunkBuffer(i32 page_count, i32 page_size, math::Vec3i map_buffer_dimensions) :
        m_page_size(page_size),
        m_chunk_by_page(page_count, ChunkRef::invalid()),
        m_data_shader_buffer("world.chunk_data_buffer"),
        m_map_shader_buffer("world.chunk_map_buffer"),
        m_fetch_shader_buffer("world.chunk_fetch_buffer"){

    m_data_buffer_size = page_count * m_page_size;
    m_map_buffer_dimensions = map_buffer_dimensions;
    m_map_buffer_size = 6 + map_buffer_dimensions.x * map_buffer_dimensions.y * map_buffer_dimensions.z;
    m_fetch_buffer_size = map_buffer_dimensions.x * map_buffer_dimensions.y * map_buffer_dimensions.z;

    m_map_buffer = static_cast<i32*>(calloc(m_map_buffer_size, sizeof(u32)));

    m_data_shader_buffer.preallocate(m_data_buffer_size * sizeof(u32), GL_DYNAMIC_DRAW);
    m_map_shader_buffer.preallocate(m_map_buffer_size * sizeof(u32), GL_DYNAMIC_DRAW);
    m_fetch_shader_buffer.preallocate(m_fetch_buffer_size * sizeof(u32), GL_DYNAMIC_DRAW);
    m_fetch_shader_buffer.clear(GL_R32UI, GL_RED, GL_UNSIGNED_INT);
}

ChunkBuffer::~ChunkBuffer() {
    delete(m_map_buffer);
}

f32 ChunkBuffer::getMemoryRatio() {
    return m_allocated_page_count / f32(m_data_buffer_size / m_page_size);
}

void ChunkBuffer::prepareAndBind(RenderContext& ctx) {
    m_fetch_shader_buffer.clear(GL_R32UI, GL_RED, GL_UNSIGNED_INT);
    m_map_shader_buffer.setDataSpan(0, m_map_buffer_size * sizeof(i32), m_map_buffer);

    opengl::ShaderManager& shader_manager = ctx.getShaderManager();
    m_data_shader_buffer.bind(shader_manager);
    m_map_shader_buffer.bind(shader_manager);
    m_fetch_shader_buffer.bind(shader_manager);
}

void ChunkBuffer::runUploadQueue(ChunkSource& chunk_source) {
    i32 size = m_upload_request_queue.getSize();
    for (i32 i = 0; i < size; i++) {
        std::optional<UploadRequest> popped = m_upload_request_queue.tryPop();
        if (popped.has_value()) {
            UploadRequest request = popped.value();
            chunk_source.accessChunk<chunk_access_policy_weak>(request.chunk_ref, [&](Chunk& chunk) {
                i32 buffer_size = chunk.getBufferSize();
                if (buffer_size <= request.allocated_page_count * m_page_size) {
                    m_data_shader_buffer.setDataSpan(request.offset_page * m_page_size * sizeof(u32), buffer_size * sizeof(u32), chunk.getBuffer());
                }
            }, [&] (bool exists) {
                if (exists) {
                    m_upload_request_queue.push(request);
                }
            });
        }
    }
}

void ChunkBuffer::rebuildChunkMap(math::Vec3i offset) {
    // calculate offset
    m_map_buffer_offset = offset - m_map_buffer_dimensions / 2;

    // clear map, write offset and dimensions
    memset(m_map_buffer, 0xFFu, m_map_buffer_size * sizeof(u32));
    m_map_buffer[0] = m_map_buffer_offset.x;
    m_map_buffer[1] = m_map_buffer_offset.y;
    m_map_buffer[2] = m_map_buffer_offset.z;
    m_map_buffer[3] = m_map_buffer_dimensions.x;
    m_map_buffer[4] = m_map_buffer_dimensions.y;
    m_map_buffer[5] = m_map_buffer_dimensions.z;

    // iterate over all chunks
    i32 successful_chunk_count = 0;
    i32 valid_chunk_count = 0;
    for (PagedChunk& paged_chunk : m_paged_chunks) {
        // skip released chunks
        if (!paged_chunk.chunk_ref.valid())
            continue;

        // get map index
        i32 map_index = getMapIndex(paged_chunk.chunk_ref.position());

        // check if chunk is in the map
        if (map_index == -1) {
            // if it is not in map, deallocate chunk memory
            for (i32 i = paged_chunk.begin_page; i < paged_chunk.end_page; i++) {
                m_chunk_by_page[i] = ChunkRef::invalid();
                m_allocated_page_count--;
            }

            // release chunk (this will not add or remove elements to m_paged_chunks)
            releasePagedChunk(paged_chunk);
        } else {
            // else, write chunk to map
            m_map_buffer[map_index] = paged_chunk.begin_page * m_page_size;
            successful_chunk_count++;
        }
        valid_chunk_count++;
    }
#if VOXEL_ENGINE_ENABLE_DEBUG_VERBOSE
    std::cout << "chunk map rebuilt: " << successful_chunk_count << "/" << valid_chunk_count << "(" << m_paged_chunks.size() << ")\n";
#endif
}

void ChunkBuffer::getFetchedChunks(FetchedChunksList& fetched_chunks_list) {
    if (fetched_chunks_list.m_raw_data.size() != m_fetch_buffer_size) {
        // only resizing operation requires lock
        ThreadLock lock(fetched_chunks_list.m_mutex);
        fetched_chunks_list.m_raw_data.resize(m_fetch_buffer_size);
    }
    fetched_chunks_list.m_map_offset = m_map_buffer_offset;
    fetched_chunks_list.m_map_dimensions = m_map_buffer_dimensions;
    m_fetch_shader_buffer.getDataSpan(0, m_fetch_buffer_size * sizeof(u32), &fetched_chunks_list.m_raw_data[0]);
}

void ChunkBuffer::updateChunkPriority(Chunk& chunk, i64 priority) {
    if (auto found = m_paged_chunk_by_ref.find(ChunkRef(chunk)); found != m_paged_chunk_by_ref.end()) {
        PagedChunk& paged_chunk = m_paged_chunks[found->second];
        paged_chunk.usage_priority = priority;
    } else {
        uploadChunk(chunk, priority);
    }
}

void ChunkBuffer::uploadChunk(Chunk& chunk, i64 priority) {
    ChunkRef chunk_ref(chunk);

    const i32 map_index = getMapIndex(chunk.getPosition());
    if (map_index == -1) {
        // chunk is not in range, covered by map, it will not be visible in any case
        return;
    }

    i32 buffer_offset;
    const i32 required_pages = (chunk.getBufferSize() + m_page_size - 1) / m_page_size;

    if (auto found = m_paged_chunk_by_ref.find(chunk_ref); found != m_paged_chunk_by_ref.end()) {
        // if the chunk is already allocated, reallocate it:
        PagedChunk& paged_chunk = m_paged_chunks[found->second];

        // if we require more pages, than we have allocated
        if (paged_chunk.end_page - paged_chunk.begin_page < required_pages) {
            // check, if we have enough space after already allocated memory
            bool can_reallocate_in_place = true;
            for (i32 i = paged_chunk.end_page; i < paged_chunk.begin_page + required_pages; i++) {
                ChunkRef chunk_by_page = m_chunk_by_page[i];
                if (chunk_by_page.valid() && chunk_by_page != chunk_ref) {
                    can_reallocate_in_place = false;
                    break;
                }
            }

            if (can_reallocate_in_place) {
                // if we can, just mark new pages to belong to this chunk
                buffer_offset = paged_chunk.begin_page;
                for (i32 i = paged_chunk.end_page; i < paged_chunk.begin_page + required_pages; i++) {
                    m_chunk_by_page[i] = chunk_ref;
                    m_allocated_page_count++;
                }
            } else {
                // if we can't do this, fully free previous allocation
                for (i32 i = paged_chunk.begin_page; i < paged_chunk.end_page; i++) {
                    m_chunk_by_page[i] = ChunkRef::invalid();
                    m_allocated_page_count--;
                }

                // temporary mark current chunk as released, so it will be ignored during memory allocation
                paged_chunk.chunk_ref = ChunkRef::invalid();

                // then allocate a new span
                buffer_offset = allocatePageSpan(required_pages, chunk_ref, priority);
                if (buffer_offset == -1) {
                    // if new allocation failed - release paged chunk and return
                    paged_chunk.chunk_ref = chunk_ref;
                    releasePagedChunk(paged_chunk);
                    return;
                }

                // restore current chunk ref
                paged_chunk.chunk_ref = chunk_ref;
            }
        } else {
            // if we require less (or equal) amount pages we already have, free excess pages of previous allocation
            buffer_offset = paged_chunk.begin_page;
            for (i32 i = paged_chunk.begin_page + required_pages; i < paged_chunk.end_page; i++) {
                m_chunk_by_page[i] = ChunkRef::invalid();
                m_allocated_page_count--;
            }
        }

        // if allocation succeeds, update paged chunk
        paged_chunk.begin_page = buffer_offset;
        paged_chunk.end_page = buffer_offset + required_pages;
    } else {
        // if chunk was not yet allocated, just allocate a new span
        buffer_offset = allocatePageSpan(required_pages, chunk_ref, priority);
        if (buffer_offset != -1) {
            // if allocated successfully, add new paged chunk
            addPagedChunk({chunk_ref, buffer_offset, buffer_offset + required_pages, priority});
        } else {
            // if allocation failed, exit
            return;
        }
    }

    if (buffer_offset != -1) {
        // copy chunk buffer to shader buffer
        m_upload_request_queue.push({ ChunkRef(chunk), buffer_offset, required_pages });

        // update map buffer
        m_map_buffer[map_index] = buffer_offset * m_page_size;
    }
}

void ChunkBuffer::removeChunk(ChunkRef chunk_ref) {
    // remove chunk from map
    const i32 map_index = getMapIndex(chunk_ref.position());
    if (map_index != -1) {
        m_map_buffer[map_index] = -1;
    }

    if (auto found = m_paged_chunk_by_ref.find(chunk_ref); found != m_paged_chunk_by_ref.end()) {
        PagedChunk& paged_chunk = m_paged_chunks[found->second];

        for (i32 i = paged_chunk.begin_page; i < paged_chunk.end_page; i++) {
            m_chunk_by_page[i] = ChunkRef::invalid();
            m_allocated_page_count--;
        }

        releasePagedChunk(paged_chunk);
    }
}

i32 ChunkBuffer::tryAllocatePageSpan(i32 page_count, ChunkRef chunk_ref, i32 search_offset) {
    const i32 max_page = m_data_buffer_size / m_page_size;

    i32 offset_page = -1;
    for (i32 page = search_offset; page < max_page; page++) {
        const ChunkRef& chunk_by_page = m_chunk_by_page[page];
        if (!chunk_by_page.valid()) {
            if (offset_page == -1)
                offset_page = page;
            else if (offset_page + page_count <= page + 1) {
                break;
            }
        } else {
            offset_page = -1;
        }
    }

    if (offset_page == -1 || offset_page + page_count > max_page) {
        return -1;
    }

#if VOXEL_ENGINE_ENABLE_DEBUG_VERBOSE
    std::cout << "allocated chunk buffer span " << offset_page << ":" << offset_page + page_count << " (" << page_count << ")" << " total_allocated_pages=" << m_allocated_page_count << '\n';
#endif
    for (i32 page = offset_page; page < offset_page + page_count; page++) {
        m_chunk_by_page[page] = chunk_ref;
        m_allocated_page_count++;
    }
    return offset_page;
}

i32 ChunkBuffer::allocatePageSpan(i32 page_count, ChunkRef chunk_ref, i64 priority) {
    // try to allocate memory span and return it
    i32 span = tryAllocatePageSpan(page_count, chunk_ref);
    if (span >= 0) {
        return span;
    }

    // run clock algorithm
    i32 clock_iteration_count = m_paged_chunks.size();
    for (i32 i = 0; i < clock_iteration_count; i++) {
        m_clock_index %= m_paged_chunks.size();
        PagedChunk& paged_chunk = m_paged_chunks[m_clock_index++];
        if (paged_chunk.usage_priority < priority && paged_chunk.chunk_ref.valid()) {
            // if usage priority is less, than new priority, deallocate this chunk

            // deallocate pages
            paged_chunk.usage_priority = priority;
            for (i32 page = paged_chunk.begin_page; page < paged_chunk.end_page; page++) {
                m_chunk_by_page[page] = ChunkRef::invalid();
                m_allocated_page_count--;
            }

            // remove from map
            i32 map_index = getMapIndex(paged_chunk.chunk_ref.position());
            if (map_index >= 0) {
                m_map_buffer[map_index] = -1;
            }

            // release paged chunk
            // this will not add or remove elements to paged chunk vector, just remove it from the map
            releasePagedChunk(paged_chunk);

            // try to allocate new span
            span = tryAllocatePageSpan(page_count, chunk_ref);
            if (span >= 0) {
                return span;
            }
        } else {
            paged_chunk.usage_priority = 0;
        }
    }

    // we failed to allocate memory
    return -1;
}

i32 ChunkBuffer::addPagedChunk(const PagedChunk& paged_chunk) {
    i32 index;
    if (m_paged_chunks_to_reuse.empty()) {
        index = i32(m_paged_chunks.size());
        m_paged_chunks.emplace_back(paged_chunk);
    } else {
        index = m_paged_chunks_to_reuse.back();
        m_paged_chunks_to_reuse.pop_back();
        m_paged_chunks[index] = paged_chunk;
    }
    m_paged_chunk_by_ref[paged_chunk.chunk_ref] = index;
    return index;
}

void ChunkBuffer::releasePagedChunk(const PagedChunk& paged_chunk) {
    if (auto found = m_paged_chunk_by_ref.find(paged_chunk.chunk_ref); found != m_paged_chunk_by_ref.end()) {
        i32 index = found->second;
        m_paged_chunk_by_ref.erase(found);
        m_paged_chunks[index].chunk_ref = ChunkRef::invalid();
        m_paged_chunks[index].usage_priority = -1;
    }
}

i32 ChunkBuffer::getMapIndex(ChunkPosition position) {
    math::Vec3i pos = math::Vec3i(position.x, position.y, position.z) - m_map_buffer_offset;
    if (pos.x >= 0 && pos.y >= 0 && pos.z >= 0 && pos.x < m_map_buffer_dimensions.x && pos.y < m_map_buffer_dimensions.y && pos.z < m_map_buffer_dimensions.z) {
        return 6 + pos.x + (pos.y + pos.z * m_map_buffer_dimensions.y) * m_map_buffer_dimensions.x;
    }
    return -1;
}

} // render
} // voxel