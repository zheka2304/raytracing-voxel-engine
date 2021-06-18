#ifndef VOXEL_ENGINE_COLOR_H
#define VOXEL_ENGINE_COLOR_H

namespace voxel {
namespace math {

class Color {
public:
    float r, g, b, a;

    Color(float r, float g, float b, float a = 1);
};

} // math
} // voxel

#endif //VOXEL_ENGINE_COLOR_H
