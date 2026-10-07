#include <header.h>
//
//  ThumbnailViewerEffectWnd.cpp
//  Regards.libViewer
//
//  Created by figuinha jacques on 02/10/2015.
//  Copyright © 2015 figuinha jacques. All rights reserved.

#include "ThumbnailCalqueWnd.h"
#include <libPicture.h>
using namespace Regards::Control;
using namespace Regards::Picture;

CThumbnailCalqueWnd::CThumbnailCalqueWnd(wxWindow* parent, wxWindowID id,
                                                     const CThemeScrollBar& themeScroll,
                                                     const CThemeThumbnail& themeThumbnail, int panelInfosId,
                                                     bool checkValidity)
	: CWindowMain("CThumbnailCalqueWnd", parent, id)
{
	thumbnailEffectScroll = nullptr;
	thumbnailCalque = nullptr;

	thumbnailCalque = new CThumbnailCalque(this, wxID_ANY, themeThumbnail);
	thumbnailEffectScroll = new CScrollbarWnd(this, thumbnailCalque, wxID_ANY);
}

void CThumbnailCalqueWnd::SetLayer(CLayerList * listOfLayer)
{
	if (thumbnailCalque != nullptr)
		thumbnailCalque->SetLayer(listOfLayer);
}

void CThumbnailCalqueWnd::UpdateScreenRatio()
{
	if (thumbnailEffectScroll != nullptr)
		thumbnailEffectScroll->UpdateScreenRatio();

	if (thumbnailCalque != nullptr)
		thumbnailCalque->UpdateScreenRatio();
}

void CThumbnailCalqueWnd::Resize()
{
	if (thumbnailEffectScroll != nullptr)
		thumbnailEffectScroll->SetSize(0, 0, GetWindowWidth(), GetWindowHeight());
	thumbnailCalque->Resize();
}

wxString CThumbnailCalqueWnd::GetFilename()
{
	if (thumbnailCalque != nullptr)
		return thumbnailCalque->GetFilename();
	return "";
}

void CThumbnailCalqueWnd::SetFile(const wxString& filename)
{
	if (thumbnailCalque != nullptr)
	{
		CLibPicture libPicture;
		CImageLoadingFormat* load = libPicture.LoadThumbnail(filename);
		thumbnailCalque->SetFile(filename, load);
		delete load;
	}
}
