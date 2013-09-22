#version 330

uniform sampler2D texture0;
uniform sampler2D texture1;

in vec2 frag_texcoord0;
in vec2 frag_texcoord1;

layout(location = 0) out vec4 out_color;

void main()
{
    out_color = texture(texture0, frag_texcoord0) + texture(texture1, frag_texcoord1) - vec4(0.5, 0.5, 0.5, 0.5);
}
