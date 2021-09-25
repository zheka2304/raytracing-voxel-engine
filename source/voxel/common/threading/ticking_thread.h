#ifndef VOXEL_ENGINE_TICKING_THREAD_H
#define VOXEL_ENGINE_TICKING_THREAD_H

#include <thread>
#include <functional>
#include <atomic>
#include <mutex>
#include <condition_variable>
#include "voxel/common/base.h"


namespace voxel {
namespace threading {

class TickingThread {
public:
    // (time since start without last tick, current tick time) -> sleep time
    using Scheduler = std::function<i32(i32, i32)>;

    // (frame counter) -> void
    using TickCallback = std::function<void(i32)>;

private:
    Scheduler m_scheduler;
    TickCallback m_tick_callback;

    std::thread m_thread;
    std::mutex m_mutex;
    std::condition_variable m_condition;
    std::atomic<bool> m_joining = false;
    std::atomic<bool> m_running = false;

public:
    TickingThread(Scheduler scheduler, TickCallback tick_callback);
    TickingThread(const TickingThread&) = delete;
    TickingThread(TickingThread&&) = delete;
    ~TickingThread();

    void setRunning(bool running);
    void join();

    static Scheduler TicksPerSecond(i32 tps);

private:
    void run();
};

} // threading
} // voxel

#endif //VOXEL_ENGINE_TICKING_THREAD_H
