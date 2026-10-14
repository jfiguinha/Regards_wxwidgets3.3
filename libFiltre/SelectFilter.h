#pragma once
#include <FiltreEffectInterface.h>
#include "CropFilter.h"
#include "FilterWindowParam.h"
#include <TreeElementValue.h>

class CEffectParameter;

class CFiltreEffet;
class CImageLoadingFormat;
class IBitmapDisplay;

class CMetadata;

namespace Regards {
    namespace Filter {

        class CSelectFilter : public CCropFilter
        {
        public:
            CSelectFilter();
            virtual ~CSelectFilter();

            CEffectParameter* GetEffectPointer();
            CDraw* GetDrawingPt();
            int GetTypeFilter();
            int GetNameFilter();
            wxString GetFilterLabel();

            void Filter(CEffectParameter* effectParameter, cv::Mat& source, const wxString& filename, IFiltreEffectInterface* filtreInterface);
            void FilterChangeParam(CEffectParameter* effectParameter, Regards::Window::CTreeElementValue* valueData, const wxString& key);
            void RenderEffect(CFiltreEffet* filtreEffet, CEffectParameter* effectParameter, const bool& preview);
            CImageLoadingFormat* ApplyEffect(CEffectParameter* effectParameter, IBitmapDisplay* bitmapViewer);

            // MODIFICATION : Utilisation de cv::Mat& à la place de wxMemoryDC*
            void Drawing(cv::Mat& matrix, IBitmapDisplay* bitmapViewer, CDraw* m_cDessin);

        private:
            wxColour ConvertScalarToWxColour(const cv::Scalar& scalar_color, int alpha);
            void AddMetadataElement(std::vector<CMetadata>& element, wxString value, int key);

            cv::Mat source;
            wxString filename;
            wxString libelleSelectType;
            wxString libelleTolerance;
        };

    }
}
