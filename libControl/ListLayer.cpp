#include <header.h>
#include "ListLayer.h"
#include <FileUtility.h>
#include "MainTheme.h"
#include "MainThemeInit.h"
#include <LibResource.h>
#include "ViewerParam.h"
#include "ViewerParamInit.h"
#include "LayerToolBar.h"
#include <ImageLoadingFormat.h>
#include <WindowManager.h>
#include "ThumbnailCalqueWnd.h"
#include "LayerToolBar.h"
#include <LayerPropertiesDialog.h>
using namespace Regards::Picture;
using namespace Regards::Window;
using namespace Regards::Control;
using namespace Regards::Dialog;


CListLayer::CListLayer(wxWindow* parent, wxWindowID id)
	: CWindowMain("CListLayer", parent, id)
{

	bool checkValidity = false;
	wxRect rect;
	CMainTheme* viewerTheme = CMainThemeInit::getInstance();

	if (viewerTheme != nullptr)
	{
		{
			CThemeSplitter theme;
			viewerTheme->GetSplitterTheme(&theme);
			windowManager = new CWindowManager(this, wxID_ANY, theme);
		}
		{
			bool checkValidity = false;

			CThemeScrollBar themeScroll;
			viewerTheme->GetScrollTheme(&themeScroll);

			CThemeThumbnail themeThumbnail;
			viewerTheme->GetThumbnailTheme(&themeThumbnail);

			CMainParam* main_param = CMainParamInit::getInstance();
			if (main_param != nullptr)
				checkValidity = main_param->GetCheckThumbnailValidity();

			thumbnailCalqueWnd = new CThumbnailCalqueWnd(this, wxID_ANY, themeScroll, themeThumbnail, PANELINFOSWNDID,
				checkValidity);

			thumbnailCalqueWnd->Show(true);

			windowManager->AddWindow(thumbnailCalqueWnd, Pos::wxCENTRAL, false, 0, rect, wxID_ANY, false);
		}

		{
			CThemeToolbar theme;
			//viewerTheme->GetThumbnailToolbarTheme(theme);
			viewerTheme->GetBitmapToolbarTheme(&theme);
			theme.position = NAVIGATOR_LEFT;
			layerToolbar = new CLayerToolBar(windowManager, wxID_ANY, theme, false);

			windowManager->AddWindow(layerToolbar, Pos::wxBOTTOM, true, layerToolbar->GetHeight(), rect, wxID_ANY, false);
		}

	}
	Connect(wxEVENT_CREATELAYER, wxCommandEventHandler(CListLayer::CreateLayer));
	Connect(wxEVENT_DELETELAYER, wxCommandEventHandler(CListLayer::DeleteLayer));
	Connect(wxEVENT_COPYLAYER, wxCommandEventHandler(CListLayer::CopyLayer));
	Connect(wxEVENT_FUSIONLAYER, wxCommandEventHandler(CListLayer::FusionLayer));
	Connect(wxEVENT_MOVEUPLAYER, wxCommandEventHandler(CListLayer::MoveUpLayer));
	Connect(wxEVENT_MOVEDOWNLAYER, wxCommandEventHandler(CListLayer::MoveDownLayer));
	Connect(wxEVENT_PROPERTIESLAYER, wxCommandEventHandler(CListLayer::PropertiesLayer));
}

CListLayer::~CListLayer()
{
}

void CListLayer::SetLayer(CLayerList* listOfLayer)
{
	this->listOfLayer = listOfLayer;
	thumbnailCalqueWnd->SetLayer(listOfLayer);
}

void CListLayer::UpdateScreenRatio()
{
	if (windowManager)
		windowManager->UpdateScreenRatio();
}

/*
void CListLayer::SetFilename(const wxString& filename)
{
	thumbnailCalqueWnd->SetFile(filename);
}
*/
void CListLayer::DeleteLayer(wxCommandEvent& event)
{
	int numSelect = thumbnailCalqueWnd->GetSelectLayer();
	if (numSelect != -1)
	{
		const wxString msg = "Do you want delete this layer ?";
		const wxString info = CLibResource::LoadStringFromResource(L"labelInformations", 1);
		if (wxMessageBox(msg, info, wxYES_NO | wxICON_WARNING) == wxYES)
		{

		}
	}
}

void CListLayer::CopyLayer(wxCommandEvent& event)
{

}

void CListLayer::CreateLayer(wxCommandEvent& event)
{
	int width = listOfLayer->GetWidth();
	int height = listOfLayer->GetHeight();
}

void CListLayer::FusionLayer(wxCommandEvent& event)
{

}

void CListLayer::MoveUpLayer(wxCommandEvent& event)
{

}

void CListLayer::MoveDownLayer(wxCommandEvent& event)
{

}

void CListLayer::PropertiesLayer(wxCommandEvent& event)
{
	CLayerPropertiesDialog dialog(
		this,
		"Mon calque",
		Regards::Dialog::LayerBlendMode::Normal,
		80);

	if (dialog.ShowModal() == wxID_OK)
	{
		const wxString layerName = dialog.GetLayerName();

		const auto blendMode = dialog.GetBlendMode();

		const int opacity = dialog.GetOpacity();

		// Utilisation des paramètres...
	}
}

void CListLayer::SetActifItem(const int& numItem, const bool& move)
{

}


int CListLayer::GetNumItem()
{


	return 0;
}


void CListLayer::Resize()
{
	if (windowManager != nullptr)
	{
		windowManager->SetSize(GetWindowWidth(), GetWindowHeight());
		needToRefresh = true;
	}
}