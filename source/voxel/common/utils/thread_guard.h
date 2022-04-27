#ifndef VOXEL_ENGINE_THREAD_GUARD_H
#define VOXEL_ENGINE_THREAD_GUARD_H

#include <thread>
#include <cassert>


namespace voxel {
namespace utils {


/* A lightweight class, that can be used to assure, that some entity is
 * used and destroyed on the same thread, it was created. It is used mostly for debug
 * purposes and can be fully disabled via defining VOXEL_ENGINE_DISABLE_THREAD_GUARD
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
#ifndef VOXEL_ENGINE_DISABLE_THREAD_GUARD
    std::thread::id m_id;
#endif

public:
    ThreadGuard(const ThreadGuard& other) = default;
    ThreadGuard(ThreadGuard&& other) = default;

    inline void init() {
#ifndef VOXEL_ENGINE_DISABLE_THREAD_GUARD
        m_id = std::this_thread::get_id();
#endif
    }

    inline ThreadGuard() {
#ifndef VOXEL_ENGINE_DISABLE_THREAD_GUARD
        init();
#endif
    }

    inline void guard() {
#ifndef VOXEL_ENGINE_DISABLE_THREAD_GUARD
        assert(std::this_thread::get_id() == m_id);
#endif
    }
};

} // utils
} // voxel

#endif //VOXEL_ENGINE_THREAD_GUARD_H
