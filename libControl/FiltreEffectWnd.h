//
//  FiltreEffectWnd.h
//  Regards.libViewer
//
//  Created by figuinha jacques on 02/10/2015.
//  Copyright © 2015 figuinha jacques. All rights reserved.
//

#pragma once
#include "FiltreEffect.h"
#include <TreeWithScrollbar.h>
#include <ImageLoadingFormat.h>
#include "LayerElement.h" // Nécessaire pour manipuler CLayerList
using namespace Regards::Window;


namespace Regards::Control
{
	class CInfoEffectWnd;

	class CFiltreEffectScrollWnd : public CTreeWithScrollbar
	{
	public:
		CFiltreEffectScrollWnd(wxWindow* parent, wxWindowID id, const CThemeScrollBar& themeScroll,
			const CThemeTree& themeTree, int bitmapWindowId);
		~CFiltreEffectScrollWnd(void) = default;

		void ApplyEffect(const int& numItem, CInfoEffectWnd* historyEffectWnd, const wxString& filename,
			const int& isVideo, int panelId, int previewId);

		// Mise à jour de la signature pour accepter la liste des calques actuelle (currentLayers)
		void OnFiltreOk(const int& numFiltre, CInfoEffectWnd* historyEffectWnd, CLayerList * currentLayers, int numLayer = -1);

		void OnFiltreCancel();
		CFiltreEffect* GetFiltreEffect();
		int GetNumFiltre();
		void SetNumLayerId(const int& layerId);
		void SetParentBitmapId(const int& bitmapWindowId);
		void SetColor(const wxColour& color1, const wxColour& color2);

	private:
		void OnUpdateFilter(wxCommandEvent& event);
		void SetBitmapToViewer(CImageLoadingFormat* bitmap);

		std::unique_ptr<CImageLoadingFormat> bitmap;
		CEffectParameter* effectParameter;
		std::unique_ptr<CFiltreEffect> filtreEffectOld;
		int numFiltre;
		int layerId = 0;
		int numLayer = 0;
		int bitmapWindowId;
	};
}