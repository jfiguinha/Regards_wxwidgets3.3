#include <header.h>
#include "PreviewBitmapToolbar.h"
#include <LibResource.h>
#include "BitmapWndViewer.h"
#include "BitmapWnd3d.h"

using namespace Regards::Control;
using namespace Regards::Viewer;

namespace
{
	// Identifiants locaux à cette barre (volontairement hors plage des WM_/IDM_ de CGeneralToolbar)
	constexpr int IDM_PB_EMAIL = 20001;
	constexpr int IDM_PB_PRINT = 20002;
	constexpr int IDM_PB_SHRINK = 20003;

	CBitmapWndViewer* FindViewer(const wxWindowID viewerId)
	{
		if (viewerId == wxID_ANY)
			return nullptr;
		auto bitmapWindow = dynamic_cast<IBitmapWnd*>(wxWindow::FindWindowById(viewerId));
		if (bitmapWindow == nullptr)
			return nullptr;
		return static_cast<CBitmapWndViewer*>(bitmapWindow->GetWndPt());
	}
}

CPreviewBitmapToolbar::CPreviewBitmapToolbar(wxWindow* parent, wxWindowID id, wxWindowID viewerId,
                                             const CThemeToolbar& theme, CToolbarInterface* toolbarInterface,
                                             const bool& vertical)
	: CGeneralToolbar(parent, id, theme, toolbarInterface, vertical), parentId(viewerId)
{

	const std::vector<int> zoomValues = {
	1, 2, 3, 4, 5, 6, 8, 12, 16, 25, 33, 50, 66, 75, 100,
	133, 150, 166, 200, 300, 400, 500, 600, 700, 800, 1200, 1600
	};

	email = CreateButton("IDB_EMAIL", "LBLEMAIL", IDM_PB_EMAIL, false);
	printer = CreateButton("IDB_PRINTERPNG", "LBLPRINT", IDM_PB_PRINT, false);
	shrink = CreateButton("IDB_SHRINK", "LBLSHRINK", IDM_PB_SHRINK, false);

	slide = std::make_unique<CToolbarSlide>(themeToolbar.slider, this);

	textCurrent = std::make_unique<CToolbarTexte>(themeToolbar.texte);
	textMax = std::make_unique<CToolbarTexte>(themeToolbar.texte);

	separatorNav = std::make_unique<CToolbarSeparator>();
	separatorZoom = std::make_unique<CToolbarSeparator>();
	separatorShare = std::make_unique<CToolbarSeparator>();

	// L'ordre de navElement = l'ordre d'affichage (cf. CToolbarWindow::RecalcLayout).
	// Le constructeur de CGeneralToolbar y a déjà inscrit tous ses boutons : on reconstruit.
	navElement.clear();

	navElement.push_back(imageFirst.get());
	navElement.push_back(imagePrec.get());
	navElement.push_back(imagePlayDiaporama.get());
	navElement.push_back(imageStopDiaporama.get());
	navElement.push_back(imageSuiv.get());
	navElement.push_back(imageEnd.get());

	navElement.push_back(separatorNav.get());

	navElement.push_back(shrink.get());
	navElement.push_back(textCurrent.get());
	navElement.push_back(slide.get());
	navElement.push_back(textMax.get());

	navElement.push_back(separatorZoom.get());

	navElement.push_back(rotate270.get());
	navElement.push_back(rotate90.get());
	navElement.push_back(flipHorizontal.get());
	navElement.push_back(flipVertical.get());

	navElement.push_back(separatorShare.get());

	navElement.push_back(email.get());
	navElement.push_back(printer.get());
	navElement.push_back(save.get());
	navElement.push_back(exportFile.get());


	if (slide != nullptr)
		slide->SetTabValue(zoomValues);
}

void CPreviewBitmapToolbar::DisableNavigationButton()
{
	CGeneralToolbar::DisableNavigationButton();
	separatorNav->SetVisible(false);
	needToRefresh = true;
}

void CPreviewBitmapToolbar::EnableNavigationButton()
{
	CGeneralToolbar::EnableNavigationButton();
	separatorNav->SetVisible(true);
	needToRefresh = true;
}

wxString CPreviewBitmapToolbar::FormatPercent(const int& value) const
{
	return wxString::Format(L"%d %%", value);
}

void CPreviewBitmapToolbar::SetTabValue(const std::vector<int>& value)
{
	zoomValues = value;
	if (slide != nullptr)
		slide->SetTabValue(value);

	textMax->SetLibelle(zoomValues.empty() ? wxString() : FormatPercent(zoomValues.back()));
	needToRefresh = true;
}

void CPreviewBitmapToolbar::UpdateZoomText(const int& position)
{
	if (position >= 0 && position < static_cast<int>(zoomValues.size()))
		textCurrent->SetLibelle(FormatPercent(zoomValues[position]));
	else
		textCurrent->SetLibelle(wxString());
}

void CPreviewBitmapToolbar::SetTrackBarPosition(const int& iPos)
{
	if (slide != nullptr)
	{
		slide->SetPosition(iPos);
		UpdateZoomText(iPos);
		needToRefresh = true;
	}
}

void CPreviewBitmapToolbar::SlidePosChange(const int& position, const wxString& key)
{
	auto wndViewer = FindViewer(parentId);
	if (wndViewer != nullptr)
		wndViewer->SetZoomPosition(position);

	UpdateZoomText(position);
	needToRefresh = true;
}

void CPreviewBitmapToolbar::ZoomPos(const int& position)
{
	auto wndViewer = FindViewer(parentId);
	if (wndViewer != nullptr)
		wndViewer->SetRatioPos(position);
}

void CPreviewBitmapToolbar::EventManager(const int& id)
{
	switch (id)
	{
	case IDM_PB_EMAIL:
	case IDM_PB_PRINT:
	case IDM_PB_SHRINK:
		{
			auto wndViewer = FindViewer(parentId);
			if (wndViewer == nullptr)
				return;

			if (id == IDM_PB_EMAIL)
				wndViewer->SendEmail();
			else if (id == IDM_PB_PRINT)
				wndViewer->PrintPicture();
			else
				wndViewer->ShrinkImage();
			break;
		}
	default:
		// navigation, rotations, flips, save, export : comportement de CGeneralToolbar
		CGeneralToolbar::EventManager(id);
		break;
	}
}
