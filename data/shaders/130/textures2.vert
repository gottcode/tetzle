#version 130

uniform mat4 matrix;

in vec3 position;
in vec2 texcoord0;
in vec2 texcoord1;

out vec2 frag_texcoord0;
out vec2 frag_texcoord1;

void main()
{
	gl_Position = matrix * vec4(position, 1.0);

	frag_texcoord0 = texcoord0;
	frag_texcoord1 = texcoord1;
}
