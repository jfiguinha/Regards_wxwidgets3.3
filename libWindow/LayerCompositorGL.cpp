// ==================== LayerCompositorGL.cpp ====================
#include <header.h>
#include "LayerCompositorGL.h"
#include <GLTexture.h>

using namespace Regards::OpenGL;

namespace
{
	const char* kVertexSource = R"GLSL(#version 330 core
void main()
{
    // Triangle plein ecran genere depuis gl_VertexID (aucun VBO).
    vec2 pos = vec2(float((gl_VertexID << 1) & 2), float(gl_VertexID & 2));
    gl_Position = vec4(pos * 2.0 - 1.0, 0.0, 1.0);
}
)GLSL";

	// Meme formule que LayerFonction::CompositeLayer / BlendChannel.
	// texelFetch : les textures ont exactement la taille de la cible, pas de filtrage.
	const char* kFragmentSource = R"GLSL(#version 330 core
uniform sampler2D uSrc;
uniform sampler2D uDst;
uniform float uOpacity;
uniform int uMode;
uniform int uCopy;
out vec4 FragColor;

vec3 BlendColor(vec3 s, vec3 d, int mode)
{
    if (mode == 1) return s * d;                       // Multiply
    if (mode == 2) return s + d - s * d;               // Screen
    if (mode == 3)                                     // Overlay
    {
        vec3 low = step(d, vec3(0.5));                 // 1.0 si d <= 0.5
        return low * (2.0 * s * d)
             + (1.0 - low) * (1.0 - 2.0 * (1.0 - s) * (1.0 - d));
    }
    if (mode == 4) return min(s, d);                   // Darken
    if (mode == 5) return max(s, d);                   // Lighten
    if (mode == 6) return min(vec3(1.0), s + d);       // Add
    if (mode == 7) return max(vec3(0.0), d - s);       // Subtract
    return s;                                          // Normal
}

void main()
{
    ivec2 p = ivec2(gl_FragCoord.xy);
    vec4 src = texelFetch(uSrc, p, 0);
    if (uCopy != 0)
    {
        FragColor = src;
        return;
    }

    vec4 dst = texelFetch(uDst, p, 0);
    float sa = src.a * uOpacity;
    if (sa <= 0.0)
    {
        FragColor = dst;
        return;
    }

    float da = dst.a;
    float oa = sa + da * (1.0 - sa);
    vec3 blended = BlendColor(src.rgb, dst.rgb, uMode);
    vec3 pm = (1.0 - sa) * da * dst.rgb
            + (1.0 - da) * sa * src.rgb
            + sa * da * blended;
    vec3 oc = (oa > 0.0) ? pm / oa : vec3(0.0);
    FragColor = vec4(clamp(oc, 0.0, 1.0), clamp(oa, 0.0, 1.0));
}
)GLSL";

	GLuint CompileShader(GLenum type, const char* source)
	{
		GLuint shader = glCreateShader(type);
		glShaderSource(shader, 1, &source, nullptr);
		glCompileShader(shader);

		GLint ok = GL_FALSE;
		glGetShaderiv(shader, GL_COMPILE_STATUS, &ok);
		if (!ok)
		{
			char log[1024] = {};
			glGetShaderInfoLog(shader, sizeof(log), nullptr, log);
			wxLogError("CLayerCompositorGL : echec de compilation du shader : %s", log);
			glDeleteShader(shader);
			return 0;
		}
		return shader;
	}

	// Sauvegarde / restaure l'etat GL modifie par la composition afin de ne pas
	// perturber le rendu qui suit (CreateScreenRender, RenderToScreen...).
	struct GLStateGuard
	{
		GLint drawFbo = 0, readFbo = 0, program = 0, vao = 0, activeTexture = 0, tex0 = 0, tex1 = 0;
		GLint viewport[4] = {0, 0, 0, 0};
		GLboolean blend, depth, scissor, cull, stencil;
		GLboolean colorMask[4];

		GLStateGuard()
		{
			glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &drawFbo);
			glGetIntegerv(GL_READ_FRAMEBUFFER_BINDING, &readFbo);
			glGetIntegerv(GL_CURRENT_PROGRAM, &program);
			glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &vao);
			glGetIntegerv(GL_VIEWPORT, viewport);
			glGetIntegerv(GL_ACTIVE_TEXTURE, &activeTexture);
			glActiveTexture(GL_TEXTURE0);
			glGetIntegerv(GL_TEXTURE_BINDING_2D, &tex0);
			glActiveTexture(GL_TEXTURE1);
			glGetIntegerv(GL_TEXTURE_BINDING_2D, &tex1);
			glActiveTexture(static_cast<GLenum>(activeTexture));

			blend = glIsEnabled(GL_BLEND);
			depth = glIsEnabled(GL_DEPTH_TEST);
			scissor = glIsEnabled(GL_SCISSOR_TEST);
			cull = glIsEnabled(GL_CULL_FACE);
			stencil = glIsEnabled(GL_STENCIL_TEST);
			glGetBooleanv(GL_COLOR_WRITEMASK, colorMask);

			glDisable(GL_BLEND);
			glDisable(GL_DEPTH_TEST);
			glDisable(GL_SCISSOR_TEST);
			glDisable(GL_CULL_FACE);
			glDisable(GL_STENCIL_TEST);
			glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
		}

		static void Restore(GLenum cap, GLboolean wasEnabled)
		{
			if (wasEnabled)
				glEnable(cap);
			else
				glDisable(cap);
		}

		~GLStateGuard()
		{
			glBindFramebuffer(GL_DRAW_FRAMEBUFFER, static_cast<GLuint>(drawFbo));
			glBindFramebuffer(GL_READ_FRAMEBUFFER, static_cast<GLuint>(readFbo));
			glUseProgram(static_cast<GLuint>(program));
			glBindVertexArray(static_cast<GLuint>(vao));
			glViewport(viewport[0], viewport[1], viewport[2], viewport[3]);

			glActiveTexture(GL_TEXTURE1);
			glBindTexture(GL_TEXTURE_2D, static_cast<GLuint>(tex1));
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, static_cast<GLuint>(tex0));
			glActiveTexture(static_cast<GLenum>(activeTexture));

			Restore(GL_BLEND, blend);
			Restore(GL_DEPTH_TEST, depth);
			Restore(GL_SCISSOR_TEST, scissor);
			Restore(GL_CULL_FACE, cull);
			Restore(GL_STENCIL_TEST, stencil);
			glColorMask(colorMask[0], colorMask[1], colorMask[2], colorMask[3]);
		}
	};
}

CLayerCompositorGL::CLayerCompositorGL()
{
	BuildProgram();
}

CLayerCompositorGL::~CLayerCompositorGL()
{
	// Pas d'appel GL ici : voir Release().
}

bool CLayerCompositorGL::BuildProgram()
{
	GLuint vs = CompileShader(GL_VERTEX_SHADER, kVertexSource);
	GLuint fs = vs ? CompileShader(GL_FRAGMENT_SHADER, kFragmentSource) : 0;
	if (!vs || !fs)
	{
		if (vs) glDeleteShader(vs);
		if (fs) glDeleteShader(fs);
		return false;
	}

	GLuint prog = glCreateProgram();
	glAttachShader(prog, vs);
	glAttachShader(prog, fs);
	glLinkProgram(prog);
	glDeleteShader(vs);
	glDeleteShader(fs);

	GLint ok = GL_FALSE;
	glGetProgramiv(prog, GL_LINK_STATUS, &ok);
	if (!ok)
	{
		char log[1024] = {};
		glGetProgramInfoLog(prog, sizeof(log), nullptr, log);
		wxLogError("CLayerCompositorGL : echec de l'edition de liens : %s", log);
		glDeleteProgram(prog);
		return false;
	}

	locSrc = glGetUniformLocation(prog, "uSrc");
	locDst = glGetUniformLocation(prog, "uDst");
	locOpacity = glGetUniformLocation(prog, "uOpacity");
	locMode = glGetUniformLocation(prog, "uMode");
	locCopy = glGetUniformLocation(prog, "uCopy");

	glGenVertexArrays(1, &vao);
	glGenFramebuffers(1, &fboTarget);
	glGenFramebuffers(1, &fboTemp);

	program = prog;
	return true;
}

bool CLayerCompositorGL::EnsureTemporary(int width, int height)
{
	if (tempTexture != 0 && tempWidth == width && tempHeight == height)
		return true;

	if (tempTexture != 0)
	{
		glDeleteTextures(1, &tempTexture);
		tempTexture = 0;
	}

	glGenTextures(1, &tempTexture);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, tempTexture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	glBindFramebuffer(GL_FRAMEBUFFER, fboTemp);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, tempTexture, 0);
	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
	{
		glDeleteTextures(1, &tempTexture);
		tempTexture = 0;
		return false;
	}

	tempWidth = width;
	tempHeight = height;
	return true;
}

bool CLayerCompositorGL::Compose(GLTexture* background, const std::vector<LayerPass>& layers,
                                 GLTexture* target, int width, int height)
{
	if (!IsValid() || background == nullptr || target == nullptr || width <= 0 || height <= 0)
		return false;

	for (const LayerPass& layer : layers)
		if (layer.texture == nullptr)
			return false;

	GLStateGuard guard;

	// Taille reelle de la cible (ne pas se fier aux membres de GLTexture).
	GLint targetWidth = 0, targetHeight = 0;
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, target->GetTextureID());
	glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &targetWidth);
	glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_HEIGHT, &targetHeight);
	if (targetWidth != width || targetHeight != height)
		return false;

	if (!EnsureTemporary(width, height))
		return false;

	glBindFramebuffer(GL_FRAMEBUFFER, fboTarget);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, target->GetTextureID(), 0);
	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
	{
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, 0, 0);
		return false;
	}

	glUseProgram(program);
	glUniform1i(locSrc, 0);
	glUniform1i(locDst, 1);
	glBindVertexArray(vao);
	glViewport(0, 0, width, height);

	// Passe 0 = fond, passes 1..N = calques. Les ecritures alternent entre la
	// cible et la texture temporaire, en sens inverse pour que la DERNIERE passe
	// ecrive dans la cible : pas de copie finale.
	const GLuint targetId = target->GetTextureID();
	const size_t total = layers.size() + 1;
	for (size_t i = 0; i < total; ++i)
	{
		const bool writeToTarget = ((total - 1 - i) % 2) == 0;
		glBindFramebuffer(GL_FRAMEBUFFER, writeToTarget ? fboTarget : fboTemp);

		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, writeToTarget ? tempTexture : targetId);
		glActiveTexture(GL_TEXTURE0);

		if (i == 0)
		{
			glBindTexture(GL_TEXTURE_2D, background->GetTextureID());
			glUniform1i(locCopy, 1);
			glUniform1f(locOpacity, 1.0f);
			glUniform1i(locMode, 0);
		}
		else
		{
			const LayerPass& layer = layers[i - 1];
			glBindTexture(GL_TEXTURE_2D, layer.texture->GetTextureID());
			glUniform1i(locCopy, 0);
			glUniform1f(locOpacity, layer.opacity);
			glUniform1i(locMode, layer.blendMode);
		}
		glDrawArrays(GL_TRIANGLES, 0, 3);
	}

	// Ne pas garder de reference sur la texture d'affichage dans notre FBO.
	glBindFramebuffer(GL_FRAMEBUFFER, fboTarget);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, 0, 0);
	return true;
}

void CLayerCompositorGL::Release()
{
	if (tempTexture) glDeleteTextures(1, &tempTexture);
	if (fboTarget) glDeleteFramebuffers(1, &fboTarget);
	if (fboTemp) glDeleteFramebuffers(1, &fboTemp);
	if (vao) glDeleteVertexArrays(1, &vao);
	if (program) glDeleteProgram(program);
	tempTexture = fboTarget = fboTemp = vao = program = 0;
	tempWidth = tempHeight = 0;
}
