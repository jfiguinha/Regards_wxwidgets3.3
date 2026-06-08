#include <header.h>
#include "ToolbarKeyword.h"
#include <LibResource.h>
#include "ListFace.h"
#include <window_id.h>
#include <SqlCriteria.h>
#include <KeywordDialogBox.h>

using namespace Regards::Viewer;
using namespace Regards::Window;
using namespace Regards::Sqlite;
#define WM_ADDKEYWORD 1023
#define WM_REMOVEKEYWORD 1024


CToolbarKeyword::CToolbarKeyword(wxWindow* parent, wxWindowID id, const CThemeToolbar& theme, const bool& vertical)
	: CToolbarWindow(parent, id, theme, vertical)
{
	themeToolbar = theme;
	add = CreateButton(L"IDB_PLUS", L"LBLADDKEYWORD", WM_ADDKEYWORD);
	libelle = CreateTexte(L"LBLADDKEYWORD", WM_ADDKEYWORD);
}

CToolbarKeyword::~CToolbarKeyword()
{
}


void CToolbarKeyword::EventManager(const int& id)
{
	switch (id)
	{
	case WM_ADDKEYWORD:
		{
			/*
			wxTextEntryDialog dlg(this, wxT("New Keyword : \n"),
				wxT("Create a new keyword"), 
				"", wxOK | wxCANCEL);

			if (dlg.ShowModal() == wxID_OK)
			{
			*/
			bool isNew = false;
			// We can be certain that this string contains letters only.
			wxString value = "New Keyword"; // dlg.GetValue();
			CSqlCriteria sqlCriteria;
			int lastId = sqlCriteria.GetCriteriaId() + 1;
			value = value + " " + to_string(lastId);
			sqlCriteria.GetOrInsertCriteriaId(1, 7, value, isNew);

			if (isNew)
			{
				wxWindow* mainWnd = this->FindWindowById(MAINVIEWERWINDOWID);
				auto eventChange = new wxCommandEvent(wxEVT_CRITERIACHANGE);
				wxQueueEvent(mainWnd, eventChange);

				wxWindow* keyword = this->FindWindowById(KEYWORDCRITERIAWINDOWID);
				auto eventRefresh = new wxCommandEvent(wxEVENT_REFRESHDATA);
				wxQueueEvent(keyword, eventRefresh);
			}
			//}
		}
		break;

	case WM_REMOVEKEYWORD:
		{
			KeywordDialogBox keywordDialog(this);
			keywordDialog.ShowModal();
			if (keywordDialog.IsOk())
			{
				wxWindow* mainWnd = this->FindWindowById(MAINVIEWERWINDOWID);
				auto eventChange = new wxCommandEvent(wxEVT_CRITERIACHANGE);
				wxQueueEvent(mainWnd, eventChange);

				wxWindow* keyword = this->FindWindowById(KEYWORDCRITERIAWINDOWID);
				auto eventRefresh = new wxCommandEvent(wxEVENT_REFRESHDATA);
				wxQueueEvent(keyword, eventRefresh);
			}
		}
		break;
	}
}
