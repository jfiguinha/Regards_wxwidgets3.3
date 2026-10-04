#pragma once
#include <MainInterface.h>
#include <EditorTheme.h>
#include <EditorThemeInit.h>
#include <EditorParamInit.h>
// IDs for the controls and the menu commands



class ImageDocumentDialog;

// Define a new frame type: this is going to be our main frame
class CEditorFrame : public wxFrame
{
public:
	// ctor(s)
	CEditorFrame(const wxString &title, const wxString& openfile, IMainInterface* mainInterface, const wxPoint &pos, const wxSize &size,
		long style = wxDEFAULT_FRAME_STYLE);

    ~CEditorFrame() = default;

	void OnOpen();
	void OnClose();

private:

	void Exit();
	void OnAbout(wxCommandEvent& WXUNUSED(event));
	void OnNewImage(wxCommandEvent& event);
	void OnQuit(wxCommandEvent& event);
	void OnClose(wxCloseEvent& event);

	std::vector<ImageDocumentDialog*> m_openedDocuments;
   
	Regards::Editor::CMainParam * viewerParam;
	Regards::Editor::CMainTheme * viewerTheme;

	wxString lastFolder = "";
	IMainInterface* mainInterface;
	DECLARE_EVENT_TABLE()
};
