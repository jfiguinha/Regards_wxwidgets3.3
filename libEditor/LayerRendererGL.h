// ==================== LayerRendererGL.h ====================
#pragma once
#include "LayerRenderer.h"
#include "LayerCompositorGL.h"

#include <memory>

// Composition par shaders OpenGL (CLayerCompositorGL). Possède les textures de
// fond, du calque actif et du collage volant. Les textures des autres calques
// restent portées par LayerElementGL.
class CLayerRendererGL : public CLayerRenderer
{
public:
    CLayerRendererGL();
    ~CLayerRendererGL() override;

    bool Compose(ILayerScene& scene, Regards::OpenGL::GLTexture* textureDisplay,
        int width, int height, bool activeDirty) override;
    void Release() override;

private:
    Regards::OpenGL::GLTexture* GetLayerTexture(ILayerScene& scene, LayerElement* layer,
        int width, int height);
    Regards::OpenGL::GLTexture* GetActiveLayerTexture(ILayerScene& scene, int width,
        int height, bool activeDirty);

    std::unique_ptr<CLayerCompositorGL> compositor;
    std::unique_ptr<Regards::OpenGL::GLTexture> backgroundTexture;
    cv::Size backgroundTextureSize;
    std::unique_ptr<Regards::OpenGL::GLTexture> activeLayerTexture;
    cv::Size activeLayerTextureSize;
    std::unique_ptr<Regards::OpenGL::GLTexture> floatingTexture;
};
