#include <header.h>
#include "LayerToolBar.h"
#include <LibResource.h>
#include <window_id.h>
using namespace Regards::Control;


CLayerToolBar::CLayerToolBar(wxWindow* parent, wxWindowID id, wxWindowID frameId, const CThemeToolbar& theme, const bool& vertical)
	: CToolbarWindow(parent, id, theme, vertical)
{
	this->frameId = frameId;
	themeToolbar = theme;
	themeToolbar.position = NAVIGATOR_LEFT;
	
	newlayer = CreateButton("IDB_LAYER_NEW", "LBLNEWLAYER", wxEVENT_CREATELAYER, false);
	deleteLayer = CreateButton("IDB_LAYER_DELETE", "LBLDELETELAYER", wxEVENT_DELETELAYER, false);
	copyLayer = CreateButton("IDB_LAYER_COPY", "LBLCOPYLAYER", wxEVENT_COPYLAYER, false);
	fusionLayer = CreateButton("IDB_LAYER_FUSION", "LBLFUSIONLAYER", wxEVENT_FUSIONLAYER, false);
	moveupLayer = CreateButton("IDB_LAYER_MOVEUP", "LBLMOVEUPLAYER", wxEVENT_MOVEUPLAYER, false);
	movedownLayer = CreateButton("IDB_LAYER_MOVEDOWN", "LBLMOVEDOWNLAYER", wxEVENT_MOVEDOWNLAYER, false);
	propertiesLayer = CreateButton("IDB_LAYER_PROPERTIES", "LBLMOVEDOWNLAYER", wxEVENT_PROPERTIESLAYER, false);
}

void CLayerToolBar::EventManager(const int& id)
{
	wxWindow * frameWindow = FindWindow(frameId);
	if (frameWindow)
	{
		wxCommandEvent evt(id);
		frameWindow->GetEventHandler()->AddPendingEvent(evt);
	}
}
