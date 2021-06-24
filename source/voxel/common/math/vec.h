

#ifndef VOXEL_ENGINE_VEC_H
#define VOXEL_ENGINE_VEC_H

#include <math.h>


namespace voxel {
namespace math {

typedef float real_t;
typedef int integer_t;

class Vec2f {
public:
    real_t x, y;

    Vec2f();

    Vec2f(real_t x, real_t y);

    inline Vec2f operator+(Vec2f const& other) const {
        return Vec2f(x + other.x, y + other.y);
    }

    inline Vec2f operator-(Vec2f const& other) const {
        return Vec2f(x - other.x, y - other.y);
    }

    inline Vec2f operator*(Vec2f const& other) const {
        return Vec2f(x * other.x, y * other.y);
    }

    inline Vec2f operator*(real_t number) const {
        return Vec2f(x * number, y * number);
    }

    inline Vec2f operator/(Vec2f const& other) const {
        return Vec2f(x / other.x, y / other.y);
    }

    inline Vec2f operator/(real_t number) const {
        return Vec2f(x / number, y / number);
    }
};


class Vec3f {
public:
    real_t x, y, z;

    Vec3f();

    Vec3f(real_t x, real_t y, real_t z);

    inline Vec3f operator+(Vec3f const& other) const {
        return Vec3f(x + other.x, y + other.y, z + other.z);
    }

    inline Vec3f operator-(Vec3f const& other) const {
        return Vec3f(x - other.x, y - other.y, z - other.z);
    }

    inline Vec3f operator*(Vec3f const& other) const {
        return Vec3f(x * other.x, y * other.y, z * other.z);
    }

    inline Vec3f operator*(real_t number) const {
        return Vec3f(x * number, y * number, z * number);
    }

    inline Vec3f operator/(Vec3f const& other) const {
        return Vec3f(x / other.x, y / other.y, z / other.z);
    }

    inline Vec3f operator/(real_t number) const {
        return Vec3f(x / number, y / number, z / number);
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


inline Vec2f floor(Vec2f v) {
    return Vec2f(::floor(v.x), ::floor(v.y));
}

inline Vec3f floor(Vec3f v) {
    return Vec3f(::floor(v.x), ::floor(v.y), ::floor(v.z));
}

inline Vec2i floor_to_int(Vec2f v) {
    return Vec2i((int) ::floor(v.x), (int) ::floor(v.y));
}

inline Vec3i floor_to_int(Vec3f v) {
    return Vec3i((int) ::floor(v.x), (int) ::floor(v.y), (int) ::floor(v.z));
}

inline Vec2f ceil(Vec2f v) {
    return Vec2f(::ceil(v.x), ::ceil(v.y));
}

inline Vec3f ceil(Vec3f v) {
    return Vec3f(::ceil(v.x), ::ceil(v.y), ::ceil(v.z));
}

inline Vec2i ceil_to_int(Vec2f v) {
    return Vec2i((int) ::ceil(v.x), (int) ::ceil(v.y));
}

inline Vec3i ceil_to_int(Vec3f v) {
    return Vec3i((int) ::ceil(v.x), (int) ::ceil(v.y), (int) ::ceil(v.z));
}

inline real_t len_sq(Vec2f v) {
    return v.x * v.x + v.y * v.y;
}

inline real_t len(Vec2f v) {
    return sqrt(v.x * v.x + v.y * v.y);
}

inline real_t len_sq(Vec3f v) {
    return v.x * v.x + v.y * v.y + v.z * v.z;
}

inline real_t len(Vec3f v) {
    return sqrt(v.x * v.x + v.y * v.y + v.z * v.z);
}

inline Vec2f normalize(Vec2f v) {
    real_t l = len(v);
    return v / l;
}

inline Vec3f normalize(Vec3f v) {
    real_t l = len(v);
    return v / l;
}

inline real_t dot(Vec2f v1, Vec2f v2) {
    return v1.x * v2.x + v1.y * v2.y;
}

inline real_t dot(Vec3f v1, Vec3f v2) {
    return v1.x * v2.x + v1.y * v2.y + v1.z * v2.z;
}

inline Vec3f cross(Vec3f v1, Vec3f v2) {
    return Vec3f(v1.y * v2.z - v1.z * v2.y, v1.z * v2.x - v1.x * v2.z, v1.x * v2.y - v1.y * v2.x);
}

} // math
} // voxel

#endif //VOXEL_ENGINE_VEC_H
