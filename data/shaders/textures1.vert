#version 120

uniform mat4 matrix;

attribute vec2 texcoord0;
attribute vec3 position;

varying vec2 frag_texcoord0;

void main(void) {
	gl_Position = matrix * vec4(position, 1.0);

	frag_texcoord0 = texcoord0;
}
