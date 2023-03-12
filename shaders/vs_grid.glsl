#version 330

layout (location = 0) in vec3 a_position;
layout (location = 1) in vec4 a_color;

uniform mat4 u_world;
uniform mat4 u_view;
uniform mat4 u_projection;

out vec4 vertex_color;
out vec4 local_pos;
out vec4 world_pos;
out vec4 view_pos;
out vec4 proj_pos;

void main () {
    vertex_color = a_color;

    vec4 world = u_world * vec4 (100.0 * a_position, 1.0);
    vec4 view = u_view * world;
    vec4 proj = u_projection * view;

    local_pos = vec4 (a_position, 1.0);
    world_pos = world;
    view_pos = view;
    proj_pos = proj;

    gl_Position = proj;
}