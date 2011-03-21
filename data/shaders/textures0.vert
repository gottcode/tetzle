#version 120

uniform mat4 matrix;

attribute vec3 position;

void main(void) {
	gl_Position = matrix * vec4(position, 1.0);
}
