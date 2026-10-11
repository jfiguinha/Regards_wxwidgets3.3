#pragma once
#include <BitmapWndViewer.h>
#include "LayerElement.h"
#include "LayerCompositorGL.h"
#include <memory>
#include <cstdint>
#include <CanvasSizeDialog.h>


class CBitmapEditor : public Regards::Control::CBitmapWndViewer
{
public:
	CBitmapEditor(CSliderInterface* slider, wxWindowID mainViewerId, const CThemeBitmapWindow& theme,
		CBitmapInterface* bitmapInterface);
	~CBitmapEditor(void);

	void Resize(const int& widthOut, const int& heightOut, const int& method);
	void SetFilename(const wxString& filename);
	void SetRealSize();
	void CanvasResize(CanvasSizeParameter canvasSize);
	void SetActifLayer(const int& numLayer);
	void ApplyEffectOnMouseRelease() override;
	void RemoveListener(const bool& applyCancel = true) override;
	void OnUpdateLayerBitmap(wxCommandEvent& event) override;
	
	CLayerList* GetListOfLayer()
	{
		return &listOfLayer;
	}

private:

	void OnPaint3D(wxGLCanvas* canvas, CRenderOpenGL* renderOpenGL) override;
	bool ApplySelectEffect(int& widthOutput, int& heightOutput)  override;
	void RenderTexture(GLTexture* textureDisplay, const bool& invertPos, int width, int height);

	// Composition des calques : GPU (FBO + shader) avec repli CPU.
	bool ComposeLayersGPU(GLTexture* textureDisplay, int width, int height, bool activeDirty);
	void ComposeLayersCPU(GLTexture* textureDisplay, int width, int height);
	GLTexture* GetLayerTexture(LayerElement* layer, int width, int height);
	GLTexture* GetActiveLayerTexture(int width, int height, bool activeDirty);

	Regards::Picture::CPictureArray pictureBackground;

	std::unique_ptr<CLayerCompositorGL> layerCompositor;
	std::unique_ptr<GLTexture> backgroundTexture;
	int backgroundTextureWidth = 0;
	int backgroundTextureHeight = 0;
	std::unique_ptr<GLTexture> activeLayerTexture;
	int activeLayerTextureWidth = 0;
	int activeLayerTextureHeight = 0;

	// Calque/version dont l'image a ete chargee dans filtreEffet.
	uint64_t lastSourceLayerId = 0;
	uint64_t lastSourceVersion = 0;
	bool gpuComposeFallbackLogged = false;

	wxWindowID parameterId;
	wxWindowID mainViewerId;
};