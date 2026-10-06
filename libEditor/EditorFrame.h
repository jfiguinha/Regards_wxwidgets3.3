#pragma once
#include <MainInterface.h>
#include <ViewerParam.h>
#include <ViewerParamInit.h>
#include <ViewerTheme.h>
#include <ViewerThemeInit.h>
using namespace Regards::Viewer;

class LayerDialog;
class EffectsDialog;
class InfoDialog;
class ParameterDialog;
class ImageDocument;
class HistoryDialog;
class ToolDialog;

struct STImageDoc
{
	ImageDocument* doc;
	wxWindowID mainId;
	wxWindowID bitmapId;
};

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
	void SetActiveDocument(ImageDocument* doc);

	// Permet à vos boîtes de dialogue (Effets, Couleurs...) de récupérer l'image active
	ImageDocument* GetActiveDocument() const { return m_activeDocument; }

private:

	void OnColorChange(wxCommandEvent& event);
	void OnAbout(wxCommandEvent& WXUNUSED(event));
	void OnToolsEffect(wxCommandEvent& event);
	void OnNewImage(wxCommandEvent& event);
	void OnOpenImage(wxCommandEvent& event);
	void OnCloseImage(wxCommandEvent& event);
	void OnSave(wxCommandEvent& event);
	void OnQuit(wxCommandEvent& event);
	void OnClose(wxCloseEvent& event);
	void Exit();
	
	void OnWindowEffects(wxCommandEvent& event);
	void OnWindowTools(wxCommandEvent& event);
	void OnWindowInfos(wxCommandEvent& event);

	void OnWindowParameter(wxCommandEvent& event);
	void OnWindowLayer(wxCommandEvent& event);
	void OnWindowHistory(wxCommandEvent& event);
	void OnWindowZoomIn(wxCommandEvent& event);
	void OnWindowZoomOut(wxCommandEvent& event);
	void OnWindowShrink(wxCommandEvent& event);
	void OnWindowRealSize(wxCommandEvent& event);

	void OnWindowCrop(wxCommandEvent& event);
	void OnWindowResize(wxCommandEvent& event);
	void OnWindowCanvas(wxCommandEvent& event);
	void OnWindowFlipVertical(wxCommandEvent& event);
	void OnWindowFlipHorizontal(wxCommandEvent& event);
	void OnWindowRotate90(wxCommandEvent& event);
	void OnWindowRotate180(wxCommandEvent& event);
	void OnWindowRotate270(wxCommandEvent& event);
	void OnSelectEffect(wxCommandEvent& event);


	std::map<int, STImageDoc> m_openedDocuments;
   
	Regards::Viewer::CMainParam * viewerParam;
	Regards::Viewer::CMainTheme * viewerTheme;

	ImageDocument* m_activeDocument = nullptr; 

	LayerDialog* layerDialog = nullptr;
	ToolDialog* toolDialog = nullptr;
	EffectsDialog* effectsDialog = nullptr;
	InfoDialog* infoDialog = nullptr;	
	ParameterDialog* parameterDialog = nullptr;
	HistoryDialog* historyDialog = nullptr;
	wxString lastFolder = "";
	IMainInterface* mainInterface;
	DECLARE_EVENT_TABLE()
};
