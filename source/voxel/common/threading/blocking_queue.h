#ifndef VOXEL_ENGINE_BLOCKING_QUEUE_H
#define VOXEL_ENGINE_BLOCKING_QUEUE_H

#include <deque>
#include <atomic>
#include <mutex>
#include <optional>
#include <condition_variable>


namespace voxel {
namespace threading {

template<typename T>
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
        m_condition.wait(lock, [=] { return !m_queue.empty(); });
        T rc(std::move(m_queue.back()));
        m_queue.pop_back();
        return rc;
    }

    std::optional<T> tryPop(bool block = false) {
        std::unique_lock<std::mutex> lock(m_mutex);
        if (block) {
            m_condition.wait(lock, [=] { return m_released || !m_queue.empty(); });
            if (m_released) {
                return std::optional<T>();
            }
            std::optional<T> result = std::move(m_queue.back());
            m_queue.pop_back();
            return result;
        } else {
            if (m_queue.empty()) {
                return std::optional<T>();
            } else {
                std::optional<T> result = std::move(m_queue.back());
                m_queue.pop_back();
                return result;
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

    std::deque<T>& getDeque() {
        return m_queue;
    }

    std::mutex& getMutex() {
        return m_mutex;
    }
};

} // threading
} // voxel

#endif //VOXEL_ENGINE_BLOCKING_QUEUE_H
