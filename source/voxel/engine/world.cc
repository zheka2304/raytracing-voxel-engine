#include "world.h"

namespace voxel {

World::World(const threading::TickingThread::Scheduler& tick_scheduler,
             Shared<ChunkProvider> chunk_provider,
             Shared<ChunkStorage> chunk_storage,
             Shared<threading::TaskExecutor> background_executor) :

             m_chunk_source(chunk_provider, chunk_storage, background_executor),
             m_ticking_thread(tick_scheduler, std::bind(&World::onTick, this)),
             m_background_executor(background_executor) {
}

World::~World() {
    m_ticking_thread.join();
}

ChunkSource& World::getChunkSource() {
    return m_chunk_source;
}

threading::TaskExecutor& World::getBackgroundExecutor() {
    return *m_background_executor;
}

void World::onTick() {
    m_chunk_source.onTick();
}

} // voxel