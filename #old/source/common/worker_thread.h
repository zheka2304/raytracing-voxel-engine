#ifndef VOXEL_ENGINE_WORKER_THREAD_H
#define VOXEL_ENGINE_WORKER_THREAD_H

#include <thread>
#include <functional>
#include "queue.h"

class WorkerThread {
private:
    BlockingQueue<std::function<void()>> tasks;
    std::thread thread;
    bool running = true;

    void run();
public:
    WorkerThread();
    ~WorkerThread();
    void queue(std::function<void()> const& task);
};

#endif //VOXEL_ENGINE_WORKER_THREAD_H
