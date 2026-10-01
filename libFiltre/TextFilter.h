#pragma once
#include "CropFilter.h"
#include "FilterWindowParam.h"

class CMetadata;

namespace Regards::Filter {

    class CTextFilterParameter;

    class CTextFilter : public CCropFilter {
    public:
        CTextFilter();
        ~CTextFilter() override = default;

        int GetTypeFilter() override;
        int GetNameFilter() override;
        wxString GetFilterLabel() override;

        CImageLoadingFormat* ApplyEffect(CEffectParameter* effectParameter, IBitmapDisplay* bitmapViewer) override;
        void Filter(CEffectParameter* effectParameter, cv::Mat& source, const wxString& filename, IFiltreEffectInterface* filtreInterface) override;
        void FilterChangeParam(CEffectParameter* effectParameter, CTreeElementValue* valueData, const wxString& key) override;

        CEffectParameter* GetEffectPointer() override;
        void RenderEffect(CFiltreEffet* filtreEffet, CEffectParameter* effectParameter, const bool& preview) override;
        void Drawing(cv::Mat& matrix, IBitmapDisplay* bitmapViewer, CDraw* m_cDessin) override;
        CDraw* GetDrawingPt() override;
        void Drawing(wxMemoryDC* dc, IBitmapDisplay* bitmapViewer, CDraw* m_cDessin) override;
    private:
        void AddMetadataElement(std::vector<CMetadata>& element, wxString value, int key);
        void AppliquerRenduTexte(cv::Mat& matrix, CTextFilterParameter* param);

        wxArrayString m_systemFonts; // <-- Stocke la liste des noms de polices
        wxString lblFontFace;
        wxString lblFontSize;
        wxString lblColor;
        wxString lblBold;
        wxString lblItalic;
        wxString lblOpacity;
    };
}
