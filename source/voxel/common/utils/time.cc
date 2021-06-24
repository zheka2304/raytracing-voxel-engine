#include "time.h"

#include <chrono>


namespace voxel {
namespace utils {

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