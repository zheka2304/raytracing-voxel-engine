#ifndef VOXEL_ENGINE_QUEUE_H
#define VOXEL_ENGINE_QUEUE_H

#include <memory>
#include "voxel/common/base.h"


namespace voxel {

// custom double ended queue, based on ring buffer
template<typename T, typename Allocator = std::allocator<T>>
class Queue {
    using alloc_traits = std::allocator_traits<Allocator>;

public:
    using pointer = typename alloc_traits::pointer;
    using reference = decltype(*std::declval<pointer>());
    using size_type = typename alloc_traits::size_type;
    using difference_type = typename alloc_traits::difference_type;

    ~Queue() {
        clear();
    }

    void reserve(size_type new_capacity) {
        if (m_capacity < new_capacity) {
            pointer new_buffer = alloc_traits::allocate(m_allocator, new_capacity);
            if (m_buffer != nullptr) {
                {
                    // move items, marked as + in the each of cases: [ ... S+++++E ... ] or [----E .... S++++]
                    size_type end = m_end < m_start ? m_capacity : m_end;
                    for (size_type index = m_start; index != end; index++) {
                        alloc_traits::construct(m_allocator, new_buffer + index, std::move(m_buffer[index]));
                        alloc_traits::destroy(m_allocator, m_buffer + index);
                    }
                }
                if (m_end < m_start) {
                    // move items, marked as + in the following case: [++++E .... S----]
                    for (size_type index = 0; index != m_end; index++) {
                        alloc_traits::construct(m_allocator, new_buffer + (m_capacity + index) % new_capacity, std::move(m_buffer[index]));
                        alloc_traits::destroy(m_allocator, m_buffer + index);
                    }
                    m_end = (m_capacity + m_end) % new_capacity;
                }
                alloc_traits::deallocate(m_allocator, m_buffer, m_capacity);
            }
            m_buffer = new_buffer;
            m_capacity = new_capacity;
        }
    }

    template<typename... Args>
    reference emplace_back(Args&& ...args) {
        reserve_another_one();
        pointer result = m_buffer + m_end;
        alloc_traits::construct(m_allocator, result, std::forward<Args>(args)...);
        m_end = (m_end + 1) % m_capacity;
        return *result;
    }

    template<typename... Args>
    reference emplace_front(Args&& ...args) {
        reserve_another_one();
        m_start = (m_start + m_capacity - 1) % m_capacity;
        alloc_traits::construct(m_allocator, m_buffer + m_start, std::forward<Args>(args)...);
        return *(m_buffer + m_start);
    }

    pointer peek_back() {
        if (m_start != m_end) {
            return m_buffer + m_end - 1;
        } else {
            return nullptr;
        }
    }

    pointer peek_front() {
        if (m_start != m_end) {
            return m_buffer + m_start;
        } else {
            return nullptr;
        }
    }

    T pop_back() {
        VOXEL_ENGINE_ASSERT(m_start != m_end);
        m_end = (m_end + m_capacity - 1) % m_capacity;
        T result = std::move(m_buffer[m_end]);
        alloc_traits::destroy(m_allocator, m_buffer + m_end);
        return result;
    }

    T pop_front() {
        VOXEL_ENGINE_ASSERT(m_start != m_end);
        T result = std::move(m_buffer[m_start]);
        alloc_traits::destroy(m_allocator, m_buffer + m_start);
        m_start = (m_start + 1) % m_capacity;
        return result;
    }

    void clear() {
        if (m_buffer != nullptr) {
            for (size_type i = m_start; i != m_end; i = (i + 1) % m_capacity) {
                alloc_traits::destroy(m_allocator, m_buffer + i);
            }
            alloc_traits::deallocate(m_allocator, m_buffer, m_capacity);
            m_start = m_end = m_capacity = 0;
            m_buffer = nullptr;
        }
    }

    size_type size() {
        return m_end < m_start ? (m_end + m_capacity) - m_start : m_end - m_start;
    }

    size_type capacity() {
        return m_capacity;
    }

    bool empty() {
        return m_end == m_start;
    }

    pointer data() {
        return m_buffer;
    }

private:
    void reserve_another_one() {
        if (m_capacity == 0 || (m_end + 1) % m_capacity == m_start) {
            reserve(m_capacity > 0 ? m_capacity * 2 : 4);
        }
    }

private:
    pointer m_buffer = nullptr;
    size_type m_capacity = 0;
    size_type m_start = 0;
    size_type m_end = 0;

    typename alloc_traits::allocator_type m_allocator;
};

} // voxel

#endif //VOXEL_ENGINE_QUEUE_H
