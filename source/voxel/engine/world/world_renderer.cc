#include "world_renderer.h"

#include <iostream>


namespace voxel {

WorldRenderer::WorldRenderer(Shared<ChunkSource> chunk_source, Shared<render::ChunkBuffer> chunk_buffer, WorldRendererSettings settings) :
    m_chunk_source(chunk_source), m_chunk_buffer(chunk_buffer), m_settings(settings) {
    m_chunk_source->addListener(this);
}

WorldRenderer::~WorldRenderer() {
    m_chunk_source->removeListener(this);
}

void WorldRenderer::addChunkToUpdateQueue(Shared<Chunk> chunk) {
    m_chunk_updates.push(chunk);
}

void WorldRenderer::render(render::RenderContext& render_context) {
    if (m_request_fetched_chunks) {
        m_chunk_buffer->getFetchedChunks(m_fetched_chunks_list);
        m_request_fetched_chunks = false;
    }

    for (i32 i = 0; i < m_settings.chunk_updates_per_frame; i++) {
        auto next = m_chunk_updates.tryPop();
        if (next.has_value()) {
            Shared<Chunk> chunk = next.value();
            auto pos = chunk->getPosition();
            std::cout << "chunk update: " << pos.x << ", " << pos.y << ", " << pos.z << "\n";
            if (chunk->tryLock()) {
                if (chunk->getState() == CHUNK_LOADED) {
                    m_chunk_buffer->uploadChunk(chunk);
                } else {
                    m_chunk_buffer->removeChunk(chunk);
                }
                chunk->unlock();
            } else {
                m_chunk_updates.push(chunk);
            }
        } else {
            break;
        }
    }

    m_chunk_buffer->rebuildChunkMap(math::Vec3i(0)); // TODO: remove
    m_chunk_buffer->prepareAndBind(render_context);
}

void WorldRenderer::onTick() {
    fetchRequestedChunks();
}

void WorldRenderer::fetchRequestedChunks() {
    // if there remaining chunks to fetch
    if (m_fetched_chunks_list.hasNext()) {
        // fetch some chunks
        for (int i = 0; i < m_settings.chunk_fetches_per_tick && m_fetched_chunks_list.hasNext(); i++) {
            ChunkPosition chunk_to_fetch = m_fetched_chunks_list.next();
            if (!m_chunk_source->fetchChunkAt(chunk_to_fetch)) {
                m_fetched_chunks_list.endIteration();
                break;
            }
        }

        // if no more left - request update from the next frame
        if (!m_fetched_chunks_list.hasNext()) {
            m_request_fetched_chunks = true;
        }
    } else if (!m_request_fetched_chunks) {
        // if no chunks are remaining and request update was already fulfilled by a render thread, update list of chunks to fetch
        m_fetched_chunks_list.runDataUpdate(m_settings.fetch_ray_count_threshold);

        // if no chunks are fetched - request them
        if (m_fetched_chunks_list.getChunksToFetch().empty()) {
            m_request_fetched_chunks = true;
        }
    }
}

void WorldRenderer::onChunkSourceTick(ChunkSource& chunk_source) {
    onTick();
}

void WorldRenderer::onChunkUpdated(ChunkSource& chunk_source, Shared<Chunk> chunk) {
    addChunkToUpdateQueue(chunk);
}

} // voxel