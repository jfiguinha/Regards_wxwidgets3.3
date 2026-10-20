// ==================== LayerRenderer.h ====================
// Interface commune des 3 modes d'affichage des calques :
//   - CLayerRendererCPU    : composition CPU (cv::Mat), rendu final via OpenGL
//   - CLayerRendererGL     : composition par shaders OpenGL
//   - CLayerRendererOpenCL : composition OpenCL (cv::UMat), rendu final via OpenGL
#pragma once
#include "LayerElement.h"
#include <LayerFonction.h>
#include <opencv2/core.hpp>
#include <opencv2/imgproc.hpp>

namespace Regards { namespace OpenGL { class GLTexture; } }

// ============================================================================
// Source de scène : ce que les renderers demandent à l'éditeur.
// Les renderers ne dépendent donc plus de CBitmapEditor.
// ============================================================================
class ILayerScene
{
public:
    virtual ~ILayerScene() = default;

    // --- Calques ---
    virtual CLayerList * SceneLayers() = 0;
    virtual int SceneActiveIndex() const = 0;

    // --- Préparation des images (taille de sortie, offset et ratio inclus) ---
    virtual cv::Mat SceneBackground(int width, int height) = 0;
    virtual cv::Mat SceneActiveLayer(int width, int height) = 0;
    virtual cv::UMat SceneActiveLayerOpenCL(int width, int height) = 0;
    // useGLContext : true -> filtre temporaire avec le contexte OpenCL de RenderOpenGL,
    //                false -> filtre CPU pur (repli CPU)
    virtual cv::Mat SceneLayer(LayerElement* layer, int width, int height, bool useGLContext) = 0;
    virtual cv::UMat SceneLayerOpenCL(LayerElement* layer, int width, int height) = 0;

    // --- Collage « volant » (dataCopy) ---
    virtual bool SceneHasFloating() = 0;
    virtual cv::Mat SceneFloatingMat() = 0;
    virtual cv::UMat SceneFloatingUMat() = 0;
    virtual float SceneRatio() const = 0;
    // Position du collage en pixels d'écran (copyX/copyY * ratio * scale_factor)
    virtual cv::Point SceneFloatingPosition() const = 0;

    // --- OpenGL ---
    virtual bool SceneHasRenderContext() const = 0;
    virtual bool SceneUpload(Regards::OpenGL::GLTexture& texture, const cv::Mat& image) = 0;
};

// ============================================================================
// Briques partagées par les trois renderers
// ============================================================================
namespace LayerRendering
{
    // Parcourt les calques dans l'ordre (index 0 -> dernier) en sautant les calques
    // nuls, invisibles ou d'opacité nulle. Opacité normalisée dans [0..1].
    template <typename LayerList, typename Visitor>
    void ForEachVisibleLayer(LayerList * layers, Visitor&& visit)
    {
        const size_t count = static_cast<size_t>(layers->size());
        for (size_t index = 0; index < count; index++) {

            LayerElement* layer = (*layers)[static_cast<size_t>(index)];

            if (layer == nullptr || !layer->isVisible) continue;

            const float opacity =
                LayerFonction::Clamp01(static_cast<float>(layer->opacity) / 100.0f);
            if (opacity <= 0.0f) continue;

            visit(index, layer, opacity);
        }
    }

    // Le cache GPU/UMat du calque est-il valide pour cette taille de sortie ?
    inline bool IsLayerTextureCurrent(const LayerElement* layer, int width, int height)
    {
        return !layer->needTextureUpdate && layer->textureWidth == width &&
            layer->textureHeight == height;
    }

    inline void MarkLayerTextureCurrent(LayerElement* layer, int width, int height)
    {
        layer->needTextureUpdate = false;
        layer->textureWidth = width;
        layer->textureHeight = height;
    }

    // Canvas transparent width x height contenant le collage volant redimensionné
    // (ratio de zoom) à la position 'position'. M = cv::Mat ou cv::UMat.
    // Retourne une matrice vide si la taille cible est nulle.
    template <typename M>
    M BuildFloatingCanvas(const M& source, float ratio, const cv::Point& position,
        int width, int height)
    {
        const int targetW = static_cast<int>(static_cast<float>(source.cols) * ratio);
        const int targetH = static_cast<int>(static_cast<float>(source.rows) * ratio);
        if (targetW <= 0 || targetH <= 0) return M();

        M canvas = M::zeros(height, width, CV_8UC4);

        M scaled;
        cv::resize(source, scaled, cv::Size(targetW, targetH), 0.0, 0.0, cv::INTER_LINEAR);
        if (scaled.channels() == 3)
            cv::cvtColor(scaled, scaled, cv::COLOR_BGR2BGRA);

        const cv::Rect dstROI =
            cv::Rect(position.x, position.y, scaled.cols, scaled.rows) &
            cv::Rect(0, 0, width, height);
        if (!dstROI.empty()) {
            const cv::Rect srcROI(dstROI.x - position.x, dstROI.y - position.y,
                dstROI.width, dstROI.height);
            scaled(srcROI).copyTo(canvas(dstROI));
        }
        return canvas;
    }
}

// ============================================================================
// Classe de base
// ============================================================================
class CLayerRenderer
{
public:
    CLayerRenderer() = default;
    virtual ~CLayerRenderer() = default;

    CLayerRenderer(const CLayerRenderer&) = delete;
    CLayerRenderer& operator=(const CLayerRenderer&) = delete;

    // Compose tous les calques visibles dans textureDisplay (width x height).
    // Retourne false si ce backend est indisponible ou en erreur (l'appelant
    // décide du repli).
    virtual bool Compose(ILayerScene& scene, Regards::OpenGL::GLTexture* textureDisplay,
        int width, int height, bool activeDirty) = 0;

    // Libère les ressources GPU (à appeler contexte GL courant).
    virtual void Release() {}
};
