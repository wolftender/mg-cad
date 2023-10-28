#version 330

in GS_OUT {
    vec2 uv;
} fs_in;

out vec4 output_color;

uniform sampler2D u_domain_sampler;
uniform vec4 u_color;

void main () {
    output_color = u_color * texture(u_domain_sampler, fs_in.uv).r;
}