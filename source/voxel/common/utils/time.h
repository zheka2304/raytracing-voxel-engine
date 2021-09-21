#ifndef VOXEL_ENGINE_TIME_H
#define VOXEL_ENGINE_TIME_H

#include "voxel/common/base.h"


namespace voxel {
namespace utils {

u64 getTimestampMillis();
u64 getTimestampNanos();
f32 getTimeSinceStart();

class Stopwatch {
    u64 m_start;
    u64 m_end;
public:
    Stopwatch();
    Stopwatch(const Stopwatch&) = default;
    Stopwatch(Stopwatch&&) = default;

    void start();
    u64 stop();
};

} // utils
} // voxel

#endif //VOXEL_ENGINE_TIME_H
