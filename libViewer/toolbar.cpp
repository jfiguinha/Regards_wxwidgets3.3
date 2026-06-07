#include <header.h>
#include "Toolbar.h"
#include <ToolbarButton.h>
#include <LibResource.h>
#include "ViewerFrame.h"
#include <window_id.h>
#include <RegardsConfigParam.h>
#include <ParamInit.h>

#include <wx/mimetype.h>
using namespace Regards::Window;
using namespace Regards::Viewer;

//
#define IDM_WINDOWSEARCH 152
#define IDM_THUMBNAILFACE 153
#define IDM_VIEWERMODE 154
#define IDM_EXPLORERMODE 155
#define IDM_SHOWINFOS 156
#define IDM_QUITTER 157
#define IDM_PRINT 158
#define IDM_SCANNER 159
#define IDM_PICTUREMODE 160
#define IDM_EDIT 161
#define IDM_EXPORT 162
#define IDM_NEWVERSION 163
#define IDM_EXPORT_DIAPORAMA 164



CToolbar::CToolbar(wxWindow* parent, wxWindowID id, const CThemeToolbar& theme, const bool& vertical)
	: CToolbarWindow(parent, id, theme, vertical)
{
	int faceDetection = 0;
	wxString export_label = CLibResource::LoadStringFromResource(L"LBLEXPORT", 1);
	wxString export_diaporama = CLibResource::LoadStringFromResource(L"LBLEXPORTDIAPORAMA", 1);
	wxString lblOpenFolder = CLibResource::LoadStringFromResource(L"LBLSELECTFILE", 1);
	wxString lblInfos = CLibResource::LoadStringFromResource(L"LBLINFOS", 1);
	wxString lblQuit = CLibResource::LoadStringFromResource(L"LBLQUIT", 1);
	wxString lblPrint = CLibResource::LoadStringFromResource(L"LBLPRINT", 1);
	wxString lblScanner = CLibResource::LoadStringFromResource(L"LBLSCANNER", 1);
	wxString lblEditor = CLibResource::LoadStringFromResource(L"LBLEDITORMODE", 1);
	wxString lblNewVersion = CLibResource::LoadStringFromResource(L"LBLUPDATE", 1);

	scanner = std::make_unique<CToolbarButton>(themeToolbar.button);
	scanner->SetButtonResourceId(L"IDB_SCANNER");
	scanner->SetLibelle(lblScanner);
	scanner->SetCommandId(IDM_SCANNER);
	navElement.push_back(scanner.get());

	print = std::make_unique<CToolbarButton>(themeToolbar.button);
	print->SetButtonResourceId(L"IDB_PRINTERPNG");
	print->SetLibelle(lblPrint);
	print->SetCommandId(IDM_PRINT);
	navElement.push_back(print.get());


	editor = std::make_unique<CToolbarButton>(themeToolbar.button);
	editor->SetButtonResourceId(L"IDB_OPEN");
	editor->SetLibelle(lblEditor);
	editor->SetCommandId(IDM_EDIT);
	navElement.push_back(editor.get());

	export_button = std::make_unique<CToolbarButton>(themeToolbar.button);
	export_button->SetButtonResourceId("IDB_EXPORT");
	export_button->SetLibelle(export_label);
	export_button->SetCommandId(IDM_EXPORT);
	export_button->SetLibelleTooltip(export_label);
	navElement.push_back(export_button.get());

	export_diaporama_button = std::make_unique<CToolbarButton>(themeToolbar.button);
	export_diaporama_button->SetButtonResourceId("IDB_MOVIE");
	export_diaporama_button->SetLibelle(export_diaporama);
	export_diaporama_button->SetCommandId(IDM_EXPORT_DIAPORAMA);
	export_diaporama_button->SetLibelleTooltip(export_diaporama);
	navElement.push_back(export_diaporama_button.get());

	imageNewVersion = std::make_unique<CToolbarButton>(themeToolbar.button);
	imageNewVersion->SetButtonResourceId(L"IDB_REFRESH");
	imageNewVersion->SetLibelle(lblNewVersion);
	imageNewVersion->SetVisible(false);
	imageNewVersion->SetCommandId(IDM_NEWVERSION);
	navElement.push_back(imageNewVersion.get());

	imageFirst = std::make_unique<CToolbarButton>(themeToolbar.button);
	imageFirst->SetButtonResourceId(L"IDB_EXIT");
	imageFirst->SetLibelle(lblQuit);
	imageFirst->SetCommandId(IDM_QUITTER);
	navElement.push_back(imageFirst.get());	
	
}

CToolbar::~CToolbar()
{
}

void CToolbar::SetUpdateVisible(const bool& isVisible)
{
	imageNewVersion->SetVisible(isVisible);
	this->Refresh();
}

void CToolbar::EventManager(const int& id)
{
	switch (id)
	{
	case IDM_WINDOWSEARCH:
		{
			wxWindow* central = this->FindWindowById(PHOTOSEEARCHPANEL);
			auto event = new wxCommandEvent(wxEVENT_SHOWPANE);
			wxQueueEvent(central, event);
		}
		break;

	case IDM_SHOWINFOS:
		{
			wxWindow* central = this->FindWindowById(PANELCLICKINFOSWNDID);
			auto event = new wxCommandEvent(wxEVENT_SHOWPANE);
			wxQueueEvent(central, event);
		}
		break;

	case IDM_PRINT:
		{
			wxWindow* central = this->FindWindowById(MAINVIEWERWINDOWID);
			auto event = new wxCommandEvent(wxEVENT_PRINT);
			wxQueueEvent(central, event);
		}
		break;

	case IDM_VIEWERMODE:
		{
			wxWindow* central = this->FindWindowById(CENTRALVIEWERWINDOWID);
			wxCommandEvent event(wxEVENT_SETMODEVIEWER);
			event.SetInt(1);
			wxPostEvent(central, event);
		}
		break;

#ifndef __NOFACE_DETECTION__
	case IDM_THUMBNAILFACE:
		{
			wxWindow* central = this->FindWindowById(CENTRALVIEWERWINDOWID);
			wxCommandEvent event(wxEVENT_SETMODEVIEWER);
			event.SetInt(2);
			wxPostEvent(central, event);
		}
		break;
#endif
	case IDM_EXPLORERMODE:
		{
			wxWindow* central = this->FindWindowById(CENTRALVIEWERWINDOWID);
			wxCommandEvent event(wxEVENT_SETMODEVIEWER);
			event.SetInt(3);
			wxPostEvent(central, event);
		}
		break;

	case IDM_PICTUREMODE:
		{
			wxWindow* central = this->FindWindowById(CENTRALVIEWERWINDOWID);
			wxCommandEvent event(wxEVENT_SETMODEVIEWER);
			event.SetInt(4);
			wxPostEvent(central, event);
		}
		break;

	case IDM_SCANNER:
		{
			wxWindow* central = this->FindWindowById(MAINVIEWERWINDOWID);
			auto event = new wxCommandEvent(wxEVENT_SHOWSCANNER);
			wxQueueEvent(central, event);
			break;
		}

	case IDM_EDIT:
		{
			wxWindow* central = this->FindWindowById(MAINVIEWERWINDOWID);
			auto event = new wxCommandEvent(wxEVENT_EDITFILE);
			wxQueueEvent(central, event);
			break;
		}

	case IDM_EXPORT:
		{
			wxWindow* central = this->FindWindowById(MAINVIEWERWINDOWID);
			auto event = new wxCommandEvent(wxEVENT_EXPORTFILE);
			wxQueueEvent(central, event);
			break;
		}

	case IDM_EXPORT_DIAPORAMA:
		{
			wxWindow* central = this->FindWindowById(MAINVIEWERWINDOWID);
			auto event = new wxCommandEvent(wxEVENT_EXPORTDIAPORAMA);
			wxQueueEvent(central, event);
			break;
		}

	case IDM_QUITTER:
		{
			wxWindow* central = this->FindWindowById(MAINVIEWERWINDOWID);
			auto event = new wxCommandEvent(wxEVT_EXIT);
			wxQueueEvent(central, event);
		}
		break;

	case IDM_NEWVERSION:
		{
			wxString siteweb = CLibResource::LoadStringFromResource("SITEWEB", 1);
			wxMimeTypesManager manager;
			wxFileType* filetype = manager.GetFileTypeFromExtension("html");
			wxString command = filetype->GetOpenCommand(siteweb);
			wxExecute(command);
		}
		break;
	}
}
