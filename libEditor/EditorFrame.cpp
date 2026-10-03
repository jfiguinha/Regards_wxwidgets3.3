#include <header.h>
#include "EditorFrame.h"
#include <window_id.h>
#include "ImageDocumentDialog.h"
#include <wx/artprov.h> // Pour utiliser des icônes système par défaut
#define MAX_ZOOM	10.0
#define MIN_ZOOM	0.1


//Connect(wxEVT_MOVE, wxMoveEventHandler(Move::OnMove));
BEGIN_EVENT_TABLE(CEditorFrame, wxFrame)
		EVT_CLOSE(CEditorFrame::OnCloseWindow)
END_EVENT_TABLE()



using namespace Regards::Editor;

// ----------------------------------------------------------------------------
// main frame
// ----------------------------------------------------------------------------

// frame constructor
CEditorFrame::CEditorFrame(const wxString& title, const wxString& openfile, IMainInterface* mainInterface, const wxPoint& pos,
                             const wxSize& size,
                             long style) :
	wxFrame(nullptr, FRAMEEDITOR_ID, title, pos, size, style)
{

    this->Maximize();

	viewerParam = CMainParamInit::getInstance();
	viewerTheme = CMainThemeInit::getInstance();
	this->mainInterface = mainInterface;	
	// create a menu bar
        // Configuration de la barre de menus
    wxMenu* menuFile = new wxMenu;
    menuFile->Append(wxID_NEW, "Nouvelle Image...\tCtrl+N");
    menuFile->Append(wxID_EXIT, "Quitter\tCtrl+Q");

    wxMenuBar* menuBar = new wxMenuBar;
    menuBar->Append(menuFile, "&Fichier");
    SetMenuBar(menuBar);

    // 2. Création de la barre d'outils (Toolbar)
    // wxTB_HORIZONTAL place la barre horizontalement (comportement par défaut)
    // wxTB_FLAT donne un look moderne sans bordures épaisses
    wxToolBar* toolBar = CreateToolBar(wxTB_HORIZONTAL | wxTB_FLAT);

    // Récupération d'icônes standards du système pour l'exemple
    wxBitmap bmpNew = wxArtProvider::GetBitmap(wxART_NEW, wxART_TOOLBAR);
    wxBitmap bmpQuit = wxArtProvider::GetBitmap(wxART_QUIT, wxART_TOOLBAR);

    // Ajout des outils à la barre
    toolBar->AddTool(wxID_NEW, "Nouvelle Image", bmpNew, "Créer une nouvelle zone de dessin");
    toolBar->AddSeparator();
    toolBar->AddTool(wxID_EXIT, "Quitter", bmpQuit, "Fermer l'application");

    // IMPORTANT : Toujours appeler Realize() après avoir ajouté des outils
    toolBar->Realize();

    // 3. Création de la barre de statut (Status Bar)
// Par défaut, elle contient un seul champ de texte libre
    wxStatusBar* statusBar = CreateStatusBar(1);

    // Texte initial affiché au démarrage
    SetStatusText("Prêt. Aucun document ouvert.");


    // Liaison des événements
    Bind(wxEVT_MENU, &CEditorFrame::OnNewImage, this, wxID_NEW);
    Bind(wxEVT_MENU, &CEditorFrame::OnQuit, this, wxID_EXIT);

}

void CEditorFrame::OnClose(wxCloseEvent& event)
{
	if (mainInterface)
		mainInterface->Close();
	Exit();
}

int CEditorFrame::OnOpen()
{
	int value = -1;

	return value;
}


// event handlers

void CEditorFrame::OnQuit(wxCommandEvent& WXUNUSED(event))
{
	Exit();
}

void CEditorFrame::Exit()
{
	if (mainInterface != nullptr)
		mainInterface->Close();
	exit(0);
}



void CEditorFrame::OnAbout(wxCommandEvent& WXUNUSED(event))
{
	if (mainInterface != nullptr)
		mainInterface->ShowAbout();
}

void CEditorFrame::OnNewImage(wxCommandEvent& event) {
	static int imageCount = 1;
	wxString name = wxString::Format("Sans titre %d", imageCount++);

	// Création et affichage du dialogue d'image de manière non-modale
	ImageDocumentDialog* imgDoc = new ImageDocumentDialog(this, name);
	imgDoc->Show(true);
	m_openedDocuments.push_back(imgDoc);
}

