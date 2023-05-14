#version 410

layout (vertices = 16) out;

void main () {
	if (gl_InvocationID == 0) {
		gl_TessLevelOuter[0] = 32.0;
		gl_TessLevelOuter[1] = 32.0;
	}

	gl_out[gl_InvocationID].gl_Position = gl_in[gl_InvocationID].gl_Position;
}
