// ==================== LayerRendererCPU.cpp ====================
#include <header.h>
#include "LayerRendererCPU.h"
#include <GLTexture.h>

bool CLayerRendererCPU::Compose(ILayerScene& scene,
    Regards::OpenGL::GLTexture* textureDisplay, int width, int height, bool /*activeDirty*/)
{
    if (textureDisplay == nullptr || width <= 0 || height <= 0) return false;

    // Fond de prévisualisation opaque (damier).
    cv::Mat composed = scene.SceneBackground(width, height);

    const int activeIndex = scene.SceneActiveIndex();
    LayerRendering::ForEachVisibleLayer(scene.SceneLayers(),
        [&](size_t index, LayerElement* layer, float opacity) {
            cv::Mat layerImage;
            if (static_cast<int>(index) == activeIndex) {
                // Le calque actif utilise le résultat des filtres/preview.
                layerImage = scene.SceneActiveLayer(width, height);
            }
            else {
                layerImage = scene.SceneLayer(layer, width, height, false);
            }
            if (layerImage.empty()) return;
            LayerFonction::CompositeLayer(composed, layerImage, opacity, layer->blendMode);
        });

    scene.SceneUpload(*textureDisplay, composed);
    return true;
}
