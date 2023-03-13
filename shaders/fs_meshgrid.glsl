#version 330

in vec2 vertex_uv;
in vec4 world_pos;
in vec4 local_pos;
in vec4 view_pos;
in vec4 proj_pos;

out vec4 output_color;

void main () {
    output_color = vec4 (vertex_uv, 0.0, 1.0);
}