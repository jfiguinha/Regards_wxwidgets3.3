//
//  InfoEffectWnd.cpp
//  Regards.libViewer
//
//  Created by figuinha jacques on 02/10/2015.
//  Copyright © 2015 figuinha jacques. All rights reserved.
//
#include <header.h>
#include "InfoEffectWnd.h"
#include <TreeWindow.h>
#include <ImageLoadingFormat.h>
#include <TreeWindow.h>
#include <ScrollbarWnd.h>
using namespace Regards::Control;
using namespace Regards::Window;

CInfoEffectWnd::CInfoEffectWnd(wxWindow* parent, wxWindowID id, const CThemeScrollBar& themeScroll,
	const CThemeTree& themeTree, int bitmap_window_id, bool eraseMemory)
	: CTreeWithScrollbar("CInfoEffectWnd", parent, id, themeScroll, themeTree)
{
	this->bitmapWindowId = bitmap_window_id;
	this->eraseMemory = eraseMemory;
	historyEffectOld = nullptr;
}

CTreeElementControlInterface* CInfoEffectWnd::GetTreeInterface()
{
	return treeWindow;
}


CInfoEffectWnd::~CInfoEffectWnd(void)
{
	if (eraseMemory && historyEffectOld)
		delete historyEffectOld;
}

void CInfoEffectWnd::SetHistoryEffect(CInfoEffect* infoEffect)
{
	if (infoEffect != nullptr)
		historyEffectOld = infoEffect;
	eraseMemory = false;
	if (treeWindow)
	{
		CTreeElementControlInterface * treeInterface = static_cast<CTreeElementControlInterface*>(treeWindow);
		if (treeInterface)
			treeInterface->UpdateTreeControl();
	}

	this->Refresh();
}

void CInfoEffectWnd::AddModification(const int& numEffect, CEffectParameter* effectParameter, const wxString& libelle)
{
	if (historyEffectOld != nullptr)
		historyEffectOld->AddModification(numEffect, effectParameter, libelle);
}

void CInfoEffectWnd::HistoryUpdate(CImageLoadingFormat* bitmap, const wxString& filename,
                                   const wxString& historyLibelle,
                                   CModificationManager* modificationManager)
{
	if (historyEffectOld == nullptr || historyEffectOld->GetFilename() != filename)
	{
		auto historyEffect = new CInfoEffect(treeWindow, modificationManager, bitmapWindowId);
		historyEffect->Init(bitmap, filename, historyLibelle);
		treeWindow->SetTreeControl(historyEffect);

		if (historyEffectOld)
			delete historyEffectOld;
		historyEffectOld = historyEffect;
	}
}
