#version 330

in vec2 vertex_uv;
in vec3 world_pos;
in vec3 local_pos;
in vec3 view_pos;
in vec3 proj_pos;

out vec4 output_color;

float grid_color () {
    vec2 coord = vertex_uv.xy;
    vec2 grid = 0.75 * abs (fract (coord - 0.5) - 0.5) / fwidth (coord);
    float line = min (grid.x, grid.y);
    
    return 1.0 - min(line, 1.0);
}

void main () {
    float intensity = max (0.0, min (1.0, grid_color ()));
    vec4 color = intensity * vec4 (1.0, 1.0, 1.0, 1.0);

    output_color = color;
}