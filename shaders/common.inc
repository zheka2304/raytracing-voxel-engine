
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

#endif  // VOXEL_ENGINE_COMMON_INC