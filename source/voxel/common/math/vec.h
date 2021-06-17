

#ifndef VOXEL_ENGINE_VEC_H
#define VOXEL_ENGINE_VEC_H

#include <math.h>


namespace voxel {
namespace vec_math {

typedef float real_t;
typedef int integer_t;

class Vec2 {
public:
    real_t x, y;

    Vec2();

    Vec2(real_t x, real_t y);

    inline Vec2 operator+(Vec2 const& other) const {
        return Vec2(x + other.x, y + other.y);
    }

    inline Vec2 operator-(Vec2 const& other) const {
        return Vec2(x - other.x, y - other.y);
    }

    inline Vec2 operator*(Vec2 const& other) const {
        return Vec2(x * other.x, y * other.y);
    }

    inline Vec2 operator*(real_t number) const {
        return Vec2(x * number, y * number);
    }

    inline Vec2 operator/(Vec2 const& other) const {
        return Vec2(x / other.x, y / other.y);
    }

    inline Vec2 operator/(real_t number) const {
        return Vec2(x / number, y / number);
    }
};


class Vec3 {
public:
    real_t x, y, z;

    Vec3();

    Vec3(real_t x, real_t y, real_t z);

    inline Vec3 operator+(Vec3 const& other) const {
        return Vec3(x + other.x, y + other.y, z + other.z);
    }

    inline Vec3 operator-(Vec3 const& other) const {
        return Vec3(x - other.x, y - other.y, z - other.z);
    }

    inline Vec3 operator*(Vec3 const& other) const {
        return Vec3(x * other.x, y * other.y, z * other.z);
    }

    inline Vec3 operator*(real_t number) const {
        return Vec3(x * number, y * number, z * number);
    }

    inline Vec3 operator/(Vec3 const& other) const {
        return Vec3(x / other.x, y / other.y, z / other.z);
    }

    inline Vec3 operator/(real_t number) const {
        return Vec3(x / number, y / number, z / number);
    }
};


class Vec2i {
public:
    integer_t x, y;

    Vec2i();

    Vec2i(integer_t x, integer_t y);

    inline Vec2i operator+(Vec2i const& other) const {
        return Vec2i(x + other.x, y + other.y);
    }

    inline Vec2i operator-(Vec2i const& other) const {
        return Vec2i(x - other.x, y - other.y);
    }

    inline Vec2i operator*(Vec2i const& other) const {
        return Vec2i(x * other.x, y * other.y);
    }

    inline Vec2i operator*(integer_t number) const {
        return Vec2i(x * number, y * number);
    }

    inline Vec2i operator/(Vec2i const& other) const {
        return Vec2i(x / other.x, y / other.y);
    }

    inline Vec2i operator/(integer_t number) const {
        return Vec2i(x / number, y / number);
    }
};


class Vec3i {
public:
    integer_t x, y, z;

    Vec3i();

    Vec3i(integer_t x, integer_t y, integer_t z);

    inline Vec3i operator+(Vec3i const& other) const {
        return Vec3i(x + other.x, y + other.y, z + other.z);
    }

    inline Vec3i operator-(Vec3i const& other) const {
        return Vec3i(x - other.x, y - other.y, z - other.z);
    }

    inline Vec3i operator*(Vec3i const& other) const {
        return Vec3i(x * other.x, y * other.y, z * other.z);
    }

    inline Vec3i operator*(integer_t number) const {
        return Vec3i(x * number, y * number, z * number);
    }

    inline Vec3i operator/(Vec3i const& other) const {
        return Vec3i(x / other.x, y / other.y, z / other.z);
    }

    inline Vec3i operator/(integer_t number) const {
        return Vec3i(x / number, y / number, z / number);
    }
};


inline Vec2 floor(Vec2 v) {
    return Vec2(::floor(v.x), ::floor(v.y));
}

inline Vec3 floor(Vec3 v) {
    return Vec3(::floor(v.x), ::floor(v.y), ::floor(v.z));
}

inline Vec2i floor_to_int(Vec2 v) {
    return Vec2i((int) ::floor(v.x), (int) ::floor(v.y));
}

inline Vec3i floor_to_int(Vec3 v) {
    return Vec3i((int) ::floor(v.x), (int) ::floor(v.y), (int) ::floor(v.z));
}

inline Vec2 ceil(Vec2 v) {
    return Vec2(::ceil(v.x), ::ceil(v.y));
}

inline Vec3 ceil(Vec3 v) {
    return Vec3(::ceil(v.x), ::ceil(v.y), ::ceil(v.z));
}

inline Vec2i ceil_to_int(Vec2 v) {
    return Vec2i((int) ::ceil(v.x), (int) ::ceil(v.y));
}

inline Vec3i ceil_to_int(Vec3 v) {
    return Vec3i((int) ::ceil(v.x), (int) ::ceil(v.y), (int) ::ceil(v.z));
}

inline real_t len_sq(Vec2 v) {
    return v.x * v.x + v.y * v.y;
}

inline real_t len(Vec2 v) {
    return sqrt(v.x * v.x + v.y * v.y);
}

inline real_t len_sq(Vec3 v) {
    return v.x * v.x + v.y * v.y + v.z * v.z;
}

inline real_t len(Vec3 v) {
    return sqrt(v.x * v.x + v.y * v.y + v.z * v.z);
}

inline Vec2 normalize(Vec2 v) {
    real_t l = len(v);
    return v / l;
}

inline Vec3 normalize(Vec3 v) {
    real_t l = len(v);
    return v / l;
}

inline real_t dot(Vec2 v1, Vec2 v2) {
    return v1.x * v2.x + v1.y * v2.y;
}

inline real_t dot(Vec3 v1, Vec3 v2) {
    return v1.x * v2.x + v1.y * v2.y + v1.z * v2.z;
}

inline Vec3 cross(Vec3 v1, Vec3 v2) {
    return Vec3(v1.y * v2.z - v1.z * v2.y, v1.z * v2.x - v1.x * v2.z, v1.x * v2.y - v1.y * v2.x);
}

} // vec_math
} // voxel

#endif //VOXEL_ENGINE_VEC_H
