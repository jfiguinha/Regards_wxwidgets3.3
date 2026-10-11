// ==================== LayerCompositorGL.h ====================
// Composition GPU des calques (OpenGL 3.3 core) : ping-pong entre deux FBO,
// un passage plein ecran par calque, avec les memes modes de fusion et la meme
// formule W3C (alpha droit) que LayerFonction::CompositeLayer.
//
// Toutes les methodes (sauf le destructeur) doivent etre appelees avec le
// contexte GL du canvas courant.
#pragma once
#include <vector>

namespace Regards
{
	namespace OpenGL
	{
		class GLTexture;
	}
}

struct LayerPass
{
	Regards::OpenGL::GLTexture* texture = nullptr; // BGRA 8 bits, meme taille que la cible
	float opacity = 1.0f;                          // 0..1
	int blendMode = 0;                             // valeur de LayerBlendMode
};

class CLayerCompositorGL
{
public:
	CLayerCompositorGL();
	~CLayerCompositorGL();
	CLayerCompositorGL(const CLayerCompositorGL&) = delete;
	CLayerCompositorGL& operator=(const CLayerCompositorGL&) = delete;

	// false si le shader n'a pas pu etre compile/lie : l'appelant doit alors
	// utiliser la composition CPU.
	bool IsValid() const { return program != 0; }

	// Ecrit directement dans 'target' (texture RGBA8 deja allouee, width x height).
	// Ordre : background, puis layers[0], layers[1], ... (dernier = dessus).
	// Retourne false, sans rien dessiner, si la cible n'est pas utilisable
	// comme attachement de FBO.
	bool Compose(Regards::OpenGL::GLTexture* background, const std::vector<LayerPass>& layers,
	             Regards::OpenGL::GLTexture* target, int width, int height);

	// Libere les objets GL. A appeler contexte courant. Le destructeur ne touche
	// volontairement pas a GL (le contexte peut deja etre detruit).
	void Release();

private:
	bool BuildProgram();
	bool EnsureTemporary(int width, int height);

	unsigned int program = 0;
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
