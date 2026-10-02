#include <header.h>
//
//  ThumbnailViewerEffectWnd.cpp
//  Regards.libViewer
//
//  Created by figuinha jacques on 02/10/2015.
//  Copyright © 2015 figuinha jacques. All rights reserved.

#include "ThumbnailDrawingWnd.h"
#include <libPicture.h>
using namespace Regards::Control;
using namespace Regards::Picture;

CThumbnailDrawingWnd::CThumbnailDrawingWnd(wxWindow* parent, wxWindowID id,
                                                     const CThemeScrollBar& themeScroll,
                                                     const CThemeThumbnail& themeThumbnail, int panelInfosId,
                                                     bool checkValidity)
	: CWindowMain("CThumbnailDrawingWnd", parent, id)
{
	thumbnailDrawingScroll = nullptr;
	thumbnailDrawing = nullptr;

	thumbnailDrawing = new CThumbnailDrawing(this, wxID_ANY, themeThumbnail, checkValidity, panelInfosId);
	thumbnailDrawingScroll = new CScrollbarWnd(this, thumbnailDrawing, wxID_ANY);

	thumbnailDrawing->Init();
}

void CThumbnailDrawingWnd::UpdateScreenRatio()
{
	if (thumbnailDrawingScroll != nullptr)
		thumbnailDrawingScroll->UpdateScreenRatio();

	if (thumbnailDrawing != nullptr)
		thumbnailDrawing->UpdateScreenRatio();
}

void CThumbnailDrawingWnd::Resize()
{
	if (thumbnailDrawingScroll != nullptr)
		thumbnailDrawingScroll->SetSize(0, 0, GetWindowWidth(), GetWindowHeight());
	thumbnailDrawing->UpdateScroll();
}


