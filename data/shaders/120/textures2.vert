#version 120

uniform mat4 matrix;

attribute vec2 texcoord1;
attribute vec2 texcoord0;
attribute vec3 position;

varying vec2 frag_texcoord0;
varying vec2 frag_texcoord1;

void main()
{
	gl_Position = matrix * vec4(position, 1.0);

	frag_texcoord0 = texcoord0;
	frag_texcoord1 = texcoord1;
}
