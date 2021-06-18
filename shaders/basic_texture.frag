#version 330 core

varying vec4 color;

void main() {
    gl_FragColor = color; //texture(TEXTURE_0, uv) * color;
}