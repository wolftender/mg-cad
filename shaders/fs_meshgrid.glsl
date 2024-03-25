#version 330

in VS_OUT {
    vec2 vertex_uv;
    vec2 domain_uv;
    vec4 local_pos;
    vec4 world_pos;
    vec4 view_pos;
    vec4 proj_pos;
} fs_in;

out vec4 output_color;

uniform sampler2D u_domain_sampler;
uniform vec4 u_color;

float grid_color () {
    vec2 coord = fs_in.vertex_uv.xy;
    vec2 grid = u_color.a * abs (fract (coord - 0.5) - 0.5) / fwidth (coord);
    float line = min (grid.x, grid.y);
    
    return 1.0 - min(line, 1.0);
}

void main () {
    float domain_alpha = texture(u_domain_sampler, fs_in.domain_uv.xy).r;

    if (domain_alpha == 0.0f) {
        discard;
    }

    float intensity = max (0.0, min (1.0, grid_color ()));

    if (intensity < 0.01) {
        discard;
    }

    vec4 color = domain_alpha * intensity * vec4 (u_color.rgb, 1.0);
    output_color = color;
}