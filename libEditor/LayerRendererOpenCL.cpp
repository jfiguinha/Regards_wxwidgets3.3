// ==================== LayerRendererOpenCL.cpp ====================
#include <header.h>
#include "LayerRendererOpenCL.h"
#include <GLTexture.h>

using Regards::OpenGL::GLTexture;

CLayerRendererOpenCL::CLayerRendererOpenCL() = default;
CLayerRendererOpenCL::~CLayerRendererOpenCL() = default;

void CLayerRendererOpenCL::Release()
{
    if (compositor) compositor->Release();
    backgroundUMat.release();
    activeLayerUMat.release();
    backgroundUMatSize = cv::Size();
    activeLayerUMatSize = cv::Size();
}

//-----------------------------------------------------------------
// UMat d'un calque non actif (image redimensionnée à la taille de sortie).
//-----------------------------------------------------------------
cv::UMat CLayerRendererOpenCL::GetLayerUMat(ILayerScene& scene, LayerElement* layer,
    int width, int height)
{
    auto* layerUMat = static_cast<LayerElementUMat*>(layer);
    cv::UMat texture = layerUMat->GetUMatTexture();
    if (!texture.empty() && LayerRendering::IsLayerTextureCurrent(layer, width, height))
        return texture;

    cv::UMat prepared = scene.SceneLayerOpenCL(layer, width, height);
    if (prepared.empty()) return cv::UMat();

    layerUMat->SetUMatTexture(prepared);
    LayerRendering::MarkLayerTextureCurrent(layer, width, height);
    return prepared;
}

//-----------------------------------------------------------------
// UMat du calque actif (résultat du pipeline de filtres).
//-----------------------------------------------------------------
cv::UMat CLayerRendererOpenCL::GetActiveLayerUMat(ILayerScene& scene, int width,
    int height, bool activeDirty)
{
    const cv::Size size(width, height);
    if (!activeLayerUMat.empty() && !activeDirty && activeLayerUMatSize == size)
        return activeLayerUMat;

    cv::UMat prepared = scene.SceneActiveLayerOpenCL(width, height);
    if (prepared.empty()) return cv::UMat();

    activeLayerUMat = prepared;
    activeLayerUMatSize = size;
    return activeLayerUMat;
}

bool CLayerRendererOpenCL::Compose(ILayerScene& scene, GLTexture* textureDisplay,
    int width, int height, bool activeDirty)
{
    if (width <= 0 || height <= 0 || textureDisplay == nullptr) return false;
    if (!compositor)
        compositor = std::make_unique<CLayerCompositorOpenCL>();
    if (!compositor->IsValid()) return false;

    const cv::Size size(width, height);
    if (backgroundUMat.empty() || backgroundUMatSize != size) {
        backgroundUMat = scene.SceneBackground(width, height).getUMat(cv::ACCESS_READ);
        backgroundUMatSize = size;
    }

    std::vector<LayerPassOpenCL> passes;
    passes.reserve(scene.SceneLayers()->size() + 1);

    const int activeIndex = scene.SceneActiveIndex();
    LayerRendering::ForEachVisibleLayer(scene.SceneLayers(),
        [&](size_t index, LayerElement* layer, float opacity) {
            cv::UMat texture = (static_cast<int>(index) == activeIndex)
                ? GetActiveLayerUMat(scene, width, height, activeDirty)
                : GetLayerUMat(scene, layer, width, height);
            if (!texture.empty())
                passes.push_back(LayerPassOpenCL{ texture, opacity, static_cast<int>(layer->blendMode) });
        });

    // Collage volant : canvas transparent global + bloc redimensionné (OpenCL)
    if (scene.SceneHasFloating())
    {
        cv::UMat canvas = LayerRendering::BuildFloatingCanvas<cv::UMat>(
            scene.SceneFloatingUMat(), scene.SceneRatio(),
            scene.SceneFloatingPosition(), width, height);
        if (!canvas.empty())
            passes.push_back(LayerPassOpenCL{ canvas, 1.0f,
                static_cast<int>(LayerBlendMode::Normal) });
    }

    cv::UMat targetUMat;
    const bool composed = compositor->Compose(backgroundUMat, passes, targetUMat, width, height);

    if (composed && !targetUMat.empty())
        scene.SceneUpload(*textureDisplay, targetUMat.getMat(cv::ACCESS_READ));

    return composed;
}
