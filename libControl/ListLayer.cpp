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

using namespace Regards::Picture;
using namespace Regards::Window;
using namespace Regards::Control;


CListLayer::CListLayer(wxWindow* parent, wxWindowID id, wxWindowID frameId)
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

			thumbnailCalqueWnd = new CThumbnailCalqueWnd(this, wxID_ANY, frameId, themeScroll, themeThumbnail, PANELINFOSWNDID,
				checkValidity);

			thumbnailCalqueWnd->Show(true);

			windowManager->AddWindow(thumbnailCalqueWnd, Pos::wxCENTRAL, false, 0, rect, wxID_ANY, false);
		}

		{
			CThemeToolbar theme;
			//viewerTheme->GetThumbnailToolbarTheme(theme);
			viewerTheme->GetBitmapToolbarTheme(&theme);
			theme.position = NAVIGATOR_LEFT;
			layerToolbar = new CLayerToolBar(windowManager, wxID_ANY, frameId, theme, false);

			windowManager->AddWindow(layerToolbar, Pos::wxBOTTOM, true, layerToolbar->GetHeight(), rect, wxID_ANY, false);
		}

	}

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


void CListLayer::RefreshList()
{
	if(thumbnailCalqueWnd)
		thumbnailCalqueWnd->RefreshList();
}

int CListLayer::GetActifLayer()
{
	if (thumbnailCalqueWnd)
		return thumbnailCalqueWnd->GetActifLayer();
	return -1;
}

vector<int> CListLayer::GetSelectLayer()
{
	if (thumbnailCalqueWnd)
		return thumbnailCalqueWnd->GetSelectLayer();
	return vector<int>();
}


void CListLayer::Resize()
{
	if (windowManager != nullptr)
	{
		windowManager->SetSize(GetWindowWidth(), GetWindowHeight());
		needToRefresh = true;
	}
}