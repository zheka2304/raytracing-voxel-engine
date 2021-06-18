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
class Wrap {
    T* m_instance = nullptr;
    std::function<T*()> m_factory;
    ThreadGuard m_thread_guard;

public:
    inline Wrap(const std::function<T*()>& factory) : m_factory(factory) {
    }

    inline ~Wrap() {
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
