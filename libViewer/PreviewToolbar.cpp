#include <header.h>
#include "PreviewToolbar.h"
#include <LibResource.h>
#include "GeneralToolbar.h"
#include "PreviewBitmapToolbar.h"
#include "PreviewVideoToolbar.h"
#include "MainWindow.h"
#include <window_id.h>
using namespace Regards::Window;
using namespace Regards::Viewer;


CPreviewToolbar::CPreviewToolbar(wxWindow* parent, wxWindowID id, const CThemeToolbar& theme,
                                 CToolbarInterface* toolbarInterface, const bool& vertical,
                                 wxWindowID viewerId, CSliderInterface* videoInterface)
	: CWindowMain("CPreviewToolbar", parent, id)
{
	generalToolbar = new CGeneralToolbar(this, id, theme, toolbarInterface, vertical);
	// wxID_ANY pour les deux autres : évite les doublons d'id avec FindWindowById(id)
	bitmapToolbar = new CPreviewBitmapToolbar(this, wxID_ANY, viewerId, theme, toolbarInterface, vertical);
	videoToolbar = new CPreviewVideoToolbar(this, wxID_ANY, theme, toolbarInterface, videoInterface, vertical);

	SetMediaToolbar(MediaMode::General);
}

std::array<CGeneralToolbar*, 3> CPreviewToolbar::AllToolbars() const
{
	return {generalToolbar, bitmapToolbar, videoToolbar};
}

void CPreviewToolbar::EnableScreenButton()
{

}

int CPreviewToolbar::GetHeight()
{
	if (activeToolbar != nullptr)
		return activeToolbar->GetHeight();
	return 0;
}

void CPreviewToolbar::SetMediaToolbar(MediaMode mediaMode)
{
	if (mediaMode != this->mediaMode)
	{
		this->mediaMode = mediaMode;
		activeToolbar = generalToolbar;
		/*
		switch (mediaMode)
		{
		case MediaMode::Bitmap:
		case MediaMode::Animation:
			activeToolbar = bitmapToolbar;
			break;
		case MediaMode::Video:
			activeToolbar = videoToolbar;
			break;
		default:
			activeToolbar = generalToolbar;
			break;
		}
		*/
		for (CGeneralToolbar* toolbar : AllToolbars())
		{
			if (toolbar != nullptr)
				toolbar->Show(toolbar == activeToolbar);
		}

		Resize();
	}

}

void CPreviewToolbar::UpdateScreenRatio()
{
	for (CGeneralToolbar* toolbar : AllToolbars())
	{
		if (toolbar != nullptr)
			toolbar->UpdateScreenRatio();
	}
	needToRefresh = true;
}

void CPreviewToolbar::Resize()
{
	if (activeToolbar != nullptr)
	{
		activeToolbar->SetSize(0, 0, GetWindowWidth(), GetWindowHeight());
		needToRefresh = true;
	}
}

void CPreviewToolbar::DisableScreenButton()
{

}

// Les états Enable/Disable sont appliqués aux trois barres pour rester cohérents
// quand le mode média change.
void CPreviewToolbar::EnableExportButton()
{
	for (CGeneralToolbar* toolbar : AllToolbars())
		toolbar->EnableExportButton();
}


void CPreviewToolbar::DisableExportButton()
{
	for (CGeneralToolbar* toolbar : AllToolbars())
		toolbar->DisableExportButton();
}

void CPreviewToolbar::EnableSaveButton()
{
	for (CGeneralToolbar* toolbar : AllToolbars())
		toolbar->EnableSaveButton();
}


void CPreviewToolbar::DisableSaveButton()
{
	for (CGeneralToolbar* toolbar : AllToolbars())
		toolbar->DisableSaveButton();
}

void CPreviewToolbar::DisableNavigationButton()
{
	for (CGeneralToolbar* toolbar : AllToolbars())
		toolbar->DisableNavigationButton();
}

void CPreviewToolbar::EnableNavigationButton()
{
	for (CGeneralToolbar* toolbar : AllToolbars())
		toolbar->EnableNavigationButton();
}

void CPreviewToolbar::SetFullscreen()
{
	for (CGeneralToolbar* toolbar : AllToolbars())
		toolbar->SetFullscreen();
}

void CPreviewToolbar::SetScreen()
{
	for (CGeneralToolbar* toolbar : AllToolbars())
		toolbar->SetScreen();
}

void CPreviewToolbar::SetTabValue(const std::vector<int>& value)
{
	bitmapToolbar->SetTabValue(value);
}

void CPreviewToolbar::SetTrackBarPosition(const int& position)
{
	bitmapToolbar->SetTrackBarPosition(position);
}

void CPreviewToolbar::SetVideoPlay()
{
	videoToolbar->SetPlay();
}

void CPreviewToolbar::SetVideoPause()
{
	videoToolbar->SetPause();
}
