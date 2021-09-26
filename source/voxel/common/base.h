#ifndef VOXEL_ENGINE_BASE_H
#define VOXEL_ENGINE_BASE_H

#include <cstdint>
#include <memory>


using f32 = float;
using f64 = double;
using f128 = long double;
using i8 = int8_t;
using u8 = uint8_t;
using i16 = int16_t;
using u16 = uint16_t;
using i32 = int32_t;
using u32 = uint32_t;
using i64 = int64_t;
using u64 = uint64_t;
using byte = u8;


namespace voxel {

// shorter names for smart pointers

template<typename T>
using Shared = std::shared_ptr<T>;
template<typename T, typename ... Args>
constexpr Shared<T> CreateShared(Args&& ... args) {
    return std::make_shared<T>(std::forward<Args>(args)...);
}

template<typename T>
using Unique = std::unique_ptr<T>;
template<typename T, typename ... Args>
constexpr Unique<T> CreateUnique(Args&& ... args) {
    return std::make_unique<T>(std::forward<Args>(args)...);
}

template<typename T>
using Weak = std::weak_ptr<T>;

} // voxel

#endif //VOXEL_ENGINE_BASE_H