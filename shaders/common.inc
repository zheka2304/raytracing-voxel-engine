
#ifndef VOXEL_ENGINE_COMMON_INC
#define VOXEL_ENGINE_COMMON_INC

// Not actual infinity, nor the maximum float value.
// These are used to represent large number, that will be greater than result of any normal calculations.
// For example: distances to non-existing objects, like shadow distance on pixel with no shadow
#define POSITIVE_INF 1e38
#define NEGATIVE_INF -1e38

// Same for 16 bit float, used in packHalf2x16, this is actually MAX_HALF_FLOAT / 2
#define POSITIVE_INF_HALF 32752.0
#define NEGATIVE_INF_HALF -32752.0

//
#define M_PI        3.14159265f
#define M_2_PI      6.28318531f
#define M_PI_2		1.57079632f
#define M_PI_4		0.78539816f



// shorter compute shader buffer declaration
#define COMPUTE_SHADER_BUFFER(BINDING, ACCESS_PERMISSIONS, TYPE_NAME, NAME, STRUCT) \
    layout(binding = BINDING) ACCESS_PERMISSIONS buffer TYPE_NAME STRUCT NAME;

COMPUTE_SHADER_BUFFER(${common.screen_size}, , u_screen_size_t, u_screen_size, {
    ivec2 size;
})

#define U_SCREEN_SIZE u_screen_size.size


uint i32_to_i16(uint x) {
    return (x & 0x7FFFu) | ((x >> 31u) << 15u);
}

uint i16_to_i32(uint x) {
    return (x & 0x7FFFu) | ((x >> 15u) << 31u);
}

ivec2 unpackInt2x16(uint x) {
    return ivec2(i16_to_i32(x & 0xFFFFu), i16_to_i32(x >> 16u));
}

uint packInt2x16(ivec2 x) {
    return i32_to_i16(uint(x.x)) | (i32_to_i16(uint(x.y)) << 16u);
}

// short macros for packing 2 of 16 bit ints or floats into 1 and unpacking
#define PACK_H2F(X) uintBitsToFloat(packHalf2x16(X))
#define UNPACK_H2F(X) unpackHalf2x16(floatBitsToUint(X))
#define PACK_I2F(X) uintBitsToFloat(packInt2x16(X))
#define UNPACK_I2F(X) unpackInt2x16(floatBitsToUint(X))

#endif  // VOXEL_ENGINE_COMMON_INC