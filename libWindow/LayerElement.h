// ==================== LayerElement.h ====================
#pragma once
#include <ImageLoadingFormat.h>
#include "LayerBlend.h"
#include <vector>
#include <memory>
#include <atomic>
#include <wx/string.h>

#define TYPE_OPENGL 1
#define TYPE_OPENCL 2

namespace Regards { namespace OpenGL { class GLTexture; } }

// ============================================================================
// CLASSE MÈRE : LayerElement
// ============================================================================
class LayerElement
{
public:
    LayerElement();
    virtual ~LayerElement();

    virtual LayerElement* Clone() const = 0;
    virtual void DeleteTexture() = 0;
    virtual void InvalidatePicture();

    virtual int TypeLayer()
    {
        return typeLayer;
    }

    void SetPicture(CImageLoadingFormat* source);
    CImageLoadingFormat* GetPicture() const { return picture; }

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
    wxString filepath = "";


    int x = 0;
    int y = 0;

    static uint64_t NextLayerId();
    static uint64_t GetLayerId();

    virtual void Write(cv::FileStorage& fs) const;
    virtual void Read(const cv::FileNode& node);

protected:
    LayerElement(const LayerElement& other);
    static CImageLoadingFormat* CloneImage(CImageLoadingFormat* source);

    CImageLoadingFormat* picture = nullptr;

    static std::atomic<uint64_t> counter;
    int typeLayer = 0;
};

// ============================================================================
// CLASSE DÉRIVÉE 1 : cv::UMat (OpenCL)
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

    cv::UMat GetUMatTexture() const { return m_umatTexture; }
    void SetUMatTexture(const cv::UMat& texture);
    cv::UMat DetachUMatTexture();

protected:
    LayerElementUMat(const LayerElementUMat& other);

private:
    cv::UMat m_umatTexture;
};

// ============================================================================
// CLASSE DÉRIVÉE 2 : GLTexture (OpenGL)
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

    Regards::OpenGL::GLTexture* GetOpenGLTexture() const { return m_glTexture; }
    void SetOpenGLTexture(Regards::OpenGL::GLTexture* texture);
    Regards::OpenGL::GLTexture* DetachOpenGLTexture();

protected:
    LayerElementGL(const LayerElementGL& other);

private:
    Regards::OpenGL::GLTexture* m_glTexture = nullptr;
};

// ============================================================================
// GESTIONNAIRE DE LISTE : CLayerList
// ============================================================================
class CLayerList
{
public:
    CLayerList() = default;
    CLayerList(const CLayerList& other);
    ~CLayerList();

    bool SaveToFile(const wxString& filename, bool savePicture = true) const;
    bool LoadFromFile(const wxString& filename);

    LayerElement* FindElementByLayerId(const uint64_t& layerId);

    void push_back(LayerElement* element);
    size_t size() const { return m_layers.size(); }
    bool empty() const { return m_layers.empty(); }

    LayerElement* operator[](size_t index) { return m_layers[index]; }
    const LayerElement* operator[](size_t index) const { return m_layers[index]; }

    auto begin() { return m_layers.begin(); }
    auto end() { return m_layers.end(); }
    auto begin() const { return m_layers.begin(); }
    auto end() const { return m_layers.end(); }

    CImageLoadingFormat* GetPictureToShow();
    CImageLoadingFormat* ClonePictureToShow();

    int GetWidth() const { return canvasWidth; }
    int GetHeight() const { return canvasHeight; }
    void IsChanged(bool changed) { this->isChanged = changed; }

    void UpdateCanvasSize(const int& width, const int& height)
    {
        if (canvasWidth != width || canvasHeight != height)
        {
            canvasWidth = width;
            canvasHeight = height;
            isChanged = true;
        }
    }

    void UpdatePictureToShow();
    bool erase(size_t index);
    bool copy(size_t index);
    bool FusionLayers(std::vector<int>* listLayer);
    bool MoveUpLayer(size_t index);
    bool MoveDownLayer(size_t index);

    void CreateLayer(wxString layerName, BackendType type = BackendType::UMat);
    bool SetPropertiesLayer(int numLayer, wxString layerName, LayerBlendMode blendMode, int opacity);

private:
    std::vector<LayerElement*> m_layers;
    CImageLoadingFormat* finalImage = nullptr;
    bool isChanged = false;
    int canvasWidth = 0;
    int canvasHeight = 0;
};
