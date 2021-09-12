#include "time.h"

#include <chrono>


namespace voxel {
namespace utils {

struct LaunchTimeHolder {
    unsigned long long m_launch_time = getTimestampMillis();
};

static LaunchTimeHolder launch_time_holder;


unsigned long long getTimestampMillis() {
    using namespace std::chrono;
    milliseconds ms = duration_cast< milliseconds >(
            high_resolution_clock::now().time_since_epoch()
    );
    return ms.count();
}

unsigned long long getTimestampNanos() {
    using namespace std::chrono;
    nanoseconds ns = duration_cast< nanoseconds >(
            high_resolution_clock::now().time_since_epoch()
    );
    return ns.count();
}

float getTimeSinceStart() {
    return (getTimestampMillis() - launch_time_holder.m_launch_time) / 1000.0;
}

Stopwatch::Stopwatch() {
    start();
}

void Stopwatch::start() {
    m_end = -1;
    m_start = getTimestampNanos();
}

unsigned long long Stopwatch::stop() {
    m_end = getTimestampNanos();
    return m_end - m_start;
}

} // utils
} // voxel