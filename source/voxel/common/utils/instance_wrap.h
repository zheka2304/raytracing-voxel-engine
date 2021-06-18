#ifndef VOXEL_ENGINE_INSTANCE_WRAP_H
#define VOXEL_ENGINE_INSTANCE_WRAP_H

#include <functional>
#include "voxel/common/utils/thread_guard.h"


namespace voxel {
namespace utils {

/*
 * Instance wrap with on-demand instantiation and optional thread guard
 */
template<typename T, bool with_thread_guard = false>
class InstanceWrap {
    T* m_instance = nullptr;
    std::function<T*()> m_factory;
    ThreadGuard m_thread_guard;

public:
    // default initialization constructor takes factory to create instance on demand
    explicit inline InstanceWrap(const std::function<T*()>& factory) : m_factory(factory) {
    }

    // copy constructor requires copy constructor for type T, it copies T, if it was created and copies factory and thread guard
    inline InstanceWrap(const InstanceWrap& other) {
        m_instance = other.m_instance != nullptr ? new T(*other.m_instance) : nullptr;
        m_factory = other.m_factory;
        m_thread_guard = other.m_thread_guard;
    }

    // move constructor just moves pointer from other wrap and copies factory & thread guard
    inline InstanceWrap(InstanceWrap&& other) {
        m_instance = other.m_instance;
        m_factory = other.m_factory;
        m_thread_guard = other.m_thread_guard;
        other.m_instance = nullptr;
    }

    inline ~InstanceWrap() {
        delete(m_instance);
    }

    inline T& get() {
        if (m_instance == nullptr) {
            m_instance = m_factory();
            if (with_thread_guard) {
                m_thread_guard.init();
            }
        }
        if (with_thread_guard) {
            m_thread_guard.guard();
        }
        return m_instance;
    }

    inline T* operator->() {
        return *get();
    }

    inline T& operator*() {
        return get();
    }


};

} // utils
} // voxel

#endif //VOXEL_ENGINE_INSTANCE_WRAP_H
