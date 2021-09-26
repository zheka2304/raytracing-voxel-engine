#include "world.h"

namespace voxel {

World::World(const threading::TickingThread::Scheduler& tick_scheduler,
             Shared<ChunkProvider> chunk_provider,
             Shared<ChunkStorage> chunk_storage,
             Shared<threading::TaskExecutor> background_executor) :

             m_chunk_source(CreateShared<ChunkSource>(chunk_provider, chunk_storage, background_executor)),
             m_ticking_thread(tick_scheduler, std::bind(&World::onTick, this)),
             m_background_executor(background_executor) {
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

const Shared<threading::TaskExecutor>& World::getBackgroundExecutor() const {
    return m_background_executor;
}

void World::onTick() {
    m_chunk_source->onTick();
}

} // voxel