#include "chunk_buffer.h"

#include <cstring>
#include <iostream>
#include "voxel/engine/world/chunk.h"


namespace voxel {
namespace render {


void FetchedChunksList::runDataUpdate(i32 threshold) {
    m_fetched_chunks.clear();
    m_fetched_chunks.reserve(1024);

    ThreadLock lock(m_mutex);
    math::Vec3i offset = m_map_offset;
    math::Vec3i size = m_map_dimensions;

    i32 i = 0;
    for (u32 request_count : m_raw_data) {
        if (request_count > threshold) {
            i32 index = i;
            i32 x = index % size.x; index /= size.x;
            i32 y = index % size.y; index /= size.y;
            i32 z = index % size.z;
            m_fetched_chunks.emplace_back(x + offset.x, y + offset.y, z + offset.z);
        }
        i++;
    }
}

std::vector<ChunkPosition>& FetchedChunksList::getChunksToFetch() {
    return m_fetched_chunks;
}


ChunkBuffer::ChunkBuffer(i32 page_count, math::Vec3i map_buffer_dimensions) :
        m_chunk_by_page(page_count, 0),
        m_data_shader_buffer("world.chunk_data_buffer"),
        m_map_shader_buffer("world.chunk_map_buffer"),
        m_fetch_shader_buffer("world.chunk_fetch_buffer"){

    m_data_buffer_size = page_count * PAGE_SIZE;
    m_map_buffer_dimensions = map_buffer_dimensions;
    m_map_buffer_size = 6 + map_buffer_dimensions.x * map_buffer_dimensions.y * map_buffer_dimensions.z;
    m_fetch_buffer_size = map_buffer_dimensions.x * map_buffer_dimensions.y * map_buffer_dimensions.z;

    m_map_buffer = static_cast<i32*>(calloc(m_map_buffer_size, sizeof(u32)));

    m_data_shader_buffer.preallocate(m_data_buffer_size * sizeof(u32), GL_DYNAMIC_DRAW);
    m_map_shader_buffer.preallocate(m_map_buffer_size * sizeof(u32), GL_DYNAMIC_DRAW);
    m_fetch_shader_buffer.preallocate(m_fetch_buffer_size * sizeof(u32), GL_DYNAMIC_DRAW);
}

ChunkBuffer::~ChunkBuffer() {
    delete(m_map_buffer);
}

void ChunkBuffer::prepareAndBind(RenderContext& ctx) {
    m_fetch_shader_buffer.clear(GL_R32UI, GL_RED, GL_UNSIGNED_INT);
    m_map_shader_buffer.setDataSpan(0, m_map_buffer_size * sizeof(i32), m_map_buffer);

    opengl::ShaderManager& shader_manager = ctx.getShaderManager();
    m_data_shader_buffer.bind(shader_manager);
    m_map_shader_buffer.bind(shader_manager);
    m_fetch_shader_buffer.bind(shader_manager);
}

void ChunkBuffer::rebuildChunkMap(math::Vec3i offset) {
    ThreadLock lock(m_page_map_lock);

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
    for (auto it = m_pages_by_chunk.begin(); it != m_pages_by_chunk.end();) {
        auto& span = it->second;

        // check, if chunk is already expired
        Shared<Chunk> chunk = span.chunk.lock();
        if (!chunk) {
            it = m_pages_by_chunk.erase(it);
            continue;
        }

        // check if chunk is in map
        i32 map_index = getMapIndex(chunk->getPosition());
        if (map_index == -1) {
            // remove chunk
            for (i32 i = span.first; i < span.second; i++) {
                m_chunk_by_page[i] = 0;
            }
            it = m_pages_by_chunk.erase(it);
            continue;
        }

        // write chunk to map
        m_map_buffer[map_index] = span.first * PAGE_SIZE;
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

ChunkBuffer::ChunkUploadResult ChunkBuffer::uploadChunk(Shared<Chunk> chunk) {
    const i32 map_index = getMapIndex(chunk->getPosition());
    if (map_index == -1) {
        // chunk is not in range, covered by map, it will not be visible in any case
        return RESULT_OUT_OF_RANGE;
    }

    const i32 required_pages = (chunk->getBufferSize() + PAGE_SIZE - 1) / PAGE_SIZE;
    const u64 chunk_hash = reinterpret_cast<u64>(chunk.get());
    i32 buffer_offset;

    ThreadLock lock(m_page_map_lock);
    auto found = m_pages_by_chunk.find(chunk_hash);
    if (found != m_pages_by_chunk.end()) {
        // if the chunk is already allocated, reallocate it:
        auto& span = found->second;

        // if we require more pages, than we have allocated
        if (span.second - span.first > required_pages) {
            // check, if we have enough space after already allocated memory
            bool can_reallocate_in_place = true;
            for (i32 i = span.second; i < span.first + required_pages; i++) {
                u64 chunk_by_page = m_chunk_by_page[i];
                if (chunk_by_page != 0 && chunk_by_page != chunk_hash) {
                    can_reallocate_in_place = false;
                    break;
                }
            }

            if (can_reallocate_in_place) {
                // if we can, just mark new pages to belong to this chunk
                buffer_offset = span.first;
                for (i32 i = span.second; i < span.first + required_pages; i++) {
                    m_chunk_by_page[i] = chunk_hash;
                }
            } else {
                // if we cant do this, fully free previous allocation
                for (i32 i = span.first; i < span.second; i++) {
                    m_chunk_by_page[i] = 0;
                }

                // then allocate a new span
                buffer_offset = allocatePageSpan(required_pages, chunk_hash);
                if (buffer_offset == -1) {
                    // if new allocation failed - remove all info about this chunk and return
                    m_pages_by_chunk.erase(found);
                    return RESULT_ALLOCATION_FAILED;
                }
            }
        } else {
            // if we require less (or equal) amount pages we already have, free excess pages of previous allocation
            buffer_offset = span.first;
            for (i32 i = span.first + required_pages; i < span.second; i++) {
                m_chunk_by_page[i] = 0;
            }
        }

        // at this point in any case, span.first will contain offset of the valid reallocated span, exactly required_pages length
        span.second = span.first + required_pages;
    } else {
        // if chunk was not yet allocated, just allocate a new span
        buffer_offset = allocatePageSpan(required_pages, chunk_hash);

        // check, if allocation was successful, and store new span, if it was
        if (buffer_offset != -1) {
            m_pages_by_chunk[chunk_hash] = { Weak<Chunk>(chunk), buffer_offset, buffer_offset + required_pages };
        } else {
            return RESULT_ALLOCATION_FAILED;
        }
    }
    lock.unlock();

    std::cout << "uploading chunk to " << buffer_offset << ":" << buffer_offset + chunk->getBufferSize() << '\n';

    if (buffer_offset != -1) {
        // copy chunk buffer to shader buffer
        m_data_shader_buffer.setDataSpan(buffer_offset * PAGE_SIZE * sizeof(u32), chunk->getBufferSize() * sizeof(u32), chunk->getBuffer());

        // update map buffer
        m_map_buffer[map_index] = buffer_offset * PAGE_SIZE;

        return RESULT_SUCCESS;
    }
    return RESULT_ALLOCATION_FAILED;
}

void ChunkBuffer::removeChunk(Shared<Chunk> chunk) {
    // remove chunk from map
    const i32 map_index = getMapIndex(chunk->getPosition());
    if (map_index != -1) {
        m_map_buffer[map_index] = -1;
    }

    // find allocated span and remove it
    u64 chunk_hash = reinterpret_cast<u64>(chunk.get());
    ThreadLock lock(m_page_map_lock);
    auto found = m_pages_by_chunk.find(chunk_hash);
    if (found != m_pages_by_chunk.end()) {
        auto& span = found->second;
        for (i32 i = span.first; i < span.second; i++) {
            m_chunk_by_page[i] = 0;
        }
    }
}

i32 ChunkBuffer::allocatePageSpan(i32 page_count, u64 chunk) {
    const i32 max_page = m_data_buffer_size / PAGE_SIZE;

    i32 offset_page = -1;
    for (i32 page = 0; page < max_page; page++) {
        i32 chunk_by_page = m_chunk_by_page[page];
        if (chunk_by_page == 0) {
            if (offset_page == -1)
                offset_page = page;
            else if (offset_page + page_count <= page) {
                break;
            }
        } else {
            offset_page = -1;
        }
    }

    if (offset_page == -1 || offset_page + page_count > max_page) {
        return -1;
    }

    std::cout << "allocated chunk buffer span " << offset_page << ":" << offset_page + page_count << '\n';
    for (i32 page = offset_page; page < offset_page + page_count; page++) {
        m_chunk_by_page[page] = chunk;
    }
    return offset_page;
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