#version 410

layout (quads) in;

uniform mat4 u_world;
uniform mat4 u_view;
uniform mat4 u_projection;

float decasteljeu (float b00, float b01, float b02, float b03, float t) {
    float t1 = t;
    float t0 = 1.0 - t;

    float b10, b11, b12;
    float b20, b21;
    float b30;

    b10 = t0 * b00 + t1 * b01;
    b11 = t0 * b01 + t1 * b02;
    b12 = t0 * b02 + t1 * b03;

    b20 = t0 * b10 + t1 * b11;
    b21 = t0 * b11 + t1 * b12;

    b30 = t0 * b20 + t1 * b21;

    return b30;
}

vec3 bernstein (vec3 p0, vec3 p1, vec3 p2, vec3 p3, float t) {
    vec3 res;
    res.x = decasteljeu (p0.x, p1.x, p2.x, p3.x, t);
    res.y = decasteljeu (p0.y, p1.y, p2.y, p3.y, t);
    res.z = decasteljeu (p0.z, p1.z, p2.z, p3.z, t);
    return res;
}

int loc (int row, int col) {
    return (row * 4) + col;
}

vec3 p (int row, int col) {
    return gl_in[loc(row, col)].gl_Position.xyz;
}

vec3 bernstein_grid (float u, float v) {
    vec3 p00, p01, p02, p03;
    vec3 p10, p11, p12, p13;
    vec3 p20, p21, p22, p23;
    vec3 p30, p31, p32, p33;

    vec3 p0 = gl_in[0].gl_Position.xyz;
    vec3 p1 = gl_in[1].gl_Position.xyz;
    vec3 p2 = gl_in[2].gl_Position.xyz;
    vec3 p3 = gl_in[3].gl_Position.xyz;
    vec3 e00 = gl_in[4].gl_Position.xyz;
    vec3 e01 = gl_in[5].gl_Position.xyz;
    vec3 e10 = gl_in[6].gl_Position.xyz;
    vec3 e11 = gl_in[7].gl_Position.xyz;
    vec3 e20 = gl_in[8].gl_Position.xyz;
    vec3 e21 = gl_in[9].gl_Position.xyz;
    vec3 e30 = gl_in[10].gl_Position.xyz;
    vec3 e31 = gl_in[11].gl_Position.xyz;
    vec3 f00 = gl_in[12].gl_Position.xyz;
    vec3 f01 = gl_in[13].gl_Position.xyz;
    vec3 f10 = gl_in[14].gl_Position.xyz;
    vec3 f11 = gl_in[15].gl_Position.xyz;
    vec3 f20 = gl_in[16].gl_Position.xyz;
    vec3 f21 = gl_in[17].gl_Position.xyz;
    vec3 f30 = gl_in[18].gl_Position.xyz;
    vec3 f31 = gl_in[19].gl_Position.xyz;

    vec3 F0 = (u*f01 + v*f00)/(u+v);
    vec3 F1 = ((1.0-u)*f10 + v*f11)/(1.0-u+v);
    vec3 F2 = ((1.0-u)*f21 + (1.0-v)*f20)/(2.0-u-v);
    vec3 F3 = (u*f30 + (1.0-v)*f31)/(1.0+u-v);

    p00 = p0;
    p01 = e00;
    p02 = e31;
    p03 = p3;
    p10 = e01;
    p11 = F0;
    p12 = F3;
    p13 = e30;
    p20 = e10;
    p21 = F1;
    p22 = F2;
    p23 = e21;
    p30 = p1;
    p31 = e11;
    p32 = e20;
    p33 = p2;

    vec3 b0, b1, b2, b3;

    b0 = bernstein (p00, p01, p02, p03, u);
    b1 = bernstein (p10, p11, p12, p13, u);
    b2 = bernstein (p20, p21, p22, p23, u);
    b3 = bernstein (p30, p31, p32, p33, u);

    return bernstein (b0, b1, b2, b3, v);
}

void main () {
	float u = gl_TessCoord.x;
	float v = gl_TessCoord.y;

    vec4 pos = vec4 (bernstein_grid (u, v), 1.0);
    gl_Position = u_projection * u_view * pos;
}