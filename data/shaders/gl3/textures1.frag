uniform sampler2D texture0;
uniform highp vec4 color;

in highp vec2 frag_texcoord0;

out highp vec4 out_color;

void main()
{
    out_color = texture(texture0, frag_texcoord0) * color;
}
