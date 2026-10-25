// ==================== LayerRendererOpenCL.h ====================
#pragma once
#include "LayerRenderer.h"
#include "LayerCompositorOpenCL.h"

#include <memory>

// Composition OpenCL (cv::UMat, CLayerCompositorOpenCL) ; le résultat est envoyé
// dans la texture d'affichage OpenGL. Les UMat des autres calques restent
// portées par LayerElementUMat.
class CLayerRendererOpenCL : public CLayerRenderer
{
public:
    CLayerRendererOpenCL(COpenCLContext* openCLContext);
    ~CLayerRendererOpenCL() override;

    bool Compose(ILayerScene& scene, Regards::OpenGL::GLTexture* textureDisplay,
        int width, int height, bool activeDirty) override;
    void Release() override;

private:
    cv::UMat GetLayerUMat(ILayerScene& scene, LayerElement* layer, int width, int height);
    cv::UMat GetActiveLayerUMat(ILayerScene& scene, int width, int height, bool activeDirty);

    COpenCLContext* openCLContext;
    std::unique_ptr<CLayerCompositorOpenCL> compositor;
    cv::UMat backgroundUMat;
    cv::Size backgroundUMatSize;
    cv::UMat activeLayerUMat;
    cv::Size activeLayerUMatSize;
};
