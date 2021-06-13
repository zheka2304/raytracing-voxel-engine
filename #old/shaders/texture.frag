#version 330 core

uniform sampler2D TEXTURE_0;

varying vec4 color;
varying vec2 uv;

void main() {
    gl_FragColor = texture(TEXTURE_0, uv) * color;
}