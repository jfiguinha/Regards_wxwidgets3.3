#pragma once
#include "CropFilter.h"
#include "FilterWindowParam.h"

class CMetadata;

namespace Regards::Filter
{
    class CPaintBucketFilter : public CCropFilter
    {
    public:
        CPaintBucketFilter();
        ~CPaintBucketFilter() override;

        int GetTypeFilter() override;
        int GetNameFilter() override;
        wxString GetFilterLabel() override;

        CImageLoadingFormat* ApplyEffect(CEffectParameter* effectParameter, IBitmapDisplay* bitmapViewer) override;
        void Filter(CEffectParameter* effectParameter, cv::Mat& source, const wxString& filename, IFiltreEffectInterface* filtreInterface) override;
        void FilterChangeParam(CEffectParameter* effectParameter, CTreeElementValue* valueData, const wxString& key);
        CEffectParameter* GetEffectPointer() override;
        void RenderEffect(CFiltreEffet* filtreEffet, CEffectParameter* effectParameter, const bool& preview) override;

        void Drawing(cv::Mat& matrix, IBitmapDisplay* bitmapViewer, CDraw* m_cDessin) override;
        CDraw* GetDrawingPt() override;

    private:

        wxString libelleColor;
        wxString libelleTolerance;
    };
}
