#ifndef VOXEL_ENGINE_QUEUE_H
#define VOXEL_ENGINE_QUEUE_H

#include <deque>
#include <atomic>
#include <mutex>
#include <condition_variable>


namespace voxel {
namespace utils {

template <typename T>
class BlockingQueue {
private:
    std::mutex m_mutex;
    std::condition_variable m_condition;
    std::deque<T> m_queue;

    std::atomic<bool> m_released = false;

public:
    void push(T const& value) {
        std::unique_lock<std::mutex> lock(m_mutex);
        m_queue.push_front(value);
        m_condition.notify_one();
    }

    T pop() {
        std::unique_lock<std::mutex> lock(m_mutex);
        m_condition.wait(lock, [=]{ return !m_queue.empty(); });
        T rc(std::move(m_queue.back()));
        m_queue.pop_back();
        return rc;
    }

    bool try_pop(T& result, bool block) {
        std::unique_lock<std::mutex> lock(m_mutex);
        if (block) {
            m_condition.wait(lock, [=]{ return m_released || !m_queue.empty(); });
            if (m_released) {
                return false;
            }
            result = std::move(m_queue.back());
            m_queue.pop_back();
            return true;
        } else {
            if (m_queue.empty()) {
                return false;
            } else {
                result = std::move(m_queue.back());
                m_queue.pop_back();
                return true;
            }
        }
    }

    void release() {
        m_released = true;
        m_condition.notify_all();
    }

    void clear() {
        std::unique_lock<std::mutex> lock(m_mutex);
        m_queue.clear();
    }
};

} // voxel
} // utils

#endif //VOXEL_ENGINE_QUEUE_H
