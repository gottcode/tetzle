uniform sampler2D texture0;
uniform sampler2D texture1;

in highp vec2 frag_texcoord0;
in highp vec2 frag_texcoord1;

out highp vec4 out_color;

void main()
{
    out_color = texture(texture0, frag_texcoord0) + texture(texture1, frag_texcoord1) - vec4(0.5, 0.5, 0.5, 0.5);
}
