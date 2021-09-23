#ifndef VOXEL_ENGINE_WORKER_THREAD_H
#define VOXEL_ENGINE_WORKER_THREAD_H

#include <functional>
#include <thread>
#include <optional>

#include "voxel/common/base.h"
#include "voxel/common/threading/task_executor.h"


namespace voxel {
namespace threading {

class WorkerThread : public TaskExecutor {
private:
    std::thread m_thread;
    bool m_running = true;

    void run();

public:
    WorkerThread();

    ~WorkerThread() override;

    void queue(const std::function<void()>& task) override;
};

} // threading
} // voxel

#endif //VOXEL_ENGINE_WORKER_THREAD_H
