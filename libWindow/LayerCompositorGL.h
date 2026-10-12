// ==================== LayerCompositorGL.h ====================
#pragma once
#include "LayerCompositor.h"
#include "GLSLShader.h"

class CLayerCompositorGL : public CLayerCompositor
{
public:
    CLayerCompositorGL();
    virtual ~CLayerCompositorGL() override;

    virtual bool IsValid() override { return shader.IsOk(); }
    virtual void Release() override;

    bool Compose(Regards::OpenGL::GLTexture* background, const std::vector<LayerPass>& layers,
        Regards::OpenGL::GLTexture* target, int width, int height);

private:
    bool BuildProgram();
    bool EnsureTemporary(int width, int height);

    Regards::OpenGL::GLSLShader shader;
    unsigned int vao = 0;
    unsigned int fboTarget = 0;
    unsigned int fboTemp = 0;
    unsigned int tempTexture = 0;
    int tempWidth = 0;
    int tempHeight = 0;

    int locSrc = -1;
    int locDst = -1;
    int locOpacity = -1;
    int locMode = -1;
    int locCopy = -1;
};
