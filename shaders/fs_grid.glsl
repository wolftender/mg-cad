#version 330

in vec4 vertex_color;
in vec4 world_pos;
in vec4 local_pos;
in vec4 view_pos;

out vec4 output_color;

uniform float u_grid_spacing;

float grid_color (float res) {
    float x = fract(world_pos.x * res);
    float y = fract(world_pos.z * res);
    float m = 0.99;
    
    return step(m,x)+step(m,y)+step(m,1.0-x)+step(m,1.0-y);
}

void main () {
    float distance = length (world_pos);
    float intensity = min (1.0, grid_color (u_grid_spacing));

    intensity = min (1.0, (10.0 / distance) * intensity);
    output_color = intensity * vec4 (0.5,0.5,0.5,0.8);
}