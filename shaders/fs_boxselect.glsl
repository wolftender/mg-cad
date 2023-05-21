#version 330

in vec4 vertex_color;
in vec2 uv;

out vec4 output_color;

void main () {
    output_color = vec4 (vertex_color.xyz, 0.35);
}