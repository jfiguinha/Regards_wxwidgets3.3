#include <header.h>
#include "FiltreToolbar.h"
#include <LibResource.h>
#include <window_id.h>
using namespace Regards::Control;

#define WM_OK 1
#define WM_CANCEL 2

CFiltreToolbar::CFiltreToolbar(wxWindow* parent, wxWindowID id, const CThemeToolbar& theme, const bool& vertical)
	: CToolbarWindow(parent, id, theme, vertical)
{
	numFiltre = 0;
	wxString libelleOk = CLibResource::LoadStringFromResource(L"IDS_LBLOK", 1);
	wxString libelleCancel = CLibResource::LoadStringFromResource(L"IDS_LBLCANCEL", 1);

	ok = std::make_unique<CToolbarButton>(themeToolbar.button);
	ok->SetButtonResourceId(L"IDB_OK");
	ok->SetCommandId(WM_OK);
	ok->SetLibelle(libelleOk);
	navElement.push_back(ok.get());

	cancel = std::make_unique<CToolbarButton>(themeToolbar.button);
	cancel->SetButtonResourceId(L"IDB_CANCEL");
	cancel->SetCommandId(WM_CANCEL);
	cancel->SetLibelle(libelleCancel);
	navElement.push_back(cancel.get());
}


CFiltreToolbar::~CFiltreToolbar()
{
}


void CFiltreToolbar::SetNumFiltre(const int& numFiltre)
{
	this->numFiltre = numFiltre;
}

void CFiltreToolbar::EventManager(const int& id)
{
	switch (id)
	{
	case WM_OK:
		{
			wxCommandEvent evt(wxEVENT_FILTREOK);
			evt.SetInt(numFiltre);
			GetParent()->GetEventHandler()->AddPendingEvent(evt);
		}
		break;

	case WM_CANCEL:
		{
			wxCommandEvent evt(wxEVENT_FILTRECANCEL);
			evt.SetInt(numFiltre);
			GetParent()->GetEventHandler()->AddPendingEvent(evt);
		}
		break;

	default:
		break;
	}
}
