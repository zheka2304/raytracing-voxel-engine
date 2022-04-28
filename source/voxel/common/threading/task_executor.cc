#include "task_executor.h"


namespace voxel {
namespace threading {

TaskExecutor::TaskExecutor() {
}

TaskExecutor::~TaskExecutor() {
}

void TaskExecutor::await(const Task<void>& task) {
    std::shared_ptr<std::condition_variable> cv = std::make_shared<std::condition_variable>();
    queue([=]() -> void {
        task();
        cv->notify_all();
    });

    std::mutex mutex;
    std::unique_lock<std::mutex> lock(mutex);
    cv->wait(lock);
}

BlockingQueue<std::function<void()>>& TaskExecutor::getQueue() {
    return m_tasks;
}

void TaskExecutor::clearQueue() {
    m_tasks.clear();
}

i32 TaskExecutor::getProcessingThreadCount() {
    return 0;
}

i32 TaskExecutor::getEstimatedRemainingTasks() {
    return m_tasks.getUnderlyingQueue().size();
}

} // threading
} // voxel