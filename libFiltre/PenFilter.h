#pragma once
#include "CropFilter.h"
//
//  BrightAndContrastFilter.hpp
//  Regards.libViewer
//
//  Created by figuinha jacques on 12/04/2016.
//  Copyright © 2016 figuinha jacques. All rights reserved.
//

#include "FilterWindowParam.h"
class CMetadata;

namespace Regards::Filter
{
	class CPenFilter : public CCropFilter
	{
	public:
		CPenFilter();
		~CPenFilter() override;

		int GetTypeFilter() override;
		int GetNameFilter() override;
		wxString GetFilterLabel() override;
		CImageLoadingFormat* ApplyEffect(CEffectParameter* effectParameter, IBitmapDisplay* bitmapViewer) override;
		void Filter(CEffectParameter* effectParameter, cv::Mat& source, const wxString& filename,
			IFiltreEffectInterface* filtreInterface)  override;
		void FilterChangeParam(CEffectParameter* effectParameter, CTreeElementValue* valueData,
			const wxString& key);
		CEffectParameter* GetEffectPointer() override;
		void RenderEffect(CFiltreEffet* filtreEffet, CEffectParameter* effectParameter,
			const bool& preview) override;

		void Drawing(cv::Mat& matrix, IBitmapDisplay* bitmapViewer, CDraw* m_cDessin) override;
		CDraw* GetDrawingPt() override;
	private:
		wxColour ConvertScalarToWxColour(const cv::Scalar& scalar_color, int alpha);
		void AddMetadataElement(vector<CMetadata>& element, wxString value, int key);
		wxString libellePenSize;
		wxString libelleColor;
		wxString libelleTypeBrush;
		wxString libelleOpacity;

		cv::Mat matPreview;    // Matrice de travail persistante (ultra-rapide)
		wxPoint ptPrecedent;   // Mémorise la position précédente pour le tracé en cours
		bool m_isDrawingActive = false;
	};
}
