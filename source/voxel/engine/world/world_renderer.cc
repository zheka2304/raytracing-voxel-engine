#include "world_renderer.h"

#include <iostream>
#include "voxel/common/profiler.h"


namespace voxel {

WorldRenderer::WorldRenderer(Shared<ChunkSource> chunk_source, Unique<render::ChunkBuffer> chunk_buffer, WorldRendererSettings settings) :
    m_chunk_source(chunk_source), m_chunk_buffer(std::move(chunk_buffer)), m_settings(settings) {
    m_chunk_source->addListener(this);
    m_chunk_buffer->rebuildChunkMap(m_chunk_map_offset_position);
    m_camera_loading_region = m_chunk_source->addLoadingRegion(m_chunk_map_offset_position, m_settings.chunk_loading_level);
}

WorldRenderer::~WorldRenderer() {
    m_chunk_source->removeLoadingRegion(m_camera_loading_region);
    m_chunk_source->removeListener(this);
}

void WorldRenderer::setCameraPosition(math::Vec3f camera_position) {
    m_camera_position = camera_position;
    math::Vec3i camera_chunk_position = math::floor_to_int(m_camera_position);
    m_camera_loading_region->setPosition(camera_chunk_position);
    if (math::len(camera_chunk_position - m_chunk_map_offset_position) > m_settings.buffer_offset_update_distance) {
        m_chunk_map_offset_position = camera_chunk_position;
        m_rebuild_chunk_map = true;
    }
}

void WorldRenderer::addChunkToUpdateQueue(ChunkRef chunk_ref) {
    m_chunk_updates.push(chunk_ref);
}

void WorldRenderer::render(render::RenderContext& render_context) {
    if (m_request_fetched_chunks.exchange(false)) {
        m_chunk_buffer->getFetchedChunks(m_fetched_chunks_list);
    }

    m_chunk_buffer->runUploadQueue(*m_chunk_source);
    m_chunk_buffer->prepareAndBind(render_context);
}

void WorldRenderer::onTick() {
    VOXEL_ENGINE_PROFILE_SCOPE(world_renderer_tick);

    if (m_rebuild_chunk_map.exchange(false)) {
        m_chunk_buffer->rebuildChunkMap(m_chunk_map_offset_position);
    }
    fetchRequestedChunks();
    runChunkUpdates();
}

void WorldRenderer::fetchRequestedChunks() {
    VOXEL_ENGINE_PROFILE_SCOPE(world_renderer_fetch_chunks);

    // if there remaining chunks to fetch
    if (m_fetched_chunks_list.hasNext()) {
        // fetch some chunks, requested by gpu
        for (i32 i = 0; i < m_settings.chunk_fetches_per_tick && m_fetched_chunks_list.hasNext(); i++) {
            auto chunk_to_fetch = m_fetched_chunks_list.next();
            i64 priority = m_chunk_fetch_priority * 64 + chunk_to_fetch.weight;
            m_chunk_source->fetchChunkAt(chunk_to_fetch.pos, priority, [&] (Chunk& chunk) {
                m_chunk_buffer->updateChunkPriority(chunk, priority);
            });
        }

        // if all chunks fetched - request more
        if (!m_fetched_chunks_list.hasNext()) {
            m_chunk_fetch_priority++;
            m_request_fetched_chunks = true;
        }
    } else if (!m_request_fetched_chunks) {
        // if no chunks are remaining and request update was already fulfilled by a render thread, update list of chunks to fetch
        m_fetched_chunks_list.runDataUpdate();
        m_fetched_chunks_list.beginIteration();

        // if no chunks are fetched - request them
        if (m_fetched_chunks_list.getChunksToFetch().empty()) {
            m_chunk_fetch_priority++;
            m_request_fetched_chunks = true;
        }
    }
}

void WorldRenderer::runChunkUpdates() {
    VOXEL_ENGINE_PROFILE_SCOPE(world_renderer_update_chunks);

    for (i32 i = 0; i < m_settings.chunk_updates_per_tick; i++) {
        auto next = m_chunk_updates.tryPop();
        if (next.has_value()) {
            ChunkRef chunk_ref = next.value();
            m_chunk_source->accessChunk<chunk_access_policy_weak>(chunk_ref, [&](Chunk& chunk) {
                if (chunk.getState() == CHUNK_LOADED) {
                    m_chunk_buffer->uploadChunk(chunk, 0);
                } else {
                    m_chunk_buffer->removeChunk(chunk_ref);
                }
            }, [&] (bool exists) {
                if (exists) {
                    m_chunk_updates.push(chunk_ref);
                } else {
                    m_chunk_buffer->removeChunk(chunk_ref);
                };
            });
        } else {
            break;
        }
    }
}

void WorldRenderer::onChunkSourceTick(ChunkSource& chunk_source) {
    onTick();
}

void WorldRenderer::onChunkUpdated(ChunkSource& chunk_source, ChunkRef chunk_ref) {
    addChunkToUpdateQueue(chunk_ref);
}

} // voxel