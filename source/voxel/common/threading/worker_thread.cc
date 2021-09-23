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

void WorkerThread::queue(std::function<void()> const& task) {
    m_tasks.push(task);
}

WorkerThread::~WorkerThread() {
    m_running = false;
    m_tasks.clear();
    m_tasks.push([]() -> void {});
    m_thread.join();
}

} // threading
} // voxel