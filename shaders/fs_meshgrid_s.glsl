#version 330

in vec2 vertex_uv;
in vec4 world_pos;
in vec4 local_pos;
in vec4 view_pos;
in vec4 proj_pos;

out vec4 output_color;

float grid_color () {
    vec2 coord = vertex_uv.xy;
    vec2 grid = 0.4 * abs (fract (coord - 0.5) - 0.5) / fwidth (coord);
    float line = min (grid.x, grid.y);
    
    return 1.0 - min(line, 1.0);
}

void main () {
    float intensity = max (0.0, min (1.0, grid_color ()));
    vec4 color = intensity * vec4 (0.960, 0.646, 0.0192, 1.0);

    output_color = color;
}