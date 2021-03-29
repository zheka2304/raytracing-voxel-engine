

#ifndef VOXEL_ENGINE_VEC_H
#define VOXEL_ENGINE_VEC_H

#include <math.h>

typedef float real_t;

class Vec2 {
public:
    real_t x, y;

    Vec2();
    Vec2(real_t x, real_t y);

    inline Vec2 operator+ (Vec2 const& other) const {
        return Vec2(x + other.x, y + other.y);
    }

    inline Vec2 operator- (Vec2 const& other) const {
        return Vec2(x - other.x, y - other.y);
    }

    inline Vec2 operator* (Vec2 const& other) const {
        return Vec2(x * other.x, y * other.y);
    }

    inline Vec2 operator* (real_t number) const {
        return Vec2(x * number, y * number);
    }

    inline Vec2 operator/ (Vec2 const& other) const {
        return Vec2(x / other.x, y / other.y);
    }

    inline Vec2 operator/ (real_t number) const {
        return Vec2(x / number, y / number);
    }
};


class Vec3 {
public:
    real_t x, y, z;

    Vec3();
    Vec3(real_t x, real_t y, real_t z);

    inline Vec3 operator+ (Vec3 const& other) const {
        return Vec3(x + other.x, y + other.y, z + other.z);
    }

    inline Vec3 operator- (Vec3 const& other) const {
        return Vec3(x - other.x, y - other.y, z - other.z);
    }

    inline Vec3 operator* (Vec3 const& other) const {
        return Vec3(x * other.x, y * other.y, z * other.z);
    }

    inline Vec3 operator* (real_t number) const {
        return Vec3(x * number, y * number, z * number);
    }

    inline Vec3 operator/ (Vec3 const& other) const {
        return Vec3(x / other.x, y / other.y, z / other.z);
    }

    inline Vec3 operator/ (real_t number) const {
        return Vec3(x / number, y / number, z / number);
    }
};


namespace VecMath {
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
};

#endif //VOXEL_ENGINE_VEC_H
