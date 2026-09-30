#pragma once
#include "CropFilter.h"
#include "FilterWindowParam.h"

class CMetadata;
struct SShapeTrace;

namespace Regards::Filter
{
	class CRectangleFilter : public CCropFilter
	{
	public:
		CRectangleFilter();
		~CRectangleFilter() override;

		int GetTypeFilter() override;
		int GetNameFilter() override;
		wxString GetFilterLabel() override;
		CImageLoadingFormat* ApplyEffect(CEffectParameter* effectParameter, IBitmapDisplay* bitmapViewer) override;
		void Filter(CEffectParameter* effectParameter, cv::Mat& source, const wxString& filename,
			IFiltreEffectInterface* filtreInterface)  override;
		void FilterChangeParam(CEffectParameter* effectParameter, CTreeElementValue* valueData, const wxString& key);
		CEffectParameter* GetEffectPointer() override;
		void RenderEffect(CFiltreEffet* filtreEffet, CEffectParameter* effectParameter, const bool& preview) override;
		void Drawing(cv::Mat& matrix, IBitmapDisplay* bitmapViewer, CDraw* m_cDessin) override;
		CDraw* GetDrawingPt() override;

	private:
		wxColour ConvertScalarToWxColour(const cv::Scalar& scalar_color, int alpha);
		void AddMetadataElement(vector<CMetadata>& element, wxString value, int key);
		void DrawShapeOnMat(cv::Mat& matrix, SShapeTrace& shape);

		wxString libelleShapeType;
		wxString libellePenSize;
		wxString libelleColor;
		wxString libelleTypeBrush;
		wxString libelleOpacity;
		wxString libelleIsFilled;
	};
}
