// ==================== LayerCompositorOpenCL.h ====================
#pragma once
#include "LayerCompositor.h"
#include <OpenCLContext.h>

using namespace Regards::OpenCL;

class CLayerCompositorOpenCL : public CLayerCompositor
{
public:
    CLayerCompositorOpenCL(COpenCLContext* openCLContext);
    virtual ~CLayerCompositorOpenCL() override;

    // Vérifie si OpenCL est disponible et actif sur la machine
    virtual bool IsValid() override { return cv::ocl::useOpenCL(); }
    virtual void Release() override;

    bool Compose(const cv::UMat& background, const std::vector<LayerPassOpenCL>& layers,
        cv::UMat& target, int width, int height);

private:
    COpenCLContext* openCLContext = nullptr;
};
