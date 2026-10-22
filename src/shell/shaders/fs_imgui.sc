$input v_color0, v_texcoord0

// Dear ImGui pixels: the vertex colour times the bound texture (the font atlas or an image).

#include <bgfx_shader.sh>

SAMPLER2D(s_texture, 0);

void main()
{
	gl_FragColor = v_color0 * texture2D(s_texture, v_texcoord0);
}
