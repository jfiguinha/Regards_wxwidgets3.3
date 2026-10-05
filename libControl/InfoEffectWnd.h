//
//  InfoEffectWnd.hpp
//  Regards.libViewer
//
//  Created by figuinha jacques on 02/10/2015.
//  Copyright © 2015 figuinha jacques. All rights reserved.
//

#pragma once
#include "InfoEffect.h"
#include <TreeWithScrollbar.h>
using namespace Regards::Window;

class CImageLoadingFormat;

namespace Regards::Control
{
	class CInfoEffectWnd : public CTreeWithScrollbar
	{
	public:
		CInfoEffectWnd(wxWindow* parent, wxWindowID id, const CThemeScrollBar& themeScroll,
		               const CThemeTree& themeTree, int bitmap_window_id, bool eraseMemory = true);
		~CInfoEffectWnd(void);

		CTreeElementControlInterface * GetTreeInterface();

		void SetHistoryEffect(CInfoEffect* infoEffect);

		void AddModification(const int& numEffect, CEffectParameter* effectParameter, const wxString& libelle);
		void HistoryUpdate(CImageLoadingFormat* bitmap, const wxString& filename, const wxString& historyLibelle,
		                   CModificationManager* modificationManager);

	private:
		CInfoEffect * historyEffectOld;
		int bitmapWindowId;
		bool eraseMemory = true;
		//const wxWindowID id_;
		//const CThemeScrollBar& theme_scroll_;
	};
}
