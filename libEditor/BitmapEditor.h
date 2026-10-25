// ==================== BitmapEditor.h ====================
#pragma once
#include <BitmapWndViewer.h>
#include "LayerElement.h"
#include "LayerRenderer.h"
#include "LayerRendererCPU.h"
#include "LayerRendererGL.h"
#include "LayerRendererOpenCL.h"
#include <ModificationManager.h>
#include <CanvasSizeDialog.h>

#include <memory>

class CSelectFilterParameter;

class CBitmapEditor : public Regards::Control::CBitmapWndViewer, private ILayerScene
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
    CLayerList* GetListOfLayer() { return listOfLayer.get(); }
    void SetListOfLayer(CLayerList* newLayers);

    bool IsSelectionEnable();
    bool IsPasteEnable();

    // --- Presse-papiers / sélection ---
    bool Copy();
    bool Cut();
    bool Paste();
    bool DeleteSelect();

    void SelectAll();
    void InverseAll();
    void CancelSelect();

    // --- Outils ---
    void SetZoomTool();
    void SetMoveTool();
    void SetSelectMoveTool();

    // --- Gestion de l'historique (Undo / Redo) ---
    void SaveHistoryState(const wxString& actionLabel);
    void Undo();
    void Redo();
    void ClearHistory();

    void Crop();



private:


    // --- Evènements ---
    void OnSetBitmap(wxCommandEvent& event) override;
    void SetBitmapByLayerId(CImageLoadingFormat* bitmapIn, const uint64_t& layerId);
    void UpdateLayerId(CImageLoadingFormat* bitmapIn, const uint64_t& layerId);
    void MouseRelease(const int& xPos, const int& yPos);
    void SetKey(const int& iKey) override;
    int GetKey() override;

    void OnFiltreOk() override;
    // nullptr si l'outil de sélection n'est pas actif
    CSelectFilterParameter* GetSelectFilter();
    // Outil de sélection actif + calque actif avec image de taille valide
    bool GetActiveSelectionContext(CSelectFilterParameter*& filter, int& imgW, int& imgH);
    void RefreshSelectionView();
    void NotifyLayerPictureUpdated();
    void ActivateSimpleTool(int previewId, int tool, const wxCursor& cursor);

    // --- Collage (dataCopy « volant » puis fixé dans le calque) ---
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
    void ComposeLayersCPU(ILayerScene& scene, GLTexture* textureDisplay, int width, int height);

    // --- Préparation des images (utilisée via ILayerScene par les renderers) ---
    bool PrepareFilteredLayer(LayerElement* layer, CFiltreEffet* filter, int width, int height);
    cv::Mat PrepareBackgroundMat(int width, int height);
    cv::Mat PrepareLayerSource(LayerElement* layer, int width, int height, CFiltreEffet* filter);
    cv::UMat PrepareLayerSourceOpenCL(LayerElement* layer, int width, int height, CFiltreEffet* filter);
    cv::Mat PrepareActiveLayerImage(int width, int height);
    cv::UMat PrepareActiveLayerImageOpenCL(int width, int height);
    bool UploadToTexture(GLTexture& texture, const cv::Mat& image);

    // --- ILayerScene (implémentation privée, consommée par les CLayerRenderer*) ---
    CLayerList* SceneLayers() override;
    int SceneActiveIndex() const override;
    cv::Mat SceneBackground(int width, int height) override;
    cv::Mat SceneActiveLayer(int width, int height) override;
    cv::UMat SceneActiveLayerOpenCL(int width, int height) override;
    cv::Mat SceneLayer(LayerElement* layer, int width, int height, bool useGLContext) override;
    cv::UMat SceneLayerOpenCL(LayerElement* layer, int width, int height) override;
    bool SceneHasFloating() override;
    cv::Mat SceneFloatingMat() override;
    cv::UMat SceneFloatingUMat() override;
    float SceneRatio() const override;
    cv::Point SceneFloatingPosition() const override;
    bool SceneHasRenderContext() const override;
    bool SceneUpload(GLTexture& texture, const cv::Mat& image) override;

    Regards::Picture::CPictureArray pictureBackground;

    // Renderers d'affichage des calques (créés à la demande)
    std::unique_ptr<CLayerRendererCPU> rendererCPU;
    std::unique_ptr<CLayerRendererGL> rendererGL;
    std::unique_ptr<CLayerRendererOpenCL> rendererOpenCL;

    uint64_t lastSourceLayerId = 0;
    uint64_t lastSourceVersion = 0;
    bool gpuComposeFallbackLogged = false;
    bool openclComposeFallbackLogged = false;

    //CModificationManager * modificationManager = nullptr;
};