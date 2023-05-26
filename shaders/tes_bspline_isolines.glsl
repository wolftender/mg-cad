#version 410

layout (isolines) in;

uniform mat4 u_world;
uniform mat4 u_view;
uniform mat4 u_projection;
uniform bool u_vertical;

float deboor (float b00, float b01, float b02, float b03, float t) {
    float N00 = 1.0;
    float N10 = (1.0 - t) * N00;
    float N11 = t * N00;
    float N20 = ((1.0 - t) * N10) / 2.0;
    float N21 = ((1.0 + t) * N10 + (2.0 - t) * N11) / 2.0;
    float N22 = (t * N11) / 2.0;
    float N30 = ((1.0 - t) * N20) / 3.0;
    float N31 = ((2.0 + t) * N20 + (2.0 - t) * N21) / 3.0;
    float N32 = ((1.0 + t) * N21 + (3.0 - t) * N22) / 3.0;
    float N33 = (t * N22) / 3.0;

    return b00 * N30 + b01 * N31 + b02 * N32 + b03 * N33;
}

vec3 bspline (vec3 p0, vec3 p1, vec3 p2, vec3 p3, float t) {
    vec3 res;
    res.x = deboor (p0.x, p1.x, p2.x, p3.x, t);
    res.y = deboor (p0.y, p1.y, p2.y, p3.y, t);
    res.z = deboor (p0.z, p1.z, p2.z, p3.z, t);
    return res;
}

int loc (int row, int col) {
    return (row * 4) + col;
}

vec3 p (int row, int col) {
    return gl_in[loc(row, col)].gl_Position.xyz;
}

vec3 bspline_grid (float u, float v) {
    vec3 p0, p1, p2, p3;

    p0 = bspline (p(0,0),p(0,1),p(0,2),p(0,3),u);
    p1 = bspline (p(1,0),p(1,1),p(1,2),p(1,3),u);
    p2 = bspline (p(2,0),p(2,1),p(2,2),p(2,3),u);
    p3 = bspline (p(3,0),p(3,1),p(3,2),p(3,3),u);

    return bspline (p0, p1, p2, p3, v);
}

void main () {
	float u = gl_TessCoord.x;
	float v = gl_TessCoord.y;

    vec4 pos;
    if (u_vertical) {
        pos = vec4 (bspline_grid (u, v), 1.0);
    } else {
        pos = vec4 (bspline_grid (v, u), 1.0);
    }

    gl_Position = u_projection * u_view * pos;
}