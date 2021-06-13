#include "worker_thread.h"


WorkerThread::WorkerThread() : thread(std::thread(&WorkerThread::run, this)) {

}

void WorkerThread::run() {
    while (running) {
        tasks.pop()();
    }
}

void WorkerThread::queue(std::function<void()> const& task) {
    tasks.push(task);
}

WorkerThread::~WorkerThread() {
    running = false;
    tasks.clear();
    queue([] () -> void {});
    thread.join();
}