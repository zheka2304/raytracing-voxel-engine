#include <cmath>
#include "baking_utils.h"

namespace ChunkBakingUtils {
    NormalHashCache::NormalHashCache() {
        for (int x = 0; x < (1 << BITS_PER_COMPONENT); x++) {
            for (int y = 0; y < (1 << BITS_PER_COMPONENT); y++) {
                for (int z = 0; z < (1 << BITS_PER_COMPONENT); z++) {
                    float nx = float(x - COMPONENT_HALF);
                    float ny = float(y - COMPONENT_HALF);
                    float nz = float(z - COMPONENT_HALF);
                    float yaw = atan2(nx, nz);
                    float pitch = atan2(-ny, sqrt(nx * nx + nz * nz));
                    int yaw_i = int((yaw + M_PI) / (2 * M_PI) * 63) & 63;
                    int pitch_i = int((pitch + M_PI_2) / (M_PI) * 31) & 31;
                    hashes[(((x << BITS_PER_COMPONENT) | y) << BITS_PER_COMPONENT) | z] = ((yaw_i << 5) | pitch_i);
                }
            }
        }
    }
}