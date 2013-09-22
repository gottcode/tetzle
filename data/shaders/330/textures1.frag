#version 330

uniform sampler2D texture0;
uniform vec4 color;

in vec2 frag_texcoord0;

layout(location = 0) out vec4 out_color;

void main()
{
    out_color = texture(texture0, frag_texcoord0) * color;
}
