#version 410

layout (vertices = 2) out;

uniform int u_res_x;
uniform int u_res_y;

patch out vec4 p1;
patch out vec4 p2;
patch out vec4 p3;
patch out vec4 p4;

void main () {
	if (gl_InvocationID == 0) {
		gl_TessLevelOuter[0] = float (u_res_x);
		gl_TessLevelOuter[1] = float (u_res_y);
	}
}
