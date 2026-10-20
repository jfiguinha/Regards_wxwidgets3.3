// ==================== LayerRendererCPU.h ====================
#pragma once
#include "LayerRenderer.h"

// Composition CPU (cv::Mat, LayerFonction::CompositeLayer) ; le résultat est
// envoyé dans la texture d'affichage OpenGL. Sert aussi de repli aux deux autres.
class CLayerRendererCPU : public CLayerRenderer
{
public:
    CLayerRendererCPU() = default;
    ~CLayerRendererCPU() override = default;

    bool Compose(ILayerScene& scene, Regards::OpenGL::GLTexture* textureDisplay,
        int width, int height, bool activeDirty) override;
};
