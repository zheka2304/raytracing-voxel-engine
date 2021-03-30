
#ifndef VOXEL_ENGINE_SIMPLE_PROFILER_H
#define VOXEL_ENGINE_SIMPLE_PROFILER_H

#include <chrono>
#include <iostream>
#include <atomic>

namespace Profiler {
    inline long long nanos() {
        using namespace std::chrono;
        nanoseconds ns = duration_cast<nanoseconds>(
                system_clock::now().time_since_epoch()
        );
        return ns.count();
    }

    class ProfilerSection {
    public:
        const char* name;
        long long start;
        bool ended = false;

        inline explicit ProfilerSection(const char* name) : name(name) {
            start = nanos();
        }

        inline void endSection() {
            if (!ended) {
                long long time = nanos() - start;
                std::cout << name << ": " << float(time / 1000000.0) << "ms\n";
                ended = true;
            }
        }

        inline ~ProfilerSection() {
            endSection();
        }
    };
}


//#define ENABLE_PROFILER

#ifdef ENABLE_PROFILER
#define PROFILER_BEGIN(NAME) Profiler::ProfilerSection NAME(# NAME);
#define PROFILER_END(NAME) NAME.endSection();
#else
#define PROFILER_BEGIN(NAME)
#define PROFILER_END(NAME)
#endif

#endif //VOXEL_ENGINE_SIMPLE_PROFILER_H
