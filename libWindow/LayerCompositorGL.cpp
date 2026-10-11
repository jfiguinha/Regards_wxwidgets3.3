// ==================== LayerCompositorGL.cpp ====================
#include <header.h>
#include "LayerCompositorGL.h"
#include <GLTexture.h>

using namespace Regards::OpenGL;

namespace
{
	// Sauvegarde / restaure l'etat GL modifie par la composition afin de ne pas
	// perturber le rendu qui suit (CreateScreenRender, RenderToScreen...).
	struct GLStateGuard
	{
		GLint drawFbo = 0, readFbo = 0, program = 0, vao = 0, activeTexture = 0, tex0 = 0, tex1 = 0;
		GLint viewport[4] = { 0, 0, 0, 0 };
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
	// Chargement des programmes via les ressources demandées
	if (!shader.CreateProgram("IDR_GLSL_VERTEX_RENDER", GL_VERTEX_SHADER))
		return false;

	if (!shader.CreateProgram("IDR_GLSL_EDITOR_RENDER", GL_FRAGMENT_SHADER))
		return false;

	// Génération du lien de shader (LinkProgram) et activation interne
	if (!shader.EnableShader())
		return false;

	GLuint progId = shader.GetProgramId();

	locSrc = glGetUniformLocation(progId, "uSrc");
	locDst = glGetUniformLocation(progId, "uDst");
	locOpacity = glGetUniformLocation(progId, "uOpacity");
	locMode = glGetUniformLocation(progId, "uMode");
	locCopy = glGetUniformLocation(progId, "uCopy");

	glGenVertexArrays(1, &vao);
	glGenFramebuffers(1, &fboTarget);
	glGenFramebuffers(1, &fboTemp);

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

	// Active le shader
	shader.EnableShader();

	glUniform1i(locSrc, 0);
	glUniform1i(locDst, 1);
	glBindVertexArray(vao);
	glViewport(0, 0, width, height);

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

	shader.DeleteShader(); // Demande de nettoyage des objets OpenGL internes au shader

	tempTexture = fboTarget = fboTemp = vao = 0;
	tempWidth = tempHeight = 0;
}
