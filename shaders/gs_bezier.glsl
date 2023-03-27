#version 330

layout (lines_adjacency) in;
layout (triangle_strip, max_vertices = 200) out;

// screen resolution
uniform vec2 u_resolution;
uniform float u_line_width;

vec2 line_start (vec4 p1, vec4 p2) {
    // screen space ndc
    vec3 s1 = p1.xyz / p1.w;
    vec3 s2 = p2.xyz / p2.w;

    // line "coordinates" offset
    // line is a quad that is kind of like skewed in the direction
    // perpendicular to the tangent of the line
    vec2 line_forward = normalize (s2.xy - s1.xy);
    vec2 line_right = vec2 (-line_forward.y, line_forward.x);
    vec2 line_offset = (vec2 (u_line_width) / u_resolution) * line_right;

    // this is not the end of the line, so we just emit two first vertices
    // the next two are expected to be emitted by next calls
    gl_Position = vec4(p1.xy + line_offset * p1.w, p1.zw);
    EmitVertex();

    gl_Position = vec4(p1.xy - line_offset * p1.w, p1.zw);
    EmitVertex();

    return line_offset;
}

vec2 line_add (vec4 p1, vec4 p2, vec2 prev_offset) {
    // screen space ndc
    vec3 s1 = p1.xyz / p1.w;
    vec3 s2 = p2.xyz / p2.w;

    vec2 line_forward = normalize (s2.xy - s1.xy);
    vec2 line_right = vec2 (-line_forward.y, line_forward.x);
    vec2 line_offset = (vec2 (u_line_width) / u_resolution) * line_right;

    vec2 n1 = normalize (line_offset);
    vec2 n2 = normalize (prev_offset);
    vec2 n = normalize (n1 + n2);

    float avg_length = u_line_width / dot (n1, n);
    if (avg_length > u_line_width * 10.0) {
        avg_length = u_line_width * 10.0;
    }

    vec2 avg_offset = n * avg_length / u_resolution;

    // this is not the end of the line, so we just emit two first vertices
    // the next two are expected to be emitted by next calls
    gl_Position = vec4(p1.xy + avg_offset * p1.w, p1.zw);
    EmitVertex();

    gl_Position = vec4(p1.xy - avg_offset * p1.w, p1.zw);
    EmitVertex();

    return line_offset;
}

void line_end (vec4 p1, vec4 p2) {
    vec3 s1 = p1.xyz / p1.w;
    vec3 s2 = p2.xyz / p2.w;

    vec2 line_forward = normalize (s2.xy - s1.xy);
    vec2 line_right = vec2 (-line_forward.y, line_forward.x);
    vec2 offset = (vec2 (u_line_width) / u_resolution) * line_right;

    // everything the same except we just emit the last two vertices this time
    gl_Position = vec4(p2.xy + offset * p2.w, p2.zw);
    EmitVertex();

    gl_Position = vec4(p2.xy - offset * p2.w, p2.zw);
    EmitVertex();
}

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

void main () {
    // polynomial control points
    vec4 b0 = gl_in[0].gl_Position;
    vec4 b1 = gl_in[1].gl_Position;
    vec4 b2 = gl_in[2].gl_Position;
    vec4 b3 = gl_in[3].gl_Position;

    vec2 offset;
    vec4 p1, p2;

    float t = 0.0;
    float step = 1.0 / 64;

    p1.x = decasteljeu (b0.x, b1.x, b2.x, b3.x, t);
    p1.y = decasteljeu (b0.y, b1.y, b2.y, b3.y, t);
    p1.z = decasteljeu (b0.z, b1.z, b2.z, b3.z, t);
    p1.w = decasteljeu (b0.w, b1.w, b2.w, b3.w, t);

    p2.x = decasteljeu (b0.x, b1.x, b2.x, b3.x, t + step);
    p2.y = decasteljeu (b0.y, b1.y, b2.y, b3.y, t + step);
    p2.z = decasteljeu (b0.z, b1.z, b2.z, b3.z, t + step);
    p2.w = decasteljeu (b0.w, b1.w, b2.w, b3.w, t + step);

    offset = line_start (p1, p2);

    for (int i = 0; i < 63; i++) {
        t = t + step;

        p1.x = decasteljeu (b0.x, b1.x, b2.x, b3.x, t);
        p1.y = decasteljeu (b0.y, b1.y, b2.y, b3.y, t);
        p1.z = decasteljeu (b0.z, b1.z, b2.z, b3.z, t);
        p1.w = decasteljeu (b0.w, b1.w, b2.w, b3.w, t);

        p2.x = decasteljeu (b0.x, b1.x, b2.x, b3.x, t + step);
        p2.y = decasteljeu (b0.y, b1.y, b2.y, b3.y, t + step);
        p2.z = decasteljeu (b0.z, b1.z, b2.z, b3.z, t + step);
        p2.w = decasteljeu (b0.w, b1.w, b2.w, b3.w, t + step);

        offset = line_add (p1, p2, offset);
    }

    line_end (p1, p2);

    EndPrimitive();
}