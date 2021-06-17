#include "worker_thread.h"


namespace voxel {
namespace utils {

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

void WorkerThread::await(const std::function<void()>& task) {
    std::shared_ptr<std::condition_variable> cv = std::make_shared<std::condition_variable>();
    queue([=] () -> void {
        task();
        cv->notify_all();
    });

    std::mutex mutex;
    std::unique_lock<std::mutex> lock(mutex);
    cv->wait(lock);
}


WorkerThread::~WorkerThread() {
    m_running = false;
    m_tasks.clear();
    queue([] () -> void {});
    m_thread.join();
}

} // utils
} // voxel