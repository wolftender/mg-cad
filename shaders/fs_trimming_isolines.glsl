#version 330

in GS_OUT {
    vec2 uv;
} fs_in;

out vec4 output_color;

uniform vec4 u_color;

void main () {
    output_color = u_color;
}