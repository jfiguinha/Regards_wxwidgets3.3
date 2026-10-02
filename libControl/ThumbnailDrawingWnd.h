//
//  ThumbnailViewerEffectWnd.hpp
//  Regards.libViewer
//
//  Created by figuinha jacques on 02/10/2015.
//  Copyright © 2015 figuinha jacques. All rights reserved.
//

#pragma once
#include "ThumbnailDrawing.h"
#include <ScrollbarWnd.h>
using namespace Regards::Control;

namespace Regards::Control
{
	class CThumbnailDrawingWnd : public CWindowMain
	{
	public:
		CThumbnailDrawingWnd(wxWindow* parent, wxWindowID idCTreeWithScrollbarInterface,
		                          const CThemeScrollBar& themeScroll, const CThemeThumbnail& themeThumbnail,
		                          int panelInfosId, bool checkValidity);
		~CThumbnailDrawingWnd(void) = default;

		void UpdateScreenRatio() override;
		void Resize() override;


	private:
		CScrollbarWnd * thumbnailDrawingScroll;
		CThumbnailDrawing * thumbnailDrawing;
	};
}
