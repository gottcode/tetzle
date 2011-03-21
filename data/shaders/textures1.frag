#version 120

uniform sampler2D texture0;
uniform vec4 color;

varying vec2 frag_texcoord0;

void main(void) {
    gl_FragColor = texture2D(texture0, frag_texcoord0) * color;
}
