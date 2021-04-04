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
    std::mutex              d_mutex;
    std::condition_variable d_condition;
    std::deque<T>           d_queue;

    std::atomic<bool> released = false;
public:
    void push(T const& value) {
        {
            std::unique_lock<std::mutex> lock(this->d_mutex);
            d_queue.push_front(value);
        }
        this->d_condition.notify_one();
    }

    T pop() {
        std::unique_lock<std::mutex> lock(this->d_mutex);
        this->d_condition.wait(lock, [=]{ return !this->d_queue.empty(); });
        T rc(std::move(this->d_queue.back()));
        this->d_queue.pop_back();
        return rc;
    }

    bool try_pop(T& result, bool block) {
        std::unique_lock<std::mutex> lock(this->d_mutex);
        if (block) {
            this->d_condition.wait(lock, [=]{ return released || !this->d_queue.empty(); });
            if (released) {
                return false;
            }
            result = std::move(this->d_queue.back());
            this->d_queue.pop_back();
            return true;
        } else {
            if (this->d_queue.empty()) {
                return false;
            } else {
                result = std::move(this->d_queue.back());
                this->d_queue.pop_back();
                return true;
            }
        }
    }

    void release() {
        this->released = true;
        this->d_condition.notify_all();
    }

    void clear() {
        std::unique_lock<std::mutex> lock(this->d_mutex);
        this->d_queue.clear();
    }
};

#endif //VOXEL_ENGINE_QUEUE_H
