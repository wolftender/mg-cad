#version 330

in GS_OUT {
    vec2 uv;
} fs_in;

out vec4 output_color;

uniform sampler2D u_domain_sampler;
uniform vec4 u_color;

void main () {
    float alpha = texture(u_domain_sampler, fs_in.uv).r;

    if (alpha == 0.0f) {
        discard;
    }

    output_color = u_color * alpha;
}