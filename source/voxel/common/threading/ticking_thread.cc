#include "ticking_thread.h"

#include "voxel/common/utils/time.h"


namespace voxel {
namespace threading {

TickingThread::Scheduler TickingThread::TicksPerSecond(i32 tps) {
    // TODO: this is pretty bad implementation
    return [=] (i32, i32 current_tick_time) -> i32 {
        return  (1000 / tps) - current_tick_time;
    };
}

TickingThread::TickingThread(Scheduler scheduler, TickCallback tick_callback) :
    m_scheduler(scheduler), m_tick_callback(tick_callback), m_thread(&TickingThread::run, this) {
}

TickingThread::~TickingThread() {
    join();
}

void TickingThread::setRunning(bool running) {
    if (m_running != running) {
        m_running = running;
        if (m_running) {
            std::unique_lock<std::mutex> lock(m_mutex);
            m_condition.notify_one();
        }
    }
}

void TickingThread::join() {
    if (!m_joining) {
        m_joining = true;
        m_thread.join();
    }
}

void TickingThread::run() {
    i32 tick_counter = 0;
    u64 start_time = utils::getTimeSinceStart();
    while (!m_joining) {
        u64 wait_start_time = utils::getTimeSinceStart();
        {
            std::unique_lock<std::mutex> lock(m_mutex);
            m_condition.wait(lock, [=] { return bool(m_running); });
        }

        // do not count time, spent for waiting
        u64 tick_start_time = utils::getTimeSinceStart();
        start_time += tick_start_time - wait_start_time;

        m_tick_callback(tick_counter++);
        u64 tick_end_time = utils::getTimeSinceStart();
        i32 delay = m_scheduler(i32(tick_start_time - start_time), i32(tick_end_time - tick_start_time));
        if (delay > 0) {
            std::this_thread::sleep_for(std::chrono::milliseconds(delay));
        }
    }
}

} // threading
} // voxel