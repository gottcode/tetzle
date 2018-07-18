uniform sampler2D texture0;
uniform highp vec4 color;

varying highp vec2 frag_texcoord0;

void main()
{
    gl_FragColor = texture2D(texture0, frag_texcoord0) * color;
}
