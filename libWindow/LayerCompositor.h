// ==================== LayerCompositor.h ====================
#pragma once

// Forward declaration pour la version OpenGL
namespace Regards { namespace OpenGL { class GLTexture; } }

// Structure intermédiaire neutre pour passer les paramètres d'un calque
template <typename TextureType>
struct GenericLayerPass
{
    TextureType texture;
    float opacity = 1.0f;
    int blendMode = 0;
};

// Alias pratiques pour les structures de passes
using LayerPass = GenericLayerPass<Regards::OpenGL::GLTexture*>;
using LayerPassOpenCL = GenericLayerPass<cv::UMat>;

class CLayerCompositor
{
public:
    CLayerCompositor() = default;
    virtual ~CLayerCompositor() = default;

    // Supprime la copie
    CLayerCompositor(const CLayerCompositor&) = delete;
    CLayerCompositor& operator=(const CLayerCompositor&) = delete;

    virtual bool IsValid() = 0;
    virtual void Release() = 0;
};
