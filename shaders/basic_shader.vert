#version 330 core

layout (location = 0) in vec3 POSITION;

#ifdef D_TEXTURE
layout (location = 1) in vec2 UV;
#endif

#ifdef D_COLOR
layout (location = 2) in vec4 COLOR;
#endif

#ifdef D_TEXTURE
varying vec2 uv;
#endif

#ifdef D_COLOR
varying vec4 color;
#endif

void main() {

#ifdef D_TEXTURE
    uv = UV;
#endif

#ifdef D_COLOR
    color = COLOR;
#endif

    gl_Position = vec4(POSITION, 1.0);
}