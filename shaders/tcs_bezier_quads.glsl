#version 410

layout (vertices = 16) out;

in VS_OUT {
	vec2 uv;
} tcs_in[];

out TCS_OUT {
	vec2 uv;
} tcs_out[];

uniform uint u_resolution_v;
uniform uint u_resolution_u;

void main () {
	if (gl_InvocationID == 0) {
		gl_TessLevelOuter[0] = float (u_resolution_v);
		gl_TessLevelOuter[1] = float (u_resolution_u);
		gl_TessLevelOuter[2] = float (u_resolution_v);
		gl_TessLevelOuter[3] = float (u_resolution_u);

		gl_TessLevelInner[0] = float (u_resolution_v);
		gl_TessLevelInner[1] = float (u_resolution_u);
	}

	gl_out[gl_InvocationID].gl_Position = gl_in[gl_InvocationID].gl_Position;
	tcs_out[gl_InvocationID].uv = tcs_in[gl_InvocationID].uv;
}

