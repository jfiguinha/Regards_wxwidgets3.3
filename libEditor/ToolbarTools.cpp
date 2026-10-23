#include <header.h>
#include "ToolbarTools.h"
#include <LibResource.h>
#include <effect_id.h>

#include <FilterData.h>
using namespace Regards::Window;
using namespace Regards::Editor;




CToolbarTools::CToolbarTools(wxWindow* parent, wxWindowID id, const CThemeToolbar& theme, wxWindow* frameToSend, const bool& vertical)
	: CToolbarWindow(parent, id, theme, vertical)
{

	saveLastPush = true;
	this->frameToSend = frameToSend;

	selectionRectangle = CreateButton("IDB_RECTANGLESELECT", "IDM_RECTANGLESELECT", IDM_RECTANGLESELECT, false);
	selectionEllipse = CreateButton("IDB_ELLIPSESELECT", "IDM_ELLIPSESELECT", IDM_ELLIPSESELECT, false);
	selectionLasso = CreateButton("IDB_LASSOSELECT", "IDM_LASSOSELECT", IDM_LASSOSELECT, false);
	selectionCrop = CreateButton("IDB_CROP", "IDM_PAINTCROP", IDM_PAINTCROP, false);
	selectMove = CreateButton("IDB_MOVESELECT", "IDB_MOVE", WM_SELECTMOVE, false);
	drawingTools = CreateButton("IDB_PEN", "IDM_PENFILTER", IDM_PENFILTER, false);
	geometricTools = CreateButton("IDB_RECTANGLE", "IDM_RECTANGLEFILTER", IDM_GEOMETRICFILTER, false);
	textTools = CreateButton("IDB_TEXT", "IDM_TEXTFILTER", IDM_TEXTFILTER, false);
	zoomTools = CreateButton("IDB_ZOOM", "WM_SELECTZOOM", WM_SELECTZOOM, false);
	pictureMoveTools = CreateButton("IDB_MOVEPICTURE", "WM_MOVEPICTURE", WM_MOVEPICTURE, false);;
	paintbucketTools = CreateButton("IDB_PAINTBUCKET", "IDM_PAINTBUCKETFILTER", IDM_PAINTBUCKETFILTER, false);
	gradientTools = CreateButton("IDB_GRADIENT", "IDM_GRADIENTFILTER", IDM_GRADIENTFILTER, false);

}


void CToolbarTools::Resize()
{
	needToRefresh = true;
}

void CToolbarTools::EventManager(const int& id)
{
	if (frameToSend)
	{
		wxCommandEvent evt(wxEVENT_TOOLCHOOSE);
		evt.SetInt(id);
		frameToSend->GetEventHandler()->AddPendingEvent(evt);
	}


}
