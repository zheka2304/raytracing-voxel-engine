#include "thread_pool.h"


namespace voxel {
namespace threading {

ThreadPool::ThreadPool(i32 thread_count) {
    for (i32 i = 0; i < thread_count; i++) {
        m_threads.emplace_back(&ThreadPool::run, this);
    }
}

ThreadPool::~ThreadPool() {
    m_running = false;
    m_tasks.clear();
    for (auto& thread : m_threads) {
        m_tasks.push([]() -> void {});
    }
    for (auto& thread : m_threads) {
        thread.join();
    }
}

void ThreadPool::run() {
    while (m_running) {
        m_tasks.pop()();
    }
}

void ThreadPool::queue(const Task<void>& task, bool immediate) {
    immediate ? m_tasks.shift(task) : m_tasks.push(task);
}

i32 ThreadPool::getProcessingThreadCount() {
    return m_threads.size();
}

} // threading
} // voxel