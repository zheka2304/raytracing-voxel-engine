#version 330 core

uniform sampler2D TEXTURE_0;

#ifdef D_TEXTURE
varying vec2 uv;
#endif

#ifdef D_COLOR
varying vec4 color;
#endif

void main() {
    vec4 result = vec4(1.0);

#ifdef D_TEXTURE
    result *= texture(TEXTURE_0, uv);
#endif

#ifdef D_COLOR
    result *= color;
#endif

    gl_FragColor = result;
}