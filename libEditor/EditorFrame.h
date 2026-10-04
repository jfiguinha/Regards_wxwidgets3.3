#pragma once
#include <MainInterface.h>
#include <EditorTheme.h>
#include <EditorThemeInit.h>
#include <EditorParamInit.h>
// IDs for the controls and the menu commands
#include "ColorPickerDialog.h"

class LayerDialog;
class EffectsDialog;
class InfoDialog;
class ParameterDialog;
class ImageDocument;

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

	// Permet aux ImageDocument de notifier la frame principale qu'ils ont le focus
	void SetActiveDocument(ImageDocument* doc) { m_activeDocument = doc; }

	// Permet à vos boîtes de dialogue (Effets, Couleurs...) de récupérer l'image active
	ImageDocument* GetActiveDocument() const { return m_activeDocument; }

private:

	void Exit();
	void OnAbout(wxCommandEvent& WXUNUSED(event));
	void OnNewImage(wxCommandEvent& event);
	void OnQuit(wxCommandEvent& event);
	void OnClose(wxCloseEvent& event);
	void OnWindowEffects(wxCommandEvent& event);
	void OnWindowTools(wxCommandEvent& event);
	void OnWindowInfos(wxCommandEvent& event);
	void OnWindowColor(wxCommandEvent& event);
	void OnWindowParameter(wxCommandEvent& event);
	void OnWindowLayer(wxCommandEvent& event);
	std::vector<ImageDocument*> m_openedDocuments;
   
	Regards::Editor::CMainParam * viewerParam;
	Regards::Editor::CMainTheme * viewerTheme;

	ImageDocument* m_activeDocument = nullptr; 
	CColorPickerDialog * colorDialog = nullptr;
	LayerDialog* layerDialog = nullptr;
	EffectsDialog* effectsDialog = nullptr;
	InfoDialog* infoDialog = nullptr;	
	ParameterDialog* parameterDialog = nullptr;
	wxString lastFolder = "";
	IMainInterface* mainInterface;
	DECLARE_EVENT_TABLE()
};
