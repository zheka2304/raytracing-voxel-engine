#ifndef VOXEL_ENGINE_THREAD_POOL_H
#define VOXEL_ENGINE_THREAD_POOL_H

#include <vector>
#include <thread>
#include "voxel/common/base.h"
#include "voxel/common/threading/task_executor.h"


namespace voxel {
namespace threading {

template<typename T>
class ThreadPoolExecutor {
public:
    using Supplier = std::function<T()>;
    using Consumer = std::function<void(const T&)>;

private:
    Supplier m_supplier;
    Consumer m_consumer;

    std::vector<std::thread> m_threads;
    bool m_running = true;

    void run() {
        while (m_running) {
            m_consumer(m_supplier());
        }
    }

public:
    ThreadPoolExecutor(Supplier supplier, Consumer consumer, i32 thread_count) : m_supplier(supplier), m_consumer(consumer) {
        for (i32 i = 0; i < thread_count; i++) {
            m_threads.emplace_back(&ThreadPoolExecutor<T>::run, this);
        }
    }

    ~ThreadPoolExecutor() {
        m_running = false;
        for (auto& thread : m_threads) {
            thread.join();
        }
    }
};

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
