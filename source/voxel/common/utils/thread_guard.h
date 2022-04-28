#ifndef VOXEL_ENGINE_THREAD_GUARD_H
#define VOXEL_ENGINE_THREAD_GUARD_H

#include <thread>
#include <cassert>


namespace voxel {
namespace utils {


/* A lightweight class, that can be used to assure, that some entity is
 * used and destroyed on the same thread, it was created. It is used mostly for debug
 * purposes and can be fully disabled via setting VOXEL_ENGINE_ENABLE_THREAD_GUARD to 0
 *
 * class Sample {
 *     utils::ThreadGuard m_thread_guard;
 *
 * public:
 *     void doSomething() {
 *         m_thread_guard.guard();
 *         // do thread safe work
 *     }
 * }
 */
class ThreadGuard {
#if VOXEL_ENGINE_ENABLE_THREAD_GUARD
    std::thread::id m_id;
#endif

public:
    ThreadGuard(const ThreadGuard& other) = default;
    ThreadGuard(ThreadGuard&& other) = default;

    inline void init() {
#if VOXEL_ENGINE_ENABLE_THREAD_GUARD
        m_id = std::this_thread::get_id();
#endif
    }

    inline ThreadGuard() {
#if VOXEL_ENGINE_ENABLE_THREAD_GUARD
        init();
#endif
    }

    inline void guard() {
#if VOXEL_ENGINE_ENABLE_THREAD_GUARD
        // do not use VOXEL_ENGINE_ASSERT, so trigger, even if it is disabled
        assert(std::this_thread::get_id() == m_id);
#endif
    }
};

} // utils
} // voxel

#endif //VOXEL_ENGINE_THREAD_GUARD_H
