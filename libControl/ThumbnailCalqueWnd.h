//
//  ThumbnailViewerEffectWnd.hpp
//  Regards.libViewer
//
//  Created by figuinha jacques on 02/10/2015.
//  Copyright © 2015 figuinha jacques. All rights reserved.
//

#pragma once
#include "ThumbnailCalque.h"
#include <ScrollbarWnd.h>
using namespace Regards::Control;

namespace Regards::Control
{
	class CThumbnailCalqueWnd : public CWindowMain
	{
	public:
		CThumbnailCalqueWnd(wxWindow* parent, wxWindowID idCTreeWithScrollbarInterface,
		                     const CThemeScrollBar& themeScroll, const CThemeThumbnail& themeThumbnail,
		                     int panelInfosId, bool checkValidity);
		~CThumbnailCalqueWnd(void) = default;

		void SetLayer(CLayerList * listOfLayer);
		void UpdateScreenRatio() override;
		void Resize() override;
		//wxString GetFilename();
		//void SetFile(const wxString& filename);
		int GetSelectLayer();
		void RefreshList();

	private:
		CScrollbarWnd * thumbnailEffectScroll;
		CThumbnailCalque * thumbnailCalque;
	};
}
