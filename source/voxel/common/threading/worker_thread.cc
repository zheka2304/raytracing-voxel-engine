#include "worker_thread.h"


namespace voxel {
namespace threading {

WorkerThread::WorkerThread() : m_thread(std::thread(&WorkerThread::run, this)) {
}

void WorkerThread::run() {
    while (m_running) {
        m_tasks.pop()();
    }
}

void WorkerThread::queue(const std::function<void()>& task, bool immediate) {
    immediate ? m_tasks.shift(task) : m_tasks.push(task);
}

i32 WorkerThread::getProcessingThreadCount() {
    return 1;
}

WorkerThread::~WorkerThread() {
    m_running = false;
    m_tasks.clear();
    m_tasks.push([]() -> void {});
    m_thread.join();
}

} // threading
} // voxel