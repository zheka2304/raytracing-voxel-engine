
#ifndef VOXEL_ENGINE_COLOR_H
#define VOXEL_ENGINE_COLOR_H

#include "types.h"

class Color {
public:
    uint8_t r, g, b, a;

    Color(uint8_t r, uint8_t g, uint8_t b, uint8_t a = 255);
    Color(float r, float g, float b, float a = 1);
};

#endif //VOXEL_ENGINE_COLOR_H
