#version 330

layout (location = 0) in vec3 a_position;
layout (location = 1) in vec2 a_uv;
layout (location = 2) in vec2 a_tex_uv;

uniform mat4 u_world;
uniform mat4 u_view;
uniform mat4 u_projection;

out VS_OUT {
    vec2 vertex_uv;
    vec2 domain_uv;
    vec4 local_pos;
    vec4 world_pos;
    vec4 view_pos;
    vec4 proj_pos;
} vs_out;

void main () {
    vs_out.vertex_uv = a_uv;
    vs_out.domain_uv = a_tex_uv;

    vec4 world = u_world * vec4 (a_position, 1.0);
    vec4 view = u_view * world;
    vec4 proj = u_projection * view;

    vs_out.local_pos = vec4 (a_position, 1.0);
    vs_out.world_pos = world;
    vs_out.view_pos = view;
    vs_out.proj_pos = proj;

    gl_Position = proj;
}