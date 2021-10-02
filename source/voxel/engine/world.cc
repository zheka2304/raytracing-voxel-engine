#include "world.h"

namespace voxel {

World::World(Shared<ChunkProvider> chunk_provider,
             Shared<ChunkStorage> chunk_storage,
             const threading::TickingThread::Scheduler& tick_scheduler,
             ChunkSource::Settings chunk_source_settings) :

             m_chunk_source(CreateShared<ChunkSource>(chunk_provider, chunk_storage, chunk_source_settings)),
             m_ticking_thread(tick_scheduler, std::bind(&World::onTick, this)) {
}

World::~World() {
    setTicking(false);
    m_ticking_thread.join();
}

void World::setTicking(bool ticking) {
    m_ticking_thread.setRunning(ticking);
}

const Shared<ChunkSource>& World::getChunkSource() const {
    return m_chunk_source;
}

void World::onTick() {
    m_chunk_source->onTick();
}

} // voxel