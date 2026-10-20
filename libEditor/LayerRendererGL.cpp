// ==================== LayerRendererGL.cpp ====================
#include <header.h>
#include "LayerRendererGL.h"
#include <GLTexture.h>

using Regards::OpenGL::GLTexture;

CLayerRendererGL::CLayerRendererGL() = default;
CLayerRendererGL::~CLayerRendererGL() = default;

void CLayerRendererGL::Release()
{
    if (compositor) compositor->Release();
    backgroundTexture.reset();
    activeLayerTexture.reset();
    floatingTexture.reset();
    backgroundTextureSize = cv::Size();
    activeLayerTextureSize = cv::Size();
}

//-----------------------------------------------------------------
// Texture GPU d'un calque non actif (image redimensionnée à la taille de
// sortie). Rechargée uniquement si l'image ou la taille de sortie a changé.
//-----------------------------------------------------------------
GLTexture* CLayerRendererGL::GetLayerTexture(ILayerScene& scene, LayerElement* layer,
    int width, int height)
{
    if (layer->TypeLayer() != TYPE_OPENGL) return nullptr;
    auto* layerGL = static_cast<LayerElementGL*>(layer);
    GLTexture* texture = layerGL->GetOpenGLTexture();
    if (texture != nullptr && LayerRendering::IsLayerTextureCurrent(layer, width, height))
        return texture;

    cv::Mat prepared = scene.SceneLayer(layer, width, height, true);
    if (prepared.empty()) return nullptr;

    auto* newTexture = new GLTexture();
    if (!scene.SceneUpload(*newTexture, prepared)) {
        delete newTexture;
        return nullptr;
    }
    layerGL->SetOpenGLTexture(newTexture);  // détruit l'ancienne texture (contexte courant)
    LayerRendering::MarkLayerTextureCurrent(layer, width, height);
    return newTexture;
}

//-----------------------------------------------------------------
// Texture GPU du calque actif : résultat des filtres/preview (filtreEffet).
// Rechargée uniquement quand filtreEffet a régénéré son image ou que la taille
// change.
//-----------------------------------------------------------------
GLTexture* CLayerRendererGL::GetActiveLayerTexture(ILayerScene& scene, int width,
    int height, bool activeDirty)
{
    const cv::Size size(width, height);
    if (activeLayerTexture && !activeDirty && activeLayerTextureSize == size)
        return activeLayerTexture.get();

    cv::Mat prepared = scene.SceneActiveLayer(width, height);
    if (prepared.empty()) return nullptr;

    if (!activeLayerTexture || activeLayerTextureSize != size) {
        activeLayerTexture = std::make_unique<GLTexture>();
        activeLayerTextureSize = size;
    }
    if (!scene.SceneUpload(*activeLayerTexture, prepared)) {
        activeLayerTexture.reset();
        return nullptr;
    }
    return activeLayerTexture.get();
}

bool CLayerRendererGL::Compose(ILayerScene& scene, GLTexture* textureDisplay,
    int width, int height, bool activeDirty)
{
    if (!scene.SceneHasRenderContext() || width <= 0 || height <= 0) return false;
    if (!compositor)
        compositor = std::make_unique<CLayerCompositorGL>();
    if (!compositor->IsValid()) return false;

    const cv::Size size(width, height);
    if (!backgroundTexture || backgroundTextureSize != size) {
        auto texture = std::make_unique<GLTexture>();
        if (!scene.SceneUpload(*texture, scene.SceneBackground(width, height)))
            return false;
        backgroundTexture = std::move(texture);
        backgroundTextureSize = size;
    }

    std::vector<LayerPass> passes;
    passes.reserve(scene.SceneLayers()->size() + 1);

    const int activeIndex = scene.SceneActiveIndex();
    LayerRendering::ForEachVisibleLayer(scene.SceneLayers(),
        [&](size_t index, LayerElement* layer, float opacity) {
            GLTexture* texture = (static_cast<int>(index) == activeIndex)
                ? GetActiveLayerTexture(scene, width, height, activeDirty)
                : GetLayerTexture(scene, layer, width, height);
            if (texture != nullptr)
                passes.push_back(LayerPass{ texture, opacity, static_cast<int>(layer->blendMode) });
        });

    // Collage volant : canvas transparent global + bloc redimensionné
    if (scene.SceneHasFloating())
    {
        cv::Mat canvas = LayerRendering::BuildFloatingCanvas<cv::Mat>(
            scene.SceneFloatingMat(), scene.SceneRatio(),
            scene.SceneFloatingPosition(), width, height);
        if (!canvas.empty())
        {
            if (!floatingTexture)
                floatingTexture = std::make_unique<GLTexture>();
            scene.SceneUpload(*floatingTexture, canvas);
            passes.push_back(LayerPass{ floatingTexture.get(), 1.0f,
                static_cast<int>(LayerBlendMode::Normal) });
        }
    }

    return compositor->Compose(backgroundTexture.get(), passes, textureDisplay, width, height);
}
