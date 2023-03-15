#version 330

in vec4 vertex_color;
in vec4 world_pos;
in vec4 local_pos;
in vec4 view_pos;
in vec4 proj_pos;

out vec4 output_color;

uniform float u_grid_spacing;
uniform vec4 u_focus_position;

float grid_color (float res) {
    vec2 coord = world_pos.xz * res;
    vec2 grid = abs (fract (coord - 0.5) - 0.5) / fwidth (coord);
    float line = min (grid.x, grid.y);
    
    return 1.0 - min(line, 1.0);
}

void main () {
    float distance = length (world_pos.xz - u_focus_position.xz);
    float intensity = min (1.0, grid_color (u_grid_spacing));

    intensity = min (1.0, (10.0 / distance) * intensity);

    float d_ax = abs (world_pos.x);
    float d_az = abs (world_pos.z);

    float xi = 1.0 - step (0.05, d_ax);
    float zi = 1.0 - step (0.05, d_az);

    output_color = intensity * vec4 (0.4 + xi, 0.4 - zi - xi, 0.4 + zi, 0.7);
}