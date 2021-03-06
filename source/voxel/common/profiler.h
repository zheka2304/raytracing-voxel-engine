#ifndef VOXEL_ENGINE_PROFILER_H
#define VOXEL_ENGINE_PROFILER_H

#include <string>
#include <vector>
#include <unordered_map>
#include <mutex>
#include <thread>
#include <chrono>
#include "voxel/common/base.h"


namespace voxel {

class Profiler {
public:
    constexpr static const i32 FRAMES_PER_MEASURE = 4;
    constexpr static const f32 ACCUMULATION_FACTOR = 0.4f;

    static Profiler& get();
    static std::string getAppProfilerStats();
    static std::string getWindowTitlePerformanceStats();

    class ScopeInfo {
        std::string m_name;
        std::thread::id m_thread_id;

        f32 m_stored_value = -1;
        f32 m_current_sum = 0;
        i32 m_current_count = 0;

    public:
        ScopeInfo(std::thread::id thread_id, const std::string& name);
        void addProfileInfo(i64 start, i64 end);
        f32 getValue() const;
    };

    class Timer {
        bool m_stopped = false;
        bool m_gpu_scope;
        ScopeInfo* m_scope_info;
        std::chrono::time_point<std::chrono::steady_clock> m_start;

    public:
        Timer(ScopeInfo* scope_info, bool gpu_scope = false);
        void stop();
        ~Timer();
    };

private:
    std::mutex m_mutex;
    std::unordered_map<std::pair<std::thread::id, std::string>, ScopeInfo, PairHash> m_scope_info_map;

public:
    ScopeInfo& getScopeInfo(std::thread::id thread_id, const std::string& name);
    ScopeInfo& getScopeInfo(const std::string& name);

    std::string getStats(const std::string& name);
    f32 getAverageValue(const std::string& name);
};

} // voxel


#if VOXEL_ENGINE_ENABLE_PROFILER

#define VOXEL_ENGINE_PROFILE_SCOPE0(name, gpu_scope) \
    static voxel::Profiler::ScopeInfo* profiler_scope_info_ ## name = nullptr; \
    if (profiler_scope_info_ ## name == nullptr) profiler_scope_info_ ## name = &voxel::Profiler::get().getScopeInfo(#name); \
    voxel::Profiler::Timer profiler_timer_ ## name(profiler_scope_info_ ## name, gpu_scope);

#define VOXEL_ENGINE_PROFILE_SCOPE(name) VOXEL_ENGINE_PROFILE_SCOPE0(name, false)

#define VOXEL_ENGINE_PROFILE_SCOPE_END(name) (profiler_timer_ ## name).stop();

#define VOXEL_ENGINE_PROFILE_GPU_SCOPE(name) VOXEL_ENGINE_PROFILE_SCOPE0(name, true);

#define VOXEL_ENGINE_PROFILE_GPU_SCOPE_END(name) VOXEL_ENGINE_PROFILE_GPU_SCOPE(name)

#else

#define VOXEL_ENGINE_PROFILE_SCOPE(name)
#define VOXEL_ENGINE_PROFILE_SCOPE_END(name)
#define VOXEL_ENGINE_PROFILE_GPU_SCOPE(name)
#define VOXEL_ENGINE_PROFILE_GPU_SCOPE_END(name)

#endif // VOXEL_ENGINE_PROFILER_ENABLE

#endif //VOXEL_ENGINE_PROFILER_H
