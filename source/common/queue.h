#ifndef VOXEL_ENGINE_QUEUE_H
#define VOXEL_ENGINE_QUEUE_H
#include <mutex>
#include <atomic>
#include <condition_variable>
#include <deque>

template <typename T>
class BlockingQueue
{
private:
    std::mutex              mutex;
    std::condition_variable condition;
    std::deque<T>           queue;

    std::atomic<bool> released = false;
public:
    void push(T const& value) {
        std::unique_lock<std::mutex> lock(this->mutex);
        queue.push_front(value);
        this->condition.notify_one();
    }

    T pop() {
        std::unique_lock<std::mutex> lock(this->mutex);
        this->d_condition.wait(lock, [=]{ return !this->queue.empty(); });
        T rc(std::move(this->queue.back()));
        this->queue.pop_back();
        return rc;
    }

    bool try_pop(T& result, bool block) {
        std::unique_lock<std::mutex> lock(this->mutex);
        if (block) {
            this->condition.wait(lock, [=]{ return this->released || !this->queue.empty(); });
            if (this->released) {
                return false;
            }
            result = std::move(this->queue.back());
            this->queue.pop_back();
            return true;
        } else {
            if (this->queue.empty()) {
                return false;
            } else {
                result = std::move(this->queue.back());
                this->queue.pop_back();
                return true;
            }
        }
    }

    void release() {
        this->released = true;
        this->condition.notify_all();
    }

    void clear() {
        std::unique_lock<std::mutex> lock(this->mutex);
        this->queue.clear();
    }
};

#endif //VOXEL_ENGINE_QUEUE_H
