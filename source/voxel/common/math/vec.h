

#ifndef VOXEL_ENGINE_VEC_H
#define VOXEL_ENGINE_VEC_H

#include <math.h>


namespace voxel {
namespace math {

typedef float real_t;
typedef int integer_t;


template<typename T, int N>
class VectorBaseClass;

template<typename T>
class VectorBaseClass<T, 2> {
public:
    union {
        T data[2];
        struct {
            T x, y;
        };
    };

    inline VectorBaseClass(T x, T y) : x(x), y(y) { }
    inline VectorBaseClass(T v) : x(v), y(v) { }
    inline VectorBaseClass() : x(0), y(0) { }
};

template<typename T>
class VectorBaseClass<T, 3> {
public:
    union {
        T data[3];
        struct {
            T x, y, z;
        };
    };

    inline VectorBaseClass(T x, T y, T z) : x(x), y(y), z(z) { }
    inline VectorBaseClass(T v) : x(v), y(v), z(v) { }
    inline VectorBaseClass() : x(0), y(0), z(0) { }
};

template<typename T>
class VectorBaseClass<T, 4> {
public:
    union {
        T data[4];
        struct {
            T x, y, z, w;
        };
    };

    inline VectorBaseClass(T x, T y, T z, T w) : x(x), y(y), z(z), w(w) { }
    inline VectorBaseClass(T v) : x(v), y(v), z(v), w(v) { }
    inline VectorBaseClass() : x(0), y(0), z(0), w(0) { }
};



template<typename T, int N>
class VectorTemplate : public VectorBaseClass<T, N> {
public:
    using VectorBaseClass<T, N>::VectorBaseClass;

    inline VectorTemplate(T* data) {
        for (int i = 0; i < N; i++) {
            this->data[i] = data[i];
        }
    }

    template<typename T2>
    inline auto operator+(const VectorTemplate<T2, N>& other) const {
        typedef decltype(static_cast<T>(1) + static_cast<T2>(1)) result_type;
        VectorTemplate<result_type, N> output;
        for (int i = 0; i < N; i++) {
            output.data[i] = this->data[i] + other.data[i];
        }
        return output;
    }

    template<typename T2>
    inline const VectorTemplate<T, N>& operator+=(const VectorTemplate<T2, N>& other) {
        for (int i = 0; i < N; i++) {
            this->data[i] += other.data[i];
        }
        return *this;
    }

    template<typename T2>
    inline auto operator-(const VectorTemplate<T2, N>& other) const {
        typedef decltype(static_cast<T>(1) - static_cast<T2>(1)) result_type;
        VectorTemplate<result_type , N> output;
        for (int i = 0; i < N; i++) {
            output.data[i] = this->data[i] - other.data[i];
        }
        return output;
    }

    template<typename T2>
    inline const VectorTemplate<T, N>& operator-=(const VectorTemplate<T2, N>& other) {
        for (int i = 0; i < N; i++) {
            this->data[i] -= other.data[i];
        }
        return *this;
    }

    template<typename T2>
    inline auto operator*(const VectorTemplate<T2, N>& other) const {
        typedef decltype(static_cast<T>(1) * static_cast<T2>(1)) result_type;
        VectorTemplate<result_type, N> output;
        for (int i = 0; i < N; i++) {
            output.data[i] = this->data[i] * other.data[i];
        }
        return output;
    }

    template<typename T2>
    inline const VectorTemplate<T, N>& operator*=(const VectorTemplate<T2, N>& other) {
        for (int i = 0; i < N; i++) {
            this->data[i] *= other.data[i];
        }
        return *this;
    }

    template<typename T2>
    inline auto operator*(T2 other) const {
        typedef decltype(static_cast<T>(1) * static_cast<T2>(1)) result_type;
        VectorTemplate<result_type, N> output;
        for (int i = 0; i < N; i++) {
            output.data[i] = this->data[i] * other;
        }
        return output;
    }

    template<typename T2>
    inline const VectorTemplate<T, N>& operator*=(T2 other) {
        for (int i = 0; i < N; i++) {
            this->data[i] *= other;
        }
        return *this;
    }

    template<typename T2>
    inline auto operator/(const VectorTemplate<T2, N>& other) const {
        typedef decltype(static_cast<T>(1) / static_cast<T2>(1)) result_type;
        VectorTemplate<result_type, N> output;
        for (int i = 0; i < N; i++) {
            output.data[i] = this->data[i] / other.data[i];
        }
        return output;
    }

    template<typename T2>
    inline const VectorTemplate<T, N>& operator/=(const VectorTemplate<T2, N>& other) {
        for (int i = 0; i < N; i++) {
            this->data[i] /= other.data[i];
        }
        return *this;
    }

    template<typename T2>
    inline auto operator/(T2 other) const {
        typedef decltype(static_cast<T>(1) / static_cast<T2>(1)) result_type;
        VectorTemplate<result_type, N> output;
        for (int i = 0; i < N; i++) {
            output.data[i] = this->data[i] / other;
        }
        return output;
    }

    template<typename T2>
    inline const VectorTemplate<T, N>& operator/=(T2 other) {
        for (int i = 0; i < N; i++) {
            this->data[i] /= other;
        }
        return *this;
    }
};


typedef VectorTemplate<real_t, 2> Vec2f;
typedef VectorTemplate<real_t, 3> Vec3f;
typedef VectorTemplate<real_t, 4> Vec4f;

typedef VectorTemplate<integer_t, 2> Vec2i;
typedef VectorTemplate<integer_t, 3> Vec3i;
typedef VectorTemplate<integer_t, 4> Vec4i;


template<typename T, int N>
inline VectorTemplate<T, N> floor(VectorTemplate<T, N> v) {
    VectorTemplate<T, N> output;
    for (int i = 0; i < N; i++) {
        output.data[i] = ::floor(v.data[i]);
    }
    return output;
}

template<typename T, int N>
inline VectorTemplate<integer_t, N> floor_to_int(VectorTemplate<T, N> v) {
    VectorTemplate<integer_t, N> output;
    for (int i = 0; i < N; i++) {
        output.data[i] = (integer_t) ::floor(v.data[i]);
    }
    return output;
}

template<typename T, int N>
inline VectorTemplate<T, N> ceil(VectorTemplate<T, N> v) {
    VectorTemplate<T, N> output;
    for (int i = 0; i < N; i++) {
        output.data[i] = ::ceil(v.data[i]);
    }
    return output;
}

template<typename T, int N>
inline VectorTemplate<integer_t, N> ceil_to_int(VectorTemplate<T, N> v) {
    VectorTemplate<integer_t, N> output;
    for (int i = 0; i < N; i++) {
        output.data[i] = (integer_t) ::ceil(v.data[i]);
    }
    return output;
}

template<typename T, int N>
inline T len_sq(VectorTemplate<T, N> v) {
    T result = 0;
    for (int i = 0; i < N; i++) {
        T component = v.data[i];
        result += component * component;
    }
    return result;
}

template<typename T, int N>
inline real_t len(VectorTemplate<T, N> v) {
    T result = 0;
    for (int i = 0; i < N; i++) {
        T component = v.data[i];
        result += component * component;
    }
    return sqrt((real_t) result);
}

template<typename T, int N>
inline VectorTemplate<T, N> normalize(VectorTemplate<T, N> v) {
    return v / len(v);
}

template<typename T1, typename T2, int N>
inline auto dot(VectorTemplate<T1, N> v1, VectorTemplate<T2, N> v2) {
    typedef decltype(static_cast<T1>(1) * static_cast<T2>(1)) result_type;
    result_type result = 0;
    for (int i = 0; i < N; i++) {
        result += v1.data[i] * v2.data[i];
    }
    return result;
}

inline Vec3f cross(Vec3f v1, Vec3f v2) {
    return Vec3f(v1.y * v2.z - v1.z * v2.y, v1.z * v2.x - v1.x * v2.z, v1.x * v2.y - v1.y * v2.x);
}

} // math
} // voxel

#endif //VOXEL_ENGINE_VEC_H
