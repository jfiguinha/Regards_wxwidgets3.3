// ==================== BitmapEditor.h ====================
#pragma once
#include <BitmapWndViewer.h>
#include "LayerElement.h"
#include "LayerCompositorGL.h"
#include "LayerCompositorOpenCL.h"

#include <CanvasSizeDialog.h>

#include <memory>

class CSelectFilterParameter;

class CBitmapEditor : public Regards::Control::CBitmapWndViewer
{
public:
    CBitmapEditor(CSliderInterface* slider, wxWindowID mainViewerId, const CThemeBitmapWindow& theme,
        CBitmapInterface* bitmapInterface);
    ~CBitmapEditor(void);

    // --- Image / géométrie ---
    void Resize(const int& widthOut, const int& heightOut, const int& method);
    void CanvasResize(CanvasSizeParameter canvasSize);
    void SetFilename(const wxString& filename);
    void SetRealSize();
    void SetActifLayer(const int& numLayer);

    // --- Surcharges ---
    void ApplyEffectOnMouseRelease() override;
    void RemoveListener(const bool& applyCancel = true) override;
    void OnUpdateLayerBitmap(wxCommandEvent& event) override;

    CRenderOpenGL* GetRenderOpenGL() const { return renderOpenGL; }
    CLayerList* GetListOfLayer() { return &listOfLayer; }

    // --- Presse-papiers / sélection ---
    bool Copy();
    bool Cut();
    bool Paste();

    void SelectAll();
    void InverseAll();
    void CancelSelect();

    // --- Outils ---
    void SetZoomTool();
    void SetMoveTool();
    void SetSelectMoveTool();

private:
    // --- Evènements ---
    void OnSetBitmap(wxCommandEvent& event) override;
    void SetBitmapByLayerId(CImageLoadingFormat* bitmapIn, const uint64_t& layerId);
    void UpdateLayerId(CImageLoadingFormat* bitmapIn, const uint64_t& layerId);
    void MouseRelease(const int& xPos, const int& yPos);
    void SetKey(const int& iKey) override;
    int GetKey() override;

    // --- Accès au calque actif / à l'outil de sélection ---
    // nullptr si l'index du calque actif est invalide ou si le calque est nul
    LayerElement* GetActiveLayerElement();
    // nullptr si l'outil de sélection n'est pas actif
    CSelectFilterParameter* GetSelectFilter();
    // Outil de sélection actif + calque actif avec image de taille valide
    bool GetActiveSelectionContext(CSelectFilterParameter*& filter, int& imgW, int& imgH);
    void RefreshSelectionView();
    void NotifyLayerPictureUpdated();
    void ActivateSimpleTool(int previewId, int tool, const wxCursor& cursor);

    // --- Collage (dataCopy « volant » puis fixé dans le calque) ---
    LayerElement* CommitFloatingPaste();
    void ApplyFloatingPasteToFilter();

    // --- Rendu ---
    void OnPaint3D(wxGLCanvas* canvas, CRenderOpenGL* renderOpenGL) override;
    bool ApplySelectEffect(int& widthOutput, int& heightOutput) override;
    void RenderTexture(GLTexture* textureDisplay, const bool& invertPos, int width, int height);
    void GenerateScreenBitmap(CFiltreEffet* filtreEffet, int& widthOutput, int& heightOutput);
    // Régénère le bitmap d'écran du calque actif si nécessaire. Retourne true si le calque actif a changé.
    bool UpdateScreenBitmap(LayerElement* activeLayer, int& widthOutput, int& heightOutput, bool bitmapIsLoad);

    // --- Etat de la vue / géométrie (communs à Resize, CanvasResize, OnUpdateLayerBitmap) ---
    void ResetViewState();
    bool UpdateBitmapSizeFromLayers();
    void ApplyGeometryChange();

    // --- Composition : routage selon le backend du calque, avec repli CPU ---
    void ComposeLayers(LayerElement* typeSource, GLTexture* textureDisplay, int width, int height, bool activeDirty);

    // --- Briques communes aux backends ---
    bool IsActiveLayer(size_t index) const
    {
        return static_cast<int>(index) == numActifLayer;
    }
    bool PrepareFilteredLayer(LayerElement* layer, CFiltreEffet* filter, int width, int height);
    cv::Mat PrepareBackgroundMat(int width, int height);
    cv::Mat PrepareLayerSource(LayerElement* layer, int width, int height, CFiltreEffet* filter);
    cv::UMat PrepareLayerSourceOpenCL(LayerElement* layer, int width, int height, CFiltreEffet* filter);
    cv::Mat PrepareActiveLayerImage(int width, int height);
    cv::UMat PrepareActiveLayerImageOpenCL(int width, int height);

    bool UploadToTexture(GLTexture& texture, const cv::Mat& image);

    // --- Backend OpenGL ---
    bool ComposeLayersGPU(GLTexture* textureDisplay, int width, int height, bool activeDirty);
    void ComposeLayersCPU(GLTexture* textureDisplay, int width, int height);
    GLTexture* GetLayerTexture(LayerElement* layer, int width, int height);
    GLTexture* GetActiveLayerTexture(int width, int height, bool activeDirty);

    // --- Backend OpenCL ---
    bool ComposeLayersOpenCL(GLTexture* textureDisplay, int width, int height, bool activeDirty);
    cv::UMat GetLayerUMat(LayerElement* layer, int width, int height);
    cv::UMat GetActiveLayerUMat(int width, int height, bool activeDirty);

    Regards::Picture::CPictureArray pictureBackground;

    // Ressources OpenGL
    std::unique_ptr<CLayerCompositorGL> layerCompositor;
    std::unique_ptr<GLTexture> backgroundTexture;
    cv::Size backgroundTextureSize;
    std::unique_ptr<GLTexture> activeLayerTexture;
    cv::Size activeLayerTextureSize;

    // Ressources OpenCL
    std::unique_ptr<CLayerCompositorOpenCL> layerCompositorOpenCL;
    cv::UMat backgroundUMat;
    cv::Size backgroundUMatSize;
    cv::UMat activeLayerUMat;
    cv::Size activeLayerUMatSize;

    bool testPaste = false;

    uint64_t lastSourceLayerId = 0;
    uint64_t lastSourceVersion = 0;
    bool gpuComposeFallbackLogged = false;
    bool openclComposeFallbackLogged = false;
};