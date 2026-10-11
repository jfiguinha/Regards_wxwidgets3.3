#pragma once
#include <BitmapWndViewer.h>
#include "LayerElement.h"
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

	Regards::Picture::CPictureArray pictureBackground;

	wxWindowID parameterId;
	wxWindowID mainViewerId;
};