#include "chunk_buffer.h"

#include <cstring>
#include <iostream>
#include "voxel/engine/world/chunk.h"

#define DEBUG_VERBOSE 0


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
    m_fetched_chunks_iter = m_fetched_chunks.begin();
}

void FetchedChunksList::endIteration() {
    m_fetched_chunks_iter = m_fetched_chunks.end();
}

FetchedChunksList::ChunkPosAndWeight FetchedChunksList::next() {
    return *(m_fetched_chunks_iter++);
}

bool FetchedChunksList::hasNext() {
    return m_fetched_chunks_iter != m_fetched_chunks.end();
}


ChunkBuffer::ChunkBuffer(i32 page_count, i32 page_size, math::Vec3i map_buffer_dimensions) :
        m_page_size(page_size),
        m_chunk_by_page(page_count, 0),
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

void ChunkBuffer::runUploadQueue() {
    i32 size = m_upload_request_queue.getSize();
    for (i32 i = 0; i < size; i++) {
        std::optional<UploadRequest> popped = m_upload_request_queue.tryPop();
        if (popped.has_value()) {
            UploadRequest request = popped.value();
            Shared<Chunk> chunk = request.chunk.lock();
            if (chunk) {
                if (chunk->tryLock()) {
                    i32 buffer_size = chunk->getBufferSize();
                    if (buffer_size <= request.allocated_page_count * m_page_size) {
                        m_data_shader_buffer.setDataSpan(request.offset_page * m_page_size * sizeof(u32), buffer_size * sizeof(u32), chunk->getBuffer());
                    }
                    chunk->unlock();
                } else {
                    m_upload_request_queue.push(request);
                }
            }
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
    for (auto it = m_uploaded_chunks.begin(); it != m_uploaded_chunks.end();) {
        UploadedChunk& uploaded_chunk = it->second;

        // check, if chunk is destroyed
        Shared<Chunk> chunk = uploaded_chunk.chunk.lock();
        if (!chunk) {
            if (uploaded_chunk.allocated) {
                m_allocated_chunk_heap.removeItem(&uploaded_chunk);
            } else {
                m_pending_chunk_heap.removeItem(&uploaded_chunk);
            }
            it = m_uploaded_chunks.erase(it);
            continue;
        }

        // get map index
        i32 map_index = getMapIndex(chunk->getPosition());

        if (uploaded_chunk.allocated) {
            // if chunk is allocated, check if chunk is in map
            if (map_index == -1) {
                // if it is not in map, deallocate chunk memory
                for (i32 i = uploaded_chunk.first; i < uploaded_chunk.second; i++) {
                    m_chunk_by_page[i] = 0;
                    m_allocated_page_count--;
                }

                // move chunk to pending heap
                m_allocated_chunk_heap.removeItem(&uploaded_chunk);
                uploaded_chunk.allocated = false;
                m_pending_chunk_heap.addItem(&uploaded_chunk);
            } else {
                // else, write chunk to map
                m_map_buffer[map_index] = uploaded_chunk.first * m_page_size;
            }
        }
        it++;
    }
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

void ChunkBuffer::updateChunkPriority(Shared<Chunk> chunk, i64 priority) {
    const u64 chunk_hash = reinterpret_cast<u64>(chunk.get());
    auto found = m_uploaded_chunks.find(chunk_hash);
    if (found != m_uploaded_chunks.end()) {
        // get chunk and update its priority
        UploadedChunk& uploaded_chunk = found->second;
        uploaded_chunk.priority = priority;

        if (uploaded_chunk.allocated) {
            // if chunk is already allocated, just update it in allocated chunks heap and exit
            m_allocated_chunk_heap.updateItem(&uploaded_chunk);
            return;
        } else {
            UploadedChunk* min_priority_chunk = m_allocated_chunk_heap.getFirst();
            if (!min_priority_chunk || priority > min_priority_chunk->priority) {
                // if chunk has enough priority to be in allocated queue

                // check, if chunk inside map
                i32 map_index = getMapIndex(chunk->getPosition());
                if (map_index >= 0) {
                    // try allocate chunk
                    const i32 required_pages = (chunk->getBufferSize() + m_page_size - 1) / m_page_size;
                    i32 allocated_span = allocatePageSpan(required_pages, chunk_hash, priority);

                    // if allocation succeeds, upload chunk and move from pending chunk heap to allocated chunk heap
                    if (allocated_span >= 0) {
                        m_pending_chunk_heap.removeItem(&uploaded_chunk);
                        uploaded_chunk.allocated = true;
                        uploaded_chunk.first = allocated_span;
                        uploaded_chunk.second = allocated_span + required_pages;
                        m_allocated_chunk_heap.addItem(&uploaded_chunk);
                        m_upload_request_queue.push({Weak<Chunk>(chunk), allocated_span, required_pages});
                        m_map_buffer[map_index] = allocated_span * m_page_size;
                        return;
                    }
                }
            }
        }

        // if anything else failed, update chunk in pending chunks heap
        m_pending_chunk_heap.updateItem(&uploaded_chunk);
    }
}

void ChunkBuffer::uploadChunk(Shared<Chunk> chunk, i64 priority) {
    const u64 chunk_hash = reinterpret_cast<u64>(chunk.get());
    const i32 map_index = getMapIndex(chunk->getPosition());
    if (map_index == -1) {
        // chunk is not in range, covered by map, it will not be visible in any case
        // add it to pending queue
        UploadedChunk& uploaded_chunk = m_uploaded_chunks.emplace(chunk_hash, UploadedChunk(Weak<Chunk>(chunk), chunk_hash, priority)).first->second;
        m_pending_chunk_heap.addItem(&uploaded_chunk);
        return;
    }

    i32 buffer_offset;
    const i32 required_pages = (chunk->getBufferSize() + m_page_size - 1) / m_page_size;

    auto found = m_uploaded_chunks.find(chunk_hash);
    if (found != m_uploaded_chunks.end()) {
        // if the chunk is already allocated, reallocate it:
        UploadedChunk& uploaded_chunk = found->second;

        if (!uploaded_chunk.allocated) {
            // if chunk is not allocated, this will allocate it, if it has enough priority
            updateChunkPriority(chunk, priority);
            return;
        } else {
            // chunk is already allocated

            // update chunk priority
            uploaded_chunk.priority = priority;
            m_allocated_chunk_heap.updateItem(&uploaded_chunk);

            // if we require more pages, than we have allocated
            if (uploaded_chunk.second - uploaded_chunk.first > required_pages) {
                // check, if we have enough space after already allocated memory
                bool can_reallocate_in_place = true;
                for (i32 i = uploaded_chunk.second; i < uploaded_chunk.first + required_pages; i++) {
                    u64 chunk_by_page = m_chunk_by_page[i];
                    if (chunk_by_page != 0 && chunk_by_page != chunk_hash) {
                        can_reallocate_in_place = false;
                        break;
                    }
                }

                if (can_reallocate_in_place) {
                    // if we can, just mark new pages to belong to this chunk
                    buffer_offset = uploaded_chunk.first;
                    for (i32 i = uploaded_chunk.second; i < uploaded_chunk.first + required_pages; i++) {
                        m_chunk_by_page[i] = chunk_hash;
                        m_allocated_page_count++;
                    }
                } else {
                    // if we cant do this, fully free previous allocation
                    for (i32 i = uploaded_chunk.first; i < uploaded_chunk.second; i++) {
                        m_chunk_by_page[i] = 0;
                        m_allocated_page_count--;
                    }

                    // then allocate a new span
                    buffer_offset = allocatePageSpan(required_pages, chunk_hash, priority);
                    if (buffer_offset == -1) {
                        // if new allocation failed - remove all info about this chunk and return
                        return;
                    }
                    uploaded_chunk.first = buffer_offset;
                }
            } else {
                // if we require less (or equal) amount pages we already have, free excess pages of previous allocation
                buffer_offset = uploaded_chunk.first;
                for (i32 i = uploaded_chunk.first + required_pages; i < uploaded_chunk.second; i++) {
                    m_chunk_by_page[i] = 0;
                    m_allocated_page_count--;
                }
            }

            // at this point in any case, span.first will contain offset of the valid reallocated span, exactly required_pages length
            uploaded_chunk.second = uploaded_chunk.first + required_pages;
        }
    } else {
        // if chunk was not yet allocated, just allocate a new span
        buffer_offset = allocatePageSpan(required_pages, chunk_hash, priority);
        if (buffer_offset != -1) {
            // if allocated successfully, add to allocated heap
            UploadedChunk& uploaded_chunk = m_uploaded_chunks.emplace(chunk_hash, UploadedChunk(Weak<Chunk>(chunk), chunk_hash, buffer_offset, buffer_offset + required_pages, priority)).first->second;
            m_allocated_chunk_heap.addItem(&uploaded_chunk);
        } else {
            // if allocation failed, add to pending heap and exit
            UploadedChunk& uploaded_chunk = m_uploaded_chunks.emplace(chunk_hash, UploadedChunk(Weak<Chunk>(chunk), chunk_hash, priority)).first->second;
            m_pending_chunk_heap.addItem(&uploaded_chunk);
            return;
        }
    }

    if (buffer_offset != -1) {
        // copy chunk buffer to shader buffer
        m_upload_request_queue.push({ Weak<Chunk>(chunk), buffer_offset, required_pages });

        // update map buffer
        m_map_buffer[map_index] = buffer_offset * m_page_size;
    }
}

void ChunkBuffer::removeChunk(Shared<Chunk> chunk) {
    // remove chunk from map
    const i32 map_index = getMapIndex(chunk->getPosition());
    if (map_index != -1) {
        m_map_buffer[map_index] = -1;
    }

    // find uploaded chunk
    u64 chunk_hash = reinterpret_cast<u64>(chunk.get());
    auto found = m_uploaded_chunks.find(chunk_hash);
    if (found != m_uploaded_chunks.end()) {
        UploadedChunk& uploaded_chunk = found->second;

        if (uploaded_chunk.allocated) {
            // if allocated, deallocate chunk span
            for (i32 i = uploaded_chunk.first; i < uploaded_chunk.second; i++) {
                m_chunk_by_page[i] = 0;
                m_allocated_page_count--;
            }

            // remove it from allocated heap
            m_allocated_chunk_heap.removeItem(&uploaded_chunk);
        } else {
            // if deallocated, remove it from deallocated heap
            m_pending_chunk_heap.removeItem(&uploaded_chunk);
        }

        // remove from lookup map
        m_uploaded_chunks.erase(found);
    }
}

i32 ChunkBuffer::tryAllocatePageSpan(i32 page_count, u64 chunk) {
    const i32 max_page = m_data_buffer_size / m_page_size;

    i32 offset_page = -1;
    for (i32 page = 0; page < max_page; page++) {
        i32 chunk_by_page = m_chunk_by_page[page];
        if (chunk_by_page == 0) {
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

#if DEBUG_VERBOSE
    std::cout << "allocated chunk buffer span " << offset_page << ":" << offset_page + page_count << " total_allocated_pages=" << m_allocated_page_count << '\n';
#endif
    for (i32 page = offset_page; page < offset_page + page_count; page++) {
        m_chunk_by_page[page] = chunk;
        m_allocated_page_count++;
    }
    return offset_page;
}

i32 ChunkBuffer::allocatePageSpan(i32 page_count, u64 chunk_hash, i64 priority) {
    while (true) {
        // try allocate memory span and return it
        i32 span = tryAllocatePageSpan(page_count, chunk_hash);
        if (span >= 0) {
            return span;
        }

        // if allocation failed, get allocated chunk with min priority
        UploadedChunk* chunk_to_deallocate = m_allocated_chunk_heap.getFirst();
        if (!chunk_to_deallocate || chunk_to_deallocate->priority > priority || chunk_to_deallocate->chunk_hash == chunk_hash) {
            // allocation failed, if:
            // - no more chunks are allocated
            // - min priority is still greater, that ours
            // - deallocated chunk equals to chunk, we are trying to allocate
            return -1;
        }
        // deallocate next chunk

        // move it from allocated heap to pending heap
        chunk_to_deallocate = m_allocated_chunk_heap.popFirst();
        chunk_to_deallocate->allocated = false;
        m_pending_chunk_heap.addItem(chunk_to_deallocate);

        // deallocate span
        for (i32 page = chunk_to_deallocate->first; page < chunk_to_deallocate->second; page++) {
            m_chunk_by_page[page] = 0;
            m_allocated_page_count--;
        }

        // remove from map
        Shared<Chunk> chunk = chunk_to_deallocate->chunk.lock();
        if (chunk) {
            i32 map_index = getMapIndex(chunk->getPosition());
            if (map_index >= 0) {
                m_map_buffer[map_index] = -1;
            }
        }
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