#version 330

layout (lines_adjacency) in;
layout (triangle_strip, max_vertices = 8) out;

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

void main () {
    // polynomial control points
    vec4 b0 = gl_in[0].gl_Position;
    vec4 b1 = gl_in[1].gl_Position;
    vec4 b2 = gl_in[2].gl_Position;
    vec4 b3 = gl_in[3].gl_Position;

    vec2 offset;
    offset = line_start (b0, b1);
    offset = line_add (b1, b2, offset);
    offset = line_add (b2, b3, offset);
    line_end (b2, b3);

    EndPrimitive();
}