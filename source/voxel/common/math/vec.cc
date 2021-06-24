#include "vec.h"


namespace voxel {
namespace math {

Vec2f::Vec2f() : x(0), y(0) {

}

Vec2f::Vec2f(real_t x, real_t y) : x(x), y(y) {

}

Vec3f::Vec3f() : x(0), y(0), z(0) {

}

Vec3f::Vec3f(real_t x, real_t y, real_t z) : x(x), y(y), z(z) {

}

Vec2i::Vec2i() : x(0), y(0) {

}

Vec2i::Vec2i(integer_t x, integer_t y) : x(x), y(y) {

}

Vec3i::Vec3i() : x(0), y(0), z(0) {

}

Vec3i::Vec3i(integer_t x, integer_t y, integer_t z) : x(x), y(y), z(z) {

}

} // math
} // voxel