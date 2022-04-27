#ifndef VOXEL_ENGINE_BASE_H
#define VOXEL_ENGINE_BASE_H

#include <cstdint>
#include <memory>
#include <functional>
#include <optional>
#include <phmap/phmap.h>


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

// hash for pairs as unordered map keys
struct PairHash {
public:
    template <typename T, typename U>
    std::size_t operator()(const std::pair<T, U>& x) const {
        return std::hash<T>()(x.first) ^ std::hash<U>()(x.second);
    }
};


// parallel hashmap and hasset aliases

template<typename... T>
using flat_hash_map = phmap::flat_hash_map<T...>;

template<typename... T>
using flat_hash_set = phmap::flat_hash_set<T...>;

template<typename... T>
using parallel_flat_hash_map = phmap::parallel_flat_hash_map<T...>;

template<typename... T>
using parallel_flat_hash_set = phmap::parallel_flat_hash_set<T...>;

} // voxel

#endif //VOXEL_ENGINE_BASE_H
