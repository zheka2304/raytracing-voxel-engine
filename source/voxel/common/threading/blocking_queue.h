#ifndef VOXEL_ENGINE_BLOCKING_QUEUE_H
#define VOXEL_ENGINE_BLOCKING_QUEUE_H

#include <queue>
#include <atomic>
#include <mutex>
#include <optional>
#include <condition_variable>
#include "voxel/common/base.h"
#include "voxel/common/utils/queue.h"


namespace voxel {
namespace threading {

template<typename T>
class BlockingQueue {
private:
    std::mutex m_mutex;
    std::condition_variable m_condition;
    Queue<T> m_queue;

    std::atomic<bool> m_released = false;

public:
    void push(const T& value) {
        std::unique_lock<std::mutex> lock(m_mutex);
        m_queue.emplace_back(value);
        m_condition.notify_one();
    }

    void shift(const T& value) {
        std::unique_lock<std::mutex> lock(m_mutex);
        m_queue.emplace_front(value);
        m_condition.notify_one();
    }

    T pop() {
        std::unique_lock<std::mutex> lock(m_mutex);
        m_condition.wait(lock, [=] { return !m_queue.empty(); });
        T result(std::move(m_queue.pop_front()));
        return result;
    }

    std::optional<T> tryPop(bool block = false) {
        std::unique_lock<std::mutex> lock(m_mutex);
        if (block) {
            m_condition.wait(lock, [=] { return m_released || !m_queue.empty(); });
            if (m_released) {
                return std::optional<T>();
            }
            std::optional<T> result(std::move(m_queue.pop_front()));
            return result;
        } else {
            if (m_queue.empty()) {
                return std::optional<T>();
            } else {
                std::optional<T> result(std::move(m_queue.pop_front()));
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

    i32 getSize() {
        std::unique_lock<std::mutex> lock(m_mutex);
        return m_queue.size();
    }

    Queue<T>& getUnderlyingQueue() {
        return m_queue;
    }

    std::mutex& getMutex() {
        return m_mutex;
    }
};

template<typename T, std::size_t MaxSize = 0>
class PriorityQueue : protected std::priority_queue<T> {
    using BaseType = std::priority_queue<T>;
    std::condition_variable m_condition;
    std::mutex m_mutex;

public:
    PriorityQueue() {
        if (MaxSize > 0) {
            BaseType::c.reserve(MaxSize);
        }
    }

    void push(const T& value) {
        std::unique_lock<std::mutex> lock(m_mutex);
        BaseType::push(value);
        if constexpr(MaxSize > 0) {
            if (BaseType::size() > MaxSize) {
                BaseType::c.pop_back();
            }
        }
        m_condition.notify_one();
    }

    T pop() {
        std::unique_lock<std::mutex> lock(m_mutex);
        m_condition.wait(lock, [=] { return !BaseType::empty(); });
        T result(std::move(BaseType::top()));
        BaseType::pop();
        return result;
    }
};


template<typename T>
class UniqueBlockingQueue {
    Queue<T> m_queue;
    flat_hash_set<T> m_item_set;

    std::mutex m_mutex;
    std::condition_variable m_condition;
    std::atomic<bool> m_released = false;

public:
    void push(const T& value) {
        std::unique_lock<std::mutex> lock(m_mutex);
        if (m_item_set.find(value) == m_item_set.end()) {
            m_queue.emplace_back(value);
            m_item_set.insert(value);
            m_condition.notify_one();
        }
    }

    void shift(const T& value) {
        std::unique_lock<std::mutex> lock(m_mutex);
        if (m_item_set.find(value) == m_item_set.end()) {
            m_queue.emplace_front(value);
            m_item_set.insert(value);
            m_condition.notify_one();
        }
    }

    T pop() {
        std::unique_lock<std::mutex> lock(m_mutex);
        m_condition.wait(lock, [=] { return !m_queue.empty(); });
        T result(std::move(m_queue.pop_front()));
        m_item_set.erase(result);
        return result;
    }

    std::optional<T> tryPop(bool block = false) {
        std::unique_lock<std::mutex> lock(m_mutex);
        if (block) {
            m_condition.wait(lock, [=] { return m_released || !m_queue.empty(); });
            if (m_released) {
                return std::optional<T>();
            }
            std::optional<T> result(std::move(m_queue.pop_front()));
            m_item_set.erase(result.value());
            return result;
        } else {
            if (m_queue.empty()) {
                return std::optional<T>();
            } else {
                std::optional<T> result(std::move(m_queue.pop_front()));
                m_item_set.erase(result.value());
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
        m_item_set.clear();
    }

    Queue<T>& getUnderlyingQueue() {
        return m_queue;
    }

    std::mutex& getMutex() {
        return m_mutex;
    }
};

} // threading
} // voxel

#endif //VOXEL_ENGINE_BLOCKING_QUEUE_H
