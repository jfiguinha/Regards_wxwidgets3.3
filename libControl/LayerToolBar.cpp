#include <header.h>
#include "LayerToolBar.h"
#include <LibResource.h>
#include <window_id.h>
using namespace Regards::Control;


CLayerToolBar::CLayerToolBar(wxWindow* parent, wxWindowID id, const CThemeToolbar& theme, const bool& vertical)
	: CToolbarWindow(parent, id, theme, vertical)
{
	themeToolbar = theme;

	deleteButton = CreateButton("IDB_DELETE", "LBLDELETELAYER", WM_CLEAR, false);
	copy = CreateButton("IDB_MULTIPLESELECT", "LBLCOPYLAYER", WM_COPY, false);
	plus = CreateButton("IDB_PLUS", "LBLPLUSLAYER", wxEVENT_CREATELAYER, false);

	navElement.push_back(deleteButton.get());
	navElement.push_back(copy.get());
	navElement.push_back(plus.get());
}

void CLayerToolBar::PostEvent(wxEventType type)
{
	wxCommandEvent evt(type);
	this->GetParent()->GetEventHandler()->AddPendingEvent(evt);
}

void CLayerToolBar::EventManager(const int& id)
{
		switch (id)
		{
		case WM_CLEAR:
			PostEvent(wxEVENT_DELETELAYER);
			break;

		case WM_COPY:
			PostEvent(wxEVENT_COPYLAYER);
			break;
		case wxEVENT_CREATELAYER:
			PostEvent(wxEVENT_CREATELAYER);
			break;
		}
}
