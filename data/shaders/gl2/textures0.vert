uniform mat4 matrix;

attribute vec3 position;

void main()
{
	gl_Position = matrix * vec4(position, 1.0);
}
