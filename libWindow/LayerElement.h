// ==================== LayerElement.h ====================
#pragma once
#include <ImageLoadingFormat.h>
#include "LayerBlend.h"

#define TYPE_OPENGL 1
#define TYPE_OPENCL 2

// Forward declaration pour éviter d'inclure les headers OpenGL partout
namespace Regards { namespace OpenGL { class GLTexture; } }



// ============================================================================
// CLASSE MÈRE : LayerElement
// ============================================================================
class LayerElement
{
public:
    LayerElement();
    virtual ~LayerElement();



    // Constructeur de copie "virtuel" (Pattern Prototype)
    virtual LayerElement* Clone() const = 0;

    // Interface virtuelle pour la gestion des ressources spécifiques
    virtual void DeleteTexture() = 0;
    virtual void InvalidatePicture();

    virtual int TypeLayer()
    {
        return typeLayer;
    }

    // Accesseurs communs
    void SetPicture(CImageLoadingFormat* source);
    CImageLoadingFormat* GetPicture() const { return picture; }

    // Propriétés publiques communes
    LayerBlendMode blendMode = LayerBlendMode::Normal;
    int numLayer = 0;
    bool isVisible = true;
    int opacity = 100;
    wxString layerName;
    bool needTextureUpdate = true;

    uint64_t layerId = NextLayerId();
    uint64_t pictureVersion = 0;
    int textureWidth = 0;
    int textureHeight = 0;

    static uint64_t NextLayerId();

protected:
    // Constructeur de copie protégé utilisé par les classes dérivées
    LayerElement(const LayerElement& other);

    CImageLoadingFormat* picture = nullptr;

    int typeLayer = 0;
};

// ============================================================================
// CLASSE DÉRIVÉE 1 : Gestion OpenCV cv::UMat (OpenCL / Transparent API)
// ============================================================================
class LayerElementUMat : public LayerElement
{
public:
    LayerElementUMat() : LayerElement() {
        typeLayer = TYPE_OPENCL;
    }
    virtual ~LayerElementUMat() { DeleteTexture(); }

    virtual LayerElement* Clone() const override { return new LayerElementUMat(*this); }
    virtual void DeleteTexture() override { m_umatTexture.release(); }

    // Accesseurs spécifiques à cv::UMat
    cv::UMat GetUMatTexture() const { return m_umatTexture; }
    void SetUMatTexture(const cv::UMat& texture);
    cv::UMat DetachUMatTexture();

protected:
    LayerElementUMat(const LayerElementUMat& other);

private:
    cv::UMat m_umatTexture;

};

// ============================================================================
// CLASSE DÉRIVÉE 2 : Gestion OpenGL GLTexture
// ============================================================================
class LayerElementGL : public LayerElement
{
public:
    LayerElementGL() : LayerElement(), m_glTexture(nullptr) {
        typeLayer = TYPE_OPENGL;

    }
    virtual ~LayerElementGL() { DeleteTexture(); }

    virtual LayerElement* Clone() const override { return new LayerElementGL(*this); }
    virtual void DeleteTexture() override;

    // Accesseurs spécifiques à OpenGL
    Regards::OpenGL::GLTexture* GetOpenGLTexture() const { return m_glTexture; }
    void SetOpenGLTexture(Regards::OpenGL::GLTexture* texture);
    Regards::OpenGL::GLTexture* DetachOpenGLTexture();

protected:
    LayerElementGL(const LayerElementGL& other);

private:
    Regards::OpenGL::GLTexture* m_glTexture = nullptr;
};

// ============================================================================
// GESTIONNAIRE DE LISTE : CLayerList (Inchangé dans ses algorithmes)
// ============================================================================
class CLayerList
{
public:
    CLayerList() = default;
    ~CLayerList();

    void push_back(LayerElement* element);
    size_t size() const { return m_layers.size(); }
    bool empty() const { return m_layers.empty(); }

    LayerElement* operator[](size_t index) { return m_layers[index]; }
    const LayerElement* operator[](size_t index) const { return m_layers[index]; }

    auto begin() { return m_layers.begin(); }
    auto end() { return m_layers.end(); }
    auto begin() const { return m_layers.begin(); }
    auto end() const { return m_layers.end(); }

    void SetPicture(CImageLoadingFormat* bitmapIn);
    CImageLoadingFormat* GetPictureToShow();
    int GetWidth() const { return canvasWidth; }
    int GetHeight() const { return canvasHeight; }
    void IsChanged(bool changed) { this->isChanged = changed; }

    void UpdatePictureToShow();
    bool erase(size_t index);
    bool copy(size_t index);
    bool FusionLayers(std::vector<int>* listLayer);
    bool MoveUpLayer(size_t index);
    bool MoveDownLayer(size_t index);

    // Ajout d'un enum ou paramètre pour savoir quel type instancier par défaut
    enum class BackendType { UMat, OpenGL };
    void CreateLayer(wxString layerName, BackendType type = BackendType::UMat);

    bool SetPropertiesLayer(int numLayer, wxString layerName, LayerBlendMode blendMode, int opacity);
    void FlushPendingTextureDeletes();

private:
    std::vector<LayerElement*> m_layers;
    std::vector<Regards::OpenGL::GLTexture*> m_pendingTextureDelete;
    void RetireTexture(LayerElement* layer);
    CImageLoadingFormat* finalImage = nullptr;
    bool isChanged = false;
    int canvasWidth = 0;
    int canvasHeight = 0;
};

