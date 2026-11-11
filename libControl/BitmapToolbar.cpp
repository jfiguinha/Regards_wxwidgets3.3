#include <header.h>
#include "BitmapToolbar.h"
#include <LibResource.h>
#include <ToolbarButton.h>
#include <ToolbarInterface.h>
#include "CentralWindow.h"
#include "BitmapWndViewer.h"
#include "BitmapWnd3d.h"
#if defined(__WXMSW__)
#include "../include/window_id.h"
#else
#include <window_id.h>
#endif

using namespace Regards::Control;
using namespace Regards::Viewer;

constexpr auto WM_EMAIL = 4;
constexpr auto WM_IMPRIMER = 5;
constexpr auto IDM_SETSHRINK = 7;
constexpr auto WM_EXPORT = 8;

CBitmapToolbar::CBitmapToolbar(wxWindow* parent, wxWindowID id, wxWindowID viewerId, const CThemeToolbar& theme,
	const bool& vertical, const bool& exportPicture,
	CToolbarInterface* toolbarInterface)
	: CToolbarWindow(parent, id, theme, vertical)
{
	themeToolbar = theme;
	this->parentId = viewerId;
	this->exportPicture = exportPicture;
	this->toolbarInterface = toolbarInterface;

	// Mode Bitmap
	email = CreateButton("IDB_EMAIL", "LBLEMAIL", WM_EMAIL, false);
	printer = CreateButton("IDB_PRINTERPNG", "LBLPRINT", WM_IMPRIMER, false);
	shrink = CreateButton("IDB_SHRINK", "LBLSHRINK", IDM_SETSHRINK, false);
	//moins = CreateButton("IDB_ZOOMMOINS", "LBLZOOMOFF", WM_ZOOMOUT, false);

	slide = std::make_unique<CToolbarSlide>(themeToolbar.slider, this);

	//plus = CreateButton("IDB_ZOOMPLUS", "LBLZOOMON", WM_ZOOMON, false);

	// Mode Animation
	imagePrec = CreateButton(L"IDB_ARROWLPNG", L"LBLPREVIOUS", WM_IMAGES_PRCDENTE, false);
	imagePrec->SetRepeatable(true);
	imagePlayDiaporama = CreateButton(L"IDB_PLAY", L"LBLPLAY", WM_DIAPORAMA_PLAY, false);
	imageStopDiaporama = CreateButton(L"IDB_STOP", L"LBLSTOP", WM_DIAPORAMA_STOP, false);
	imageSuiv = CreateButton(L"IDB_ARROWRPNG", L"LBLNEXT", WM_IMAGES_SUIVANTE, false);
	imageSuiv->SetRepeatable(true);

	separator = std::make_unique<CToolbarSeparator>();

	// Mode par défaut : Bitmap (comportement historique de CBitmapToolbar)
	ApplyMedia();
}

void CBitmapToolbar::SetMedia(const MediaMode& mode)
{
	if (mode == mediaMode)
		return;

	mediaMode = mode;
	ApplyMedia();
	Resize();
	needToRefresh = true;
}

void CBitmapToolbar::ApplyMedia()
{
	const bool bitmapMode = (mediaMode == MediaMode::Bitmap);
	const bool animationMode = (mediaMode == MediaMode::Animation);
	const bool bitmapButtons = bitmapMode || animationMode;

	// Ordre d'affichage : [navigation animation] | séparateur | [email, imprimante, shrink, zoom]
	// Aucun et Video : aucun bouton (les commandes video sont dans CSliderVideo)
	navElement.clear();
	if (animationMode)
	{
		navElement.push_back(imagePrec.get());
		navElement.push_back(imagePlayDiaporama.get());
		navElement.push_back(imageStopDiaporama.get());
		navElement.push_back(imageSuiv.get());
		navElement.push_back(separator.get());
	}
	if (bitmapButtons)
	{
		navElement.push_back(email.get());
		navElement.push_back(printer.get());
		navElement.push_back(shrink.get());
		navElement.push_back(slide.get());
	}

	imagePrec->SetVisible(animationMode);
	imageSuiv->SetVisible(animationMode);
	imagePlayDiaporama->SetVisible(animationMode && !animationRunning);
	imageStopDiaporama->SetVisible(animationMode && animationRunning);
	separator->SetVisible(animationMode);

	email->SetVisible(bitmapButtons);
	printer->SetVisible(bitmapButtons);
	shrink->SetVisible(bitmapButtons);
}

void CBitmapToolbar::ZoomPos(const int& position)
{
	auto bitmapWindow = dynamic_cast<IBitmapWnd*>(this->FindWindowById(parentId));
	if (bitmapWindow != nullptr)
	{
		auto wndViewer = static_cast<CBitmapWndViewer*>(bitmapWindow->GetWndPt());
		if (wndViewer != nullptr)
			wndViewer->SetRatioPos(position);
	}
}

void CBitmapToolbar::ZoomOn()
{
	auto bitmapWindow = dynamic_cast<IBitmapWnd*>(this->FindWindowById(parentId));
	if (bitmapWindow != nullptr)
	{
		auto wndViewer = static_cast<CBitmapWndViewer*>(bitmapWindow->GetWndPt());
		if (wndViewer != nullptr)
			wndViewer->ZoomOn();
	}

	//SetTrackBarPosition(bitmapWindow->GetPosRatio());
	ChangeZoomInPos();
}

void CBitmapToolbar::ChangeZoomInPos()
{
	auto bitmapWindow = dynamic_cast<IBitmapWnd*>(this->FindWindowById(parentId));
	if (slide != nullptr && bitmapWindow != nullptr)
	{
		auto wndViewer = static_cast<CBitmapWndViewer*>(bitmapWindow->GetWndPt());
		if (wndViewer != nullptr)
		{
			int dwPos = wndViewer->GetPosRatio();
			if (dwPos >= slide->GetNbValue())
				dwPos = slide->GetNbValue() - 1;
			SetTrackBarPosition(dwPos);
		}
	}
}

void CBitmapToolbar::ChangeZoomOutPos()
{
	auto bitmapWindow = dynamic_cast<IBitmapWnd*>(this->FindWindowById(parentId));
	if (slide != nullptr && bitmapWindow != nullptr)
	{
		auto wndViewer = static_cast<CBitmapWndViewer*>(bitmapWindow->GetWndPt());
		if (wndViewer != nullptr)
		{
			int dwPos = wndViewer->GetPosRatio();
			if (dwPos < 0)
				dwPos = 0;
			SetTrackBarPosition(dwPos);
		}
	}
}


void CBitmapToolbar::ZoomOut()
{
	auto bitmapWindow = dynamic_cast<IBitmapWnd*>(this->FindWindowById(parentId));
	if (bitmapWindow != nullptr)
	{
		auto wndViewer = static_cast<CBitmapWndViewer*>(bitmapWindow->GetWndPt());
		if (wndViewer != nullptr)
			wndViewer->ZoomOut();
	}
	//SetTrackBarPosition(bitmapWindow->GetPosRatio());
	ChangeZoomOutPos();
}

void CBitmapToolbar::SetTabValue(vector<int> value)
{
	if (slide != nullptr)
		slide->SetTabValue(value);
}

void CBitmapToolbar::ClickButton(const int& id)
{}

void CBitmapToolbar::SetTrackBarPosition(const int& iPos)
{
	int positionTrackBar = iPos;
	if (slide != nullptr)
	{
		slide->SetPosition(positionTrackBar);
		needToRefresh = true;
	}
}

void CBitmapToolbar::SlidePosChange(const int& position, const wxString& key)
{
	auto bitmapWindow = dynamic_cast<IBitmapWnd*>(this->FindWindowById(parentId));
	if (bitmapWindow != nullptr)
	{
		auto wndViewer = static_cast<CBitmapWndViewer*>(bitmapWindow->GetWndPt());
		if (wndViewer != nullptr)
			wndViewer->SetZoomPosition(position);
	}
}

void CBitmapToolbar::NextPicture()
{
	auto mainWindow = static_cast<CCentralWindow*>(this->FindWindowById(CENTRALVIEWERWINDOWID));
	if (mainWindow != nullptr)
		mainWindow->AnimationPictureNext();
}

void CBitmapToolbar::PreviousPicture()
{
	auto mainWindow = static_cast<CCentralWindow*>(this->FindWindowById(CENTRALVIEWERWINDOWID));
	if (mainWindow != nullptr)
		mainWindow->AnimationPicturePrevious();
}

void CBitmapToolbar::AnimationStart()
{
	auto mainWindow = static_cast<CCentralWindow*>(this->FindWindowById(CENTRALVIEWERWINDOWID));
	if (mainWindow != nullptr)
	{
		animationRunning = true;
		imagePlayDiaporama->SetVisible(false);
		imageStopDiaporama->SetVisible(true);
		auto event = new wxCommandEvent(wxEVENT_ANIMATIONSTART);
		wxQueueEvent(mainWindow, event);
		needToRefresh = true;
	}
}

void CBitmapToolbar::AnimationStop()
{
	auto mainWindow = static_cast<CCentralWindow*>(this->FindWindowById(CENTRALVIEWERWINDOWID));
	if (mainWindow != nullptr)
	{
		animationRunning = false;
		imageStopDiaporama->SetVisible(false);
		imagePlayDiaporama->SetVisible(true);

		auto event = new wxCommandEvent(wxEVENT_ANIMATIONSTOP);
		wxQueueEvent(mainWindow, event);

		needToRefresh = true;
	}
}

void CBitmapToolbar::EventManager(const int& id)
{
	switch (mediaMode)
	{
	case MediaMode::Bitmap:
		EventManagerBitmap(id);
		break;
	case MediaMode::Animation:
		// Tous les boutons sont présents : navigation d'abord, sinon boutons bitmap
		if (!EventManagerAnimation(id))
			EventManagerBitmap(id);
		break;
	default:;
	}
}

bool CBitmapToolbar::EventManagerAnimation(const int& id)
{
	switch (id)
	{
	case WM_DIAPORAMA_PLAY:
		AnimationStart();
		return true;

	case WM_DIAPORAMA_STOP:
		AnimationStop();
		return true;

	case WM_IMAGES_PRCDENTE:
		PreviousPicture();
		return true;

	case WM_IMAGES_SUIVANTE:
		NextPicture();
		return true;
	default:;
	}
	return false;
}

void CBitmapToolbar::EventManagerBitmap(const int& id)
{
	auto bitmapWindow = dynamic_cast<IBitmapWnd*>(this->FindWindowById(parentId));
	if (bitmapWindow != nullptr)
	{
		auto wndViewer = static_cast<CBitmapWndViewer*>(bitmapWindow->GetWndPt());
		if (wndViewer != nullptr)
		{
			switch (id)
			{
			case WM_IMPRIMER:
				wndViewer->PrintPicture();
				break;
			case IDM_SETSHRINK:
				wndViewer->ShrinkImage();
				break;

			case WM_EXPORT:
			{
				if (exportPicture)
					wndViewer->ExportPicture();
				else
					wndViewer->SavePicture();
				break;
			}
			case WM_EMAIL:
				wndViewer->SendEmail();
				break;

			case WM_ZOOMOUT:
				ZoomOut();
				break;
			case WM_ZOOMON:
				ZoomOn();
				break;
			default:;
			}
		}
	}
}