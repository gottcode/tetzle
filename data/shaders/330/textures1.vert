#version 330

uniform mat4 matrix;

layout(location = 0) in vec3 position;
layout(location = 1) in vec2 texcoord0;

out vec2 frag_texcoord0;

void main()
{
	gl_Position = matrix * vec4(position, 1.0);

	frag_texcoord0 = texcoord0;
}
