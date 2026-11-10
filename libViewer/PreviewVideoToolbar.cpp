#include <header.h>
#include "PreviewVideoToolbar.h"
#include <LibResource.h>
#include <window_id.h>
#include <SliderVideo.h>   // PLAYBUTTONID, PAUSEBUTTONID, REPEATID, SCREENFORMATID, VolumePopup

using namespace Regards::Viewer;

namespace
{
	// Identifiants locaux à cette barre (volontairement hors plage des WM_/IDM_ de CGeneralToolbar)
	constexpr int IDM_PV_REPEAT = 20011;
	constexpr int IDM_PV_VOLUME = 20012;
	constexpr int IDM_PV_ZOOM = 20013;
	constexpr int IDM_PV_SHRINK = 20014;

	// Mêmes valeurs que dans CSliderVideo::OnLButtonDown (27 paliers de zoom)
	constexpr int VIDEO_ZOOM_COUNT = 27;
	constexpr int VIDEO_VOLUME_MAX = 100;
}

CPreviewVideoToolbar::CPreviewVideoToolbar(wxWindow* parent, wxWindowID id, const CThemeToolbar& theme,
                                           CToolbarInterface* toolbarInterface, CSliderInterface* sliderEvent,
                                           const bool& vertical)
	: CGeneralToolbar(parent, id, theme, toolbarInterface, vertical), sliderEvent(sliderEvent)
{
	repeat = CreateButton("IDB_REFRESH", "LBLREPEAT", IDM_PV_REPEAT, false);
	volume = CreateButton("IDB_VOLUME_UP_VIDEO", "LBLVOLUME", IDM_PV_VOLUME, false);
	zoom = CreateButton("IDB_ZOOMPLUS", "LBLZOOMON", IDM_PV_ZOOM, false);
	shrink = CreateButton("IDB_SCREENPNG", "LBLSHRINK", IDM_PV_SHRINK, false);

	separatorNav = std::make_unique<CToolbarSeparator>();
	separatorMedia = std::make_unique<CToolbarSeparator>();

	// L'ordre de navElement = l'ordre d'affichage (cf. CToolbarWindow::RecalcLayout).
	navElement.clear();

	navElement.push_back(imageFirst.get());
	navElement.push_back(imagePrec.get());
	navElement.push_back(imagePlayDiaporama.get());
	navElement.push_back(imageStopDiaporama.get());
	navElement.push_back(imageSuiv.get());
	navElement.push_back(imageEnd.get());

	navElement.push_back(separatorNav.get());

	navElement.push_back(repeat.get());
	navElement.push_back(volume.get());
	navElement.push_back(zoom.get());
	navElement.push_back(shrink.get());

	navElement.push_back(separatorMedia.get());

	navElement.push_back(rotate270.get());
	navElement.push_back(rotate90.get());
	navElement.push_back(flipHorizontal.get());
	navElement.push_back(flipVertical.get());
	navElement.push_back(save.get());
	navElement.push_back(exportFile.get());
}

void CPreviewVideoToolbar::DisableNavigationButton()
{
	CGeneralToolbar::DisableNavigationButton();
	separatorNav->SetVisible(false);
	needToRefresh = true;
}

void CPreviewVideoToolbar::EnableNavigationButton()
{
	CGeneralToolbar::EnableNavigationButton();
	separatorNav->SetVisible(true);
	needToRefresh = true;
}

void CPreviewVideoToolbar::SetPlay()
{
	imagePlayDiaporama->SetVisible(false);
	imageStopDiaporama->SetVisible(true);
	needToRefresh = true;
}

void CPreviewVideoToolbar::SetPause()
{
	imagePlayDiaporama->SetVisible(true);
	imageStopDiaporama->SetVisible(false);
	needToRefresh = true;
}

void CPreviewVideoToolbar::ShowPopup(CToolbarButton* button, int value, int count, const wxString& key)
{
	if (sliderEvent == nullptr)
		return;

	Regards::UI::ModernTheme modernTheme;
	modernTheme.background = themeToolbar.colorTop;
	modernTheme.secondaryText = wxColour(220, 224, 230);

	const wxPoint screenPos = ClientToScreen(wxPoint(button->GetXPos(), button->GetYPos()));

	auto popup = new Regards::UI::CVolumePopupModern(this, value, modernTheme, count,
	                                                 [this, key](int newValue)
	                                                 {
		                                                 if (sliderEvent != nullptr)
			                                                 sliderEvent->SlidePosChange(newValue, key);
	                                                 });

	// Popup centré au-dessus du bouton, 4 px de marge (comme CSliderVideo)
	const int xOffset = screenPos.x + (button->GetWidth() / 2) - (popup->GetSize().x / 2);
	const int yOffset = screenPos.y - popup->GetSize().y - 4;

	popup->Position(wxPoint(xOffset, yOffset), wxSize(0, 0));
	popup->Popup();
}

void CPreviewVideoToolbar::EventManager(const int& id)
{
	switch (id)
	{
	// Le bouton play/stop de la base pilote ici la lecture vidéo, pas le diaporama
	case WM_DIAPORAMA_PLAY:
		if (sliderEvent != nullptr)
			sliderEvent->ClickButton(PLAYBUTTONID);
		break;

	case WM_DIAPORAMA_STOP:
		if (sliderEvent != nullptr)
			sliderEvent->ClickButton(PAUSEBUTTONID);
		break;

	case IDM_PV_REPEAT:
		if (sliderEvent != nullptr)
			sliderEvent->ClickButton(REPEATID);
		break;

	case IDM_PV_VOLUME:
		if (sliderEvent != nullptr)
			ShowPopup(volume.get(), sliderEvent->GetVolume(), VIDEO_VOLUME_MAX, L"VOLUME");
		break;

	case IDM_PV_ZOOM:
		if (sliderEvent != nullptr)
			ShowPopup(zoom.get(), sliderEvent->GetZoomIndex(), VIDEO_ZOOM_COUNT, L"ZOOM");
		break;

	case IDM_PV_SHRINK:
		if (sliderEvent != nullptr)
			sliderEvent->ClickButton(SCREENFORMATID);
		break;

	default:
		// navigation, rotations, flips, save, export : comportement de CGeneralToolbar
		CGeneralToolbar::EventManager(id);
		break;
	}
}
