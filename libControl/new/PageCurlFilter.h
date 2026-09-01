#pragma once

#include <memory>
#include "BitmapFusionFilter.h"
#include "PageCurlMesh.h"

class CImageLoadingFormat;
class IBitmapDisplay;

namespace Regards::Filter {
    class CPageCurlFilter : public CBitmapFusionFilter {
    public:
        CPageCurlFilter();
        ~CPageCurlFilter() override;

        void RenderTexture(CRenderBitmapOpenGL * renderOpenGL, const float& time,
            const float& invert, const int& width, const int& height,
            const int& left, const int& top);

        int GetTypeFilter() override;

        void SetTransitionBitmap(const bool& start, IBitmapDisplay* bmpViewer,
            CImageLoadingFormat* bmpSecond);

        bool RenderTexture(CImageLoadingFormat* nextPicture,
            CImageLoadingFormat* source, IBitmapDisplay* bmpViewer,
            CRenderBitmapOpenGL* renderOpenGL,
            const float& scale_factor, const int& etape);

        GLTexture* GetTexture(const int& numTexture);

    private:
        void GenerateTexture(CImageLoadingFormat* nextPicture,
            CImageLoadingFormat* source, IBitmapDisplay* bmpViewer);

    private:
        bool initTexture = true;

        CPageCurlMesh pageCurlMesh;

        bool meshInitialized = false;

        int meshWidth = 0;
        int meshHeight = 0;
    };

}  // namespace Regards::Filter
