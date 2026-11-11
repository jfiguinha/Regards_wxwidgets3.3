#include <header.h>
#include "SliderVideoToolbar.h"
#include "SliderVideo.h" // Pour les IDs : PLAYBUTTONID, PAUSEBUTTONID, etc.
#include <ConvertUtility.h>
#include <vector>

using namespace Regards::Video;

CSliderVideoToolbar::CSliderVideoToolbar(wxWindow* parent, wxWindowID id, CSliderInterface* sliderEvent,
	const CThemeToolbar& theme, const bool& vertical)
	: CToolbarWindow(parent, id, theme, vertical)
{
	this->sliderEvent = sliderEvent;
	themeToolbar = theme;

	// Création des boutons de la barre vidéo
	btnPlay = CreateButton("IDB_PLAY_VIDEO", "LBLPLAY", PLAYBUTTONID, false);
	btnPause = CreateButton("IDB_PAUSE_VIDEO", "LBLPAUSE", PAUSEBUTTONID, false);

	videoSlide = std::make_unique<CToolbarSliderVideo>(themeToolbar.slider, this);
	videoSlide->SetLabels(L"00:00:00", L"00:00:00");
	navElement.push_back(videoSlide.get());

	btnRepeat = CreateButton("IDB_REFRESH", "LBLREPEAT", REPEATID, false);
	btnSpeaker = CreateButton("IDB_VOLUME_UP_VIDEO", "LBLVOLUME", SPEAKERBUTTONID, false);
	btnZoom = CreateButton("IDB_ZOOM", "LBLZOOM", ZOOMBUTTONID, false);
	btnScreenFormat = CreateButton("IDB_SCREENPNG", "LBLSCREEN", SCREENFORMATID, false);

	/*
	// Ordre d'affichage dans la toolbar
	navElement.push_back(btnPlay.get());
	navElement.push_back(btnPause.get());
	
	navElement.push_back(btnRepeat.get());
	navElement.push_back(btnSpeaker.get());
	navElement.push_back(btnZoom.get());
	navElement.push_back(btnScreenFormat.get());
	*/
	// État initial : en pause (bouton Play visible, Pause masqué)
	btnPlay->SetVisible(true);
	btnPause->SetVisible(false);
}


void CSliderVideoToolbar::SetPlay()
{
	isPlay = true;
	btnPlay->SetVisible(false);
	btnPause->SetVisible(true);
	needToRefresh = true;
}

void CSliderVideoToolbar::SetPause()
{
	isPlay = false;
	btnPlay->SetVisible(true);
	btnPause->SetVisible(false);
	needToRefresh = true;
}

void CSliderVideoToolbar::SetZoomIndex(const int& index)
{
	zoomIndex = index;
}

void CSliderVideoToolbar::SetTotalSecondTime(const int64_t& millisecondTime)
{
	if (videoSlide != nullptr)
	{
		videoSlide->SetTotalDuration(millisecondTime);
		needToRefresh = true;
	}
}

void CSliderVideoToolbar::SetPastSecondTime(const int64_t& millisecondTime)
{
	if (videoSlide != nullptr && !videoSlide->IsDragging())
	{
		videoSlide->SetCurrentTime(millisecondTime);
		needToRefresh = true;
	}
}

void CSliderVideoToolbar::UpdatePositionEvent()
{
	if (sliderEvent != nullptr && videoSlide != nullptr)
	{
		sliderEvent->MoveSlider(videoSlide->GetCurrentTime() / 1000);
	}
}

// ============================================================
// Implémentation de CSliderInterface
// ============================================================

void CSliderVideoToolbar::MoveSlider(const int64_t& positionMs)
{
	if (sliderEvent != nullptr)
	{
		// CSliderVideo transmettait la position en secondes à sliderEvent->MoveSlider
		sliderEvent->MoveSlider(positionMs / 1000);
	}
	needToRefresh = true;
}

void CSliderVideoToolbar::SlidePosChange(const int& position, const wxString& key)
{
	if (sliderEvent != nullptr)
		sliderEvent->SlidePosChange(position, key);
}

void CSliderVideoToolbar::ZoomPos(const int& position)
{
	if (sliderEvent != nullptr)
		sliderEvent->ZoomPos(position);
}

void CSliderVideoToolbar::ClickButton(const int& id)
{
	if (sliderEvent != nullptr)
		sliderEvent->ClickButton(id);
}

void CSliderVideoToolbar::SetTrackBarPosition(const int& iPos)
{
	if (videoSlide != nullptr)
	{
		videoSlide->SetPosition(iPos);
		needToRefresh = true;
	}
}

// ============================================================
// Gestion des événements des boutons
// ============================================================

void CSliderVideoToolbar::EventManager(const int& id)
{
	switch (id)
	{
	case PLAYBUTTONID:
	case PAUSEBUTTONID:
	case SCREENFORMATID:
		if (sliderEvent != nullptr)
			sliderEvent->ClickButton(id);
		break;

	case REPEATID:
		repeatActif = !repeatActif;
		btnRepeat->SetActif();
		if (sliderEvent != nullptr)
			sliderEvent->ClickButton(REPEATID);
		needToRefresh = true;
		break;

	case SPEAKERBUTTONID:
		ShowVolumePopup();
		break;

	case ZOOMBUTTONID:
		ShowZoomPopup();
		break;

	default:
		break;
	}
}

wxRect CSliderVideoToolbar::GetButtonRect(CToolbarButton* targetButton) const
{
	if (targetButton == nullptr)
		return wxRect(0, 0, 0, 0);

	return wxRect(targetButton->GetXPos(), targetButton->GetYPos(),
		targetButton->GetWidth(), targetButton->GetHeight());
}

void CSliderVideoToolbar::ShowVolumePopup()
{
	if (sliderEvent == nullptr)
		return;

	Regards::UI::ModernTheme m_theme;
	m_theme.background = themeToolbar.colorBack;
	m_theme.secondaryText = themeToolbar.slider.colorInactifReplacement;

	const wxRect btnRect = GetButtonRect(btnSpeaker.get());
	const wxPoint screenPos = ClientToScreen(wxPoint(btnRect.x, btnRect.y));

	popup = new Regards::UI::CVolumePopupModern(this, sliderEvent->GetVolume(), m_theme, 100,
		[this](int newVolume) {
			if (sliderEvent != nullptr)
				sliderEvent->SlidePosChange(newVolume, "VOLUME");
		});

	const int popupHeight = popup->GetSize().y;
	const int popupWidth = popup->GetSize().x;
	const int xOffset = screenPos.x + (btnRect.width / 2) - (popupWidth / 2);
	const int yOffset = screenPos.y - popupHeight - 4;

	popup->Position(wxPoint(xOffset, yOffset), wxSize(0, 0));
	popup->Popup();
}

void CSliderVideoToolbar::ShowZoomPopup()
{
	if (sliderEvent == nullptr)
		return;

	Regards::UI::ModernTheme m_theme;
	m_theme.background = themeToolbar.colorBack;
	m_theme.secondaryText = themeToolbar.slider.colorInactifReplacement;

	const std::vector<int> zoomValues = {
		1, 2, 3, 4, 5, 6, 8, 12, 16, 25, 33, 50, 66, 75, 100,
		133, 150, 166, 200, 300, 400, 500, 600, 700, 800, 1200, 1600
	};

	const wxRect btnRect = GetButtonRect(btnZoom.get());
	const wxPoint screenPos = ClientToScreen(wxPoint(btnRect.x, btnRect.y));

	popup = new Regards::UI::CVolumePopupModern(this, sliderEvent->GetZoomIndex(), m_theme,
		static_cast<int>(zoomValues.size()),
		[this](int newZoom) {
			if (sliderEvent != nullptr)
				sliderEvent->SlidePosChange(newZoom, "ZOOM");
		});

	const int popupHeight = popup->GetSize().y;
	const int popupWidth = popup->GetSize().x;
	const int xOffset = screenPos.x + (btnRect.width / 2) - (popupWidth / 2);
	const int yOffset = screenPos.y - popupHeight - 4;

	popup->Position(wxPoint(xOffset, yOffset), wxSize(0, 0));
	popup->Popup();
}
