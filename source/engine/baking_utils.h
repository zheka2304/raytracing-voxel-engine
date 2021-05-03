
#ifndef VOXEL_ENGINE_BAKING_UTILS_H
#define VOXEL_ENGINE_BAKING_UTILS_H

namespace ChunkBakingUtils {
    class NormalHashCache {
    public:
        static const int BITS_PER_COMPONENT = 3;
        static const int COMPONENT_HALF = (1 << BITS_PER_COMPONENT) / 2;
        static const int TOTAL_CACHE_SIZE = (1 << (BITS_PER_COMPONENT * 3));
        unsigned short hashes[TOTAL_CACHE_SIZE];

        NormalHashCache();
    };

    static NormalHashCache _normalHashCache;
    unsigned int inline normalToHash(int xComponent, int yComponent, int zComponent) {
        return _normalHashCache.hashes[((((xComponent + NormalHashCache::COMPONENT_HALF) << NormalHashCache::BITS_PER_COMPONENT) | (yComponent + NormalHashCache::COMPONENT_HALF)) << NormalHashCache::BITS_PER_COMPONENT) | (zComponent + NormalHashCache::COMPONENT_HALF)];
    }
}


#define __NORMAL_BAKING_ADD_VOXEL(GETTER, X, Y, Z, X_VAR, X_VAL, Y_VAR, Y_VAL, Z_VAR, Z_VAL) \
    if (GETTER(X, Y, Z)) { X_VAR += X_VAL; Y_VAR += Y_VAL; Z_VAR += Z_VAL; }

#define __NORMAL_BAKING_ADD_ALL_VOXELS_X(GETTER, X, Y, Z, X_VAR, Y_VAR, Y_VAL, Z_VAR, Z_VAL) \
    __NORMAL_BAKING_ADD_VOXEL(GETTER, X - 3, Y, Z, X_VAR, -1, Y_VAR, Y_VAL, Z_VAR, Z_VAL) \
    __NORMAL_BAKING_ADD_VOXEL(GETTER, X - 2, Y, Z, X_VAR, -1, Y_VAR, Y_VAL, Z_VAR, Z_VAL) \
    __NORMAL_BAKING_ADD_VOXEL(GETTER, X - 1, Y, Z, X_VAR, -2, Y_VAR, Y_VAL, Z_VAR, Z_VAL) \
    __NORMAL_BAKING_ADD_VOXEL(GETTER, X    , Y, Z, X_VAR, 0, Y_VAR, Y_VAL, Z_VAR, Z_VAL) \
    __NORMAL_BAKING_ADD_VOXEL(GETTER, X + 1, Y, Z, X_VAR, 2, Y_VAR, Y_VAL, Z_VAR, Z_VAL) \
    __NORMAL_BAKING_ADD_VOXEL(GETTER, X + 2, Y, Z, X_VAR, 1, Y_VAR, Y_VAL, Z_VAR, Z_VAL) \
    __NORMAL_BAKING_ADD_VOXEL(GETTER, X + 3, Y, Z, X_VAR, 1, Y_VAR, Y_VAL, Z_VAR, Z_VAL)

#define __NORMAL_BAKING_ADD_ALL_VOXELS_XY(GETTER, X, Y, Z, X_VAR, Y_VAR, Z_VAR, Z_VAL) \
    __NORMAL_BAKING_ADD_ALL_VOXELS_X(GETTER, X, Y - 3, Z, X_VAR, Y_VAR, -1, Z_VAR, Z_VAL) \
    __NORMAL_BAKING_ADD_ALL_VOXELS_X(GETTER, X, Y - 2, Z, X_VAR, Y_VAR, -1, Z_VAR, Z_VAL) \
    __NORMAL_BAKING_ADD_ALL_VOXELS_X(GETTER, X, Y - 1, Z, X_VAR, Y_VAR, -2, Z_VAR, Z_VAL) \
    __NORMAL_BAKING_ADD_ALL_VOXELS_X(GETTER, X, Y    , Z, X_VAR, Y_VAR, 0, Z_VAR, Z_VAL) \
    __NORMAL_BAKING_ADD_ALL_VOXELS_X(GETTER, X, Y + 1, Z, X_VAR, Y_VAR, 2, Z_VAR, Z_VAL) \
    __NORMAL_BAKING_ADD_ALL_VOXELS_X(GETTER, X, Y + 2, Z, X_VAR, Y_VAR, 1, Z_VAR, Z_VAL) \
    __NORMAL_BAKING_ADD_ALL_VOXELS_X(GETTER, X, Y + 3, Z, X_VAR, Y_VAR, 1,  Z_VAR, Z_VAL)

#define NORMAL_BAKING_ADD_ALL_VOXELS(GETTER, X, Y, Z, X_VAR, Y_VAR, Z_VAR) \
    __NORMAL_BAKING_ADD_ALL_VOXELS_XY(GETTER, X, Y, Z - 3, X_VAR, Y_VAR, Z_VAR, -1) \
    __NORMAL_BAKING_ADD_ALL_VOXELS_XY(GETTER, X, Y, Z - 2, X_VAR, Y_VAR, Z_VAR, -1) \
    __NORMAL_BAKING_ADD_ALL_VOXELS_XY(GETTER, X, Y, Z - 1, X_VAR, Y_VAR, Z_VAR, -2) \
    __NORMAL_BAKING_ADD_ALL_VOXELS_XY(GETTER, X, Y, Z   , X_VAR, Y_VAR, Z_VAR, 0) \
    __NORMAL_BAKING_ADD_ALL_VOXELS_XY(GETTER, X, Y, Z + 1, X_VAR, Y_VAR, Z_VAR, 2) \
    __NORMAL_BAKING_ADD_ALL_VOXELS_XY(GETTER, X, Y, Z + 2, X_VAR, Y_VAR, Z_VAR, 1) \
    __NORMAL_BAKING_ADD_ALL_VOXELS_XY(GETTER, X, Y, Z + 3, X_VAR, Y_VAR, Z_VAR, 1)


#endif //VOXEL_ENGINE_BAKING_UTILS_H
