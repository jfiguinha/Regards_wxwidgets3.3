#include <header.h>
#include "ToolbarTools.h"
#include <LibResource.h>
#include <effect_id.h>

#include <FilterData.h>
using namespace Regards::Window;
using namespace Regards::Editor;


#define WM_SELECTMOVE FILTER_DRAWING_END + 1
#define WM_SELECTZOOM FILTER_DRAWING_END + 2
#define WM_MOVEPICTURE FILTER_DRAWING_END + 3

CToolbarTools::CToolbarTools(wxWindow* parent, wxWindowID id, const CThemeToolbar& theme, const bool& vertical)
	: CToolbarWindow(parent, id, theme, vertical)
{

	saveLastPush = true;


	selection = CreateButton("IDB_SELECTFILTER", "IDM_SELECTFILTER", IDM_SELECTFILTER, false);
	selectionCrop = CreateButton("IDB_CROP", "IDM_PAINTCROP", IDM_PAINTCROP, false);
	selectMove = CreateButton("IDB_MOVESELECT", "IDB_MOVE", WM_SELECTMOVE, false);
	drawingTools = CreateButton("IDB_PEN", "IDM_PENFILTER", IDM_PENFILTER, false);
	geometricTools = CreateButton("IDB_RECTANGLE", "IDM_RECTANGLEFILTER", IDM_RECTANGLEFILTER, false);
	textTools = CreateButton("IDB_TEXT", "IDM_TEXTFILTER", IDM_TEXTFILTER, false);
	zoomTools = CreateButton("IDB_ZOOM", "WM_SELECTZOOM", WM_SELECTZOOM, false);
	pictureMoveTools = CreateButton("IDB_MOVEPICTURE", "WM_MOVEPICTURE", WM_MOVEPICTURE, false);;
	paintbucketTools = CreateButton("IDB_PAINTBUCKET", "IDM_PAINTBUCKETFILTER", IDM_PAINTBUCKETFILTER, false);
	gradientTools = CreateButton("IDB_GRADIENT", "IDM_GRADIENTFILTER", IDM_GRADIENTFILTER, false);

}


void CToolbarTools::Resize()
{
	/*
	int nbElement = static_cast<int>(navElement.size());
	themeToolbar..SetTailleX(GetWindowWidth() / nbElement);

	for (auto& nav : navElement)
	{
		nav->Resize(themeToolbar.texte.GetTailleX(), themeToolbar.texte.GetTailleY());
	}
	*/
	needToRefresh = true;
}

void CToolbarTools::EventManager(const int& id)
{
	switch (id)
	{
	case IDM_SELECTFILTER:
		break;
	case IDM_PAINTCROP:
		break;
	case WM_SELECTMOVE:
		break;
	case IDM_PENFILTER:
		break;
	case IDM_RECTANGLEFILTER:
		break;
	case IDM_TEXTFILTER:
		break;
	case WM_SELECTZOOM:
		break;
	case WM_MOVEPICTURE:
		break;
	case IDM_PAINTBUCKETFILTER:
		break;
	case IDM_GRADIENTFILTER:
		break;
	}
}
