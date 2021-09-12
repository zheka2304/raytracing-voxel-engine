#ifndef VOXEL_ENGINE_TIME_H
#define VOXEL_ENGINE_TIME_H

namespace voxel {
namespace utils {

unsigned long long getTimestampMillis();
unsigned long long getTimestampNanos();
float getTimeSinceStart();

class Stopwatch {
    unsigned long long m_start;
    unsigned long long m_end;
public:
    Stopwatch();
    Stopwatch(const Stopwatch&) = default;
    Stopwatch(Stopwatch&&) = default;

    void start();
    unsigned long long stop();
};

} // utils
} // voxel

#endif //VOXEL_ENGINE_TIME_H
