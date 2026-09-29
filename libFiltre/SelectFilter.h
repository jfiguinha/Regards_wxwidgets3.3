#pragma once
#include "CropFilter.h"
#include "FilterWindowParam.h"

class CMetadata;
struct SSelectionTrace;

namespace Regards::Filter
{
	class CSelectFilter : public CCropFilter
	{
	public:
		CSelectFilter();
		~CSelectFilter() override;

		int GetTypeFilter() override;
		int GetNameFilter() override;
		wxString GetFilterLabel() override;

		CImageLoadingFormat* ApplyEffect(CEffectParameter* effectParameter, IBitmapDisplay* bitmapViewer) override;
		void Filter(CEffectParameter* effectParameter, cv::Mat& source, const wxString& filename,
			IFiltreEffectInterface* filtreInterface) override;
		void FilterChangeParam(CEffectParameter* effectParameter, CTreeElementValue* valueData, const wxString& key);
		CEffectParameter* GetEffectPointer() override;
		void RenderEffect(CFiltreEffet* filtreEffet, CEffectParameter* effectParameter, const bool& preview) override;

		// Surcharges pour la capture et le rendu à l'écran (wxWidgets) et OpenCV
		void Drawing(wxMemoryDC* dc, IBitmapDisplay* bitmapViewer, CDraw* m_cDessin) override;
		void Drawing(cv::Mat& matrix, IBitmapDisplay* bitmapViewer, CDraw* m_cDessin) override;
		CDraw* GetDrawingPt() override;

	private:
		wxColour ConvertScalarToWxColour(const cv::Scalar& scalar_color, int alpha);
		void AddMetadataElement(vector<CMetadata>& element, wxString value, int key);
		void DrawSelectionOnMat(cv::Mat& matrix, SSelectionTrace& selection);

		wxString libelleSelectType;
		wxString libelleTolerance;

	};
}
