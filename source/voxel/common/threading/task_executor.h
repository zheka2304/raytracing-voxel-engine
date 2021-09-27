#ifndef VOXEL_ENGINE_TASK_EXECUTOR_H
#define VOXEL_ENGINE_TASK_EXECUTOR_H

#include <functional>
#include <mutex>
#include <condition_variable>

#include "voxel/common/threading/blocking_queue.h"
#include "voxel/common/base.h"


namespace voxel {
namespace threading {

class TaskExecutor {
public:
    template<typename T>
    using Task = std::function<T()>;

protected:
    BlockingQueue<std::function<void()>> m_tasks;

public:
    TaskExecutor();
    TaskExecutor(const TaskExecutor&) = delete;
    TaskExecutor(TaskExecutor&&) = delete;
    virtual ~TaskExecutor();

    virtual void queue(const Task<void>& task, bool immediate = false) = 0;

    void await(const Task<void>& task);

    template<typename T>
    T awaitResult(const Task<T>& task) {
        Shared<std::optional<T>> result = CreateShared<std::optional<T>>();
        Shared<std::condition_variable> cv = CreateShared<std::condition_variable>();
        queue([=]() -> void {
            *result = task();
            cv->notify_all();
        });

        std::mutex mutex;
        std::unique_lock<std::mutex> lock(mutex);
        cv->wait(lock);
        return result->value();
    }

    BlockingQueue<std::function<void()>>& getQueue();
    void clearQueue();

    virtual i32 getProcessingThreadCount();
    i32 getEstimatedRemainingTasks();
};

} // threading
} // voxel

#endif //VOXEL_ENGINE_TASK_EXECUTOR_H
