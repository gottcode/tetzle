#version 120

uniform sampler2D texture0;
uniform sampler2D texture1;

varying vec2 frag_texcoord0;
varying vec2 frag_texcoord1;

void main()
{
    gl_FragColor = texture2D(texture0, frag_texcoord0) + texture2D(texture1, frag_texcoord1) - vec4(0.5, 0.5, 0.5, 0.5);
}
