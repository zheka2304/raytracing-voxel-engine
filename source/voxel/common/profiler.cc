#include "profiler.h"

#include <sstream>
#include <iostream>
#include <glad/glad.h>


namespace voxel {

Profiler::ScopeInfo::ScopeInfo(std::thread::id thread_id, const std::string& name) : m_thread_id(thread_id), m_name(name) {
}

void Profiler::ScopeInfo::addProfileInfo(i64 start, i64 end) {
    i64 time_i = end - start;
    m_current_sum += time_i;
    m_current_count++;
    if (m_current_count >= FRAMES_PER_MEASURE || m_stored_value < 0) {
        f32 value = m_current_sum / m_current_count * 0.000001f;
        if (m_stored_value >= 0) m_stored_value = m_stored_value * (1.0 - ACCUMULATION_FACTOR) + value * ACCUMULATION_FACTOR;
        else m_stored_value = value;
        m_current_sum = 0;
        m_current_count = 0;
    }
}

f32 Profiler::ScopeInfo::getValue() const {
    return std::max(m_stored_value, 0.0f);
}


Profiler::Timer::Timer(ScopeInfo* scope_info, bool gpu_scope) :
    m_scope_info(scope_info), m_gpu_scope(gpu_scope) {
    if (m_gpu_scope) {
        glFinish();
    }
    m_start = std::chrono::steady_clock::now();
}

void Profiler::Timer::stop() {
    if (!m_stopped) {
        if (m_gpu_scope) {
            glFinish();
        }
        auto end = std::chrono::steady_clock::now();
        i64 start_i = std::chrono::duration_cast<std::chrono::microseconds>(m_start.time_since_epoch()).count();
        i64 end_i = std::chrono::duration_cast<std::chrono::microseconds>(end.time_since_epoch()).count();
        if (m_scope_info) {
            m_scope_info->addProfileInfo(start_i, end_i);
        }
        m_stopped = true;
    }
}

Profiler::Timer::~Timer() {
    stop();
}

Profiler::ScopeInfo& Profiler::getScopeInfo(std::thread::id thread_id, const std::string& name) {
    std::unique_lock<std::mutex> lock(m_mutex);
    std::pair<std::thread::id, std::string> key(thread_id, name);
    auto found = m_scope_info_map.find(key);
    if (found != m_scope_info_map.end()) {
        return found->second;
    } else {
        return m_scope_info_map.emplace(key, ScopeInfo(thread_id, name)).first->second;
    }
}

Profiler::ScopeInfo& Profiler::getScopeInfo(const std::string& name) {
    return getScopeInfo(std::this_thread::get_id(), name);
}

std::string Profiler::getStats(const std::string& name) {
    std::unique_lock<std::mutex> lock(m_mutex);
    std::stringstream ss;
    for (auto& pair : m_scope_info_map) {
        if (pair.first.second == name) {
            ss << name << " " << i32(pair.second.getValue() * 100000.0f) / 100.0f << "\n";
        }
    }
    return ss.str();
}

f32 Profiler::getAverageValue(const std::string& name) {
    std::unique_lock<std::mutex> lock(m_mutex);
    f32 value = 0;
    i32 count = 0;
    for (auto& pair : m_scope_info_map) {
        if (pair.first.second == name) {
            value += pair.second.getValue();
            count++;
        }
    }
    return count > 0 ? i32(value / count * 100000.0f) / 100.0f : 0;
}

} // voxel