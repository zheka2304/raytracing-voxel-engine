#ifndef VOXEL_ENGINE_WORKER_THREAD_H
#define VOXEL_ENGINE_WORKER_THREAD_H

#include <functional>
#include <thread>
#include <optional>
#include "voxel/common/utils/queue.h"


namespace voxel {
namespace utils {

class WorkerThread {
private:
    BlockingQueue<std::function<void()>> m_tasks;
    std::thread m_thread;
    bool m_running = true;

    void run();
public:
    WorkerThread();
    ~WorkerThread();

    void queue(const std::function<void()>& task);
    void await(const std::function<void()>& task);

    template<typename T>
    T awaitResult(const std::function<T()>& task)  {
        std::shared_ptr<std::optional<T>> result = std::make_shared<std::optional<T>>();
        std::shared_ptr<std::condition_variable> cv = std::make_shared<std::condition_variable>();
        queue([=] () -> void {
            *result = task();
            cv->notify_all();
        });

        std::mutex mutex;
        std::unique_lock<std::mutex> lock(mutex);
        cv->wait(lock);
        return result->value();
    }
};

} // utils
} // voxel

#endif //VOXEL_ENGINE_WORKER_THREAD_H
