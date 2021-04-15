#include "color.h"


#define FLOAT_TO_INT(v) uint8_t((v) * 255)

Color::Color(uint8_t r, uint8_t g, uint8_t b, uint8_t a) :
    r(r), g(g), b(b), a(a) {

}

Color::Color(float r, float g, float b, float a) :
    r(FLOAT_TO_INT(r)), g(FLOAT_TO_INT(g)), b(FLOAT_TO_INT(b)), a(FLOAT_TO_INT(a)) {

}
