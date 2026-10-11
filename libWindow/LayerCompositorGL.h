// ==================== LayerCompositorGL.h ====================
#pragma once
#include <vector>
#include "GLSLShader.h" // Ajout de l'inclusion pour GLSLShader

namespace Regards
{
	namespace OpenGL
	{
		class GLTexture;
	}
}

struct LayerPass
{
	Regards::OpenGL::GLTexture* texture = nullptr;
	float opacity = 1.0f;
	int blendMode = 0;
};

class CLayerCompositorGL
{
public:
	CLayerCompositorGL();
	~CLayerCompositorGL();
	CLayerCompositorGL(const CLayerCompositorGL&) = delete;
	CLayerCompositorGL& operator=(const CLayerCompositorGL&) = delete;

	// Utilise la méthode IsOk() de la classe GLSLShader
	bool IsValid() { return shader.IsOk(); }

	bool Compose(Regards::OpenGL::GLTexture* background, const std::vector<LayerPass>& layers,
		Regards::OpenGL::GLTexture* target, int width, int height);

	void Release();

private:
	bool BuildProgram();
	bool EnsureTemporary(int width, int height);

	Regards::OpenGL::GLSLShader shader; // Remplacement de 'unsigned int program'
	unsigned int vao = 0;
	unsigned int fboTarget = 0;
	unsigned int fboTemp = 0;
	unsigned int tempTexture = 0;
	int tempWidth = 0;
	int tempHeight = 0;

	int locSrc = -1;
	int locDst = -1;
	int locOpacity = -1;
	int locMode = -1;
	int locCopy = -1;
};
