#ifndef VOXEL_ENGINE_THREAD_POOL_H
#define VOXEL_ENGINE_THREAD_POOL_H

#include <vector>
#include <thread>
#include "voxel/common/base.h"
#include "voxel/common/threading/task_executor.h"


namespace voxel {
namespace threading {

class ThreadPool : public TaskExecutor {
    std::vector<std::thread> m_threads;
    bool m_running = true;

    void run();
public:
    ThreadPool(i32 thread_count);
    ~ThreadPool() override;

    void queue(const Task<void> &task, bool immediate = false) override;
    i32 getProcessingThreadCount() override;
};

} // threading
} // voxel

#endif //VOXEL_ENGINE_THREAD_POOL_H
