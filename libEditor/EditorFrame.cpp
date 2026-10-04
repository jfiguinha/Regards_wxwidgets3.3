#include <header.h>
#include "EditorFrame.h"
#include <window_id.h>
#include "ImageDocument.h"
#include <LibResource.h>
#include <wx/filename.h>
#include <wx/artprov.h> // Pour utiliser des icônes système par défaut
#include <ColorDialog.h>
#include <EffectsDialog.h>
#include <InfoDialog.h>
#include <LayerDialog.h>
#include <ParameterDialog.h>
#define MAX_ZOOM	10.0
#define MIN_ZOOM	0.1

// --- Énumération locale pour les IDs du menu Window ---
enum {
    ID_WINDOW_EFFECTS = wxID_HIGHEST + 1,
    ID_WINDOW_TOOLS,
    ID_WINDOW_INFOS,
    ID_WINDOW_COLOR,
    ID_WINDOW_PARAMETER,
	ID_WINDOW_LAYER
};

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
    // Note : Suppression de this->Maximize() pour permettre à Fit() d'ajuster
    // la frame au plus proche si c'est une barre d'outils flottante.
    // Si vous souhaitez qu'elle soit maximisée malgré tout, décommentez la ligne ci-dessous :
    // this->Maximize();

    viewerParam = CMainParamInit::getInstance();
    viewerTheme = CMainThemeInit::getInstance();
    this->mainInterface = mainInterface;

    // 1. Configuration de la barre de menus
    wxMenu* menuFile = new wxMenu;
    menuFile->Append(wxID_NEW, "Nouvelle Image...\tCtrl+N");
    menuFile->Append(wxID_EXIT, "Quitter\tCtrl+Q");

    // --- NOUVEAU : Création du menu Window ---
    wxMenu* menuWindow = new wxMenu;
    menuWindow->Append(ID_WINDOW_EFFECTS, "Effets");
    menuWindow->Append(ID_WINDOW_TOOLS, "Tools");
    menuWindow->Append(ID_WINDOW_INFOS, "Infos");
    menuWindow->Append(ID_WINDOW_COLOR, "Color");
    menuWindow->Append(ID_WINDOW_PARAMETER, "Parameter");
    menuWindow->Append(ID_WINDOW_LAYER, "Layer");

    wxMenuBar* menuBar = new wxMenuBar;
    menuBar->Append(menuFile, "&Fichier");
    menuBar->Append(menuWindow, "&Window"); // Ajout du menu à la barre globale
    SetMenuBar(menuBar);

    // 2. Création de la barre d'outils (Toolbar)
    wxToolBar* toolBar = CreateToolBar(wxTB_HORIZONTAL | wxTB_FLAT);

    wxBitmap bmpNew = wxArtProvider::GetBitmap(wxART_NEW, wxART_TOOLBAR);
    wxBitmap bmpQuit = wxArtProvider::GetBitmap(wxART_QUIT, wxART_TOOLBAR);

    toolBar->AddTool(wxID_NEW, "Nouvelle Image", bmpNew, "Créer une nouvelle zone de dessin");
    toolBar->AddSeparator();
    toolBar->AddTool(wxID_EXIT, "Quitter", bmpQuit, "Fermer l'application");

    toolBar->Realize();

    // 3. Création de la barre de statut (Status Bar)
    wxStatusBar* statusBar = CreateStatusBar(1);
    SetStatusText("Prêt. Aucun document ouvert.");

    // Ajustement de la taille de la Frame à sa barre d'outils et ses menus
    Fit();
    SetMinSize(GetSize());

    // Liaison des événements de base
    Bind(wxEVT_MENU, &CEditorFrame::OnNewImage, this, wxID_NEW);
    Bind(wxEVT_MENU, &CEditorFrame::OnQuit, this, wxID_EXIT);

    // --- NOUVEAU : Liaison des événements du menu Window ---
    Bind(wxEVT_MENU, &CEditorFrame::OnWindowEffects, this, ID_WINDOW_EFFECTS);
    Bind(wxEVT_MENU, &CEditorFrame::OnWindowTools, this, ID_WINDOW_TOOLS);
    Bind(wxEVT_MENU, &CEditorFrame::OnWindowInfos, this, ID_WINDOW_INFOS);
    Bind(wxEVT_MENU, &CEditorFrame::OnWindowColor, this, ID_WINDOW_COLOR);
    Bind(wxEVT_MENU, &CEditorFrame::OnWindowParameter, this, ID_WINDOW_PARAMETER);
    Bind(wxEVT_MENU, &CEditorFrame::OnWindowLayer, this, ID_WINDOW_LAYER);
}

void CEditorFrame::OnClose(wxCloseEvent& event)
{
    if (mainInterface)
        mainInterface->Close();
    Exit();
}

void CEditorFrame::OnOpen()
{
    wxString openPicture = CLibResource::LoadStringFromResource(L"LBLOPENPICTUREFILE", 1);

    wxFileDialog openFileDialog(nullptr, openPicture, lastFolder, "",
        "*.*", wxFD_OPEN | wxFD_FILE_MUST_EXIST);

    if (openFileDialog.ShowModal() == wxID_CANCEL)
        return;

    wxFileName filename(openFileDialog.GetPath());
    lastFolder = filename.GetPath();

    static int imageCount = 1;
    imageCount++;

    CMainTheme* viewerTheme = CMainThemeInit::getInstance();
    ImageDocument* imgDoc = new ImageDocument(this, SHOWBITMAPVIEWERID, BITMAPWINDOWVIEWERID, nullptr, viewerTheme, openFileDialog.GetPath());
    imgDoc->Show(true);
    m_openedDocuments.push_back(imgDoc);
}

// --- NOUVEAU : Gestionnaires d'événements pour le menu Window ---

void CEditorFrame::OnWindowEffects(wxCommandEvent& event)
{
    SetStatusText("Ouverture de la boîte des Effets...");
    // TODO: Instancier et afficher le dialogue EffectsDialog ici
    if(!effectsDialog)
    {
		effectsDialog = new EffectsDialog(this);
        effectsDialog->SetFilename(m_activeDocument ? m_activeDocument->GetFileName() : "");
    }
    effectsDialog->Show(true);
}

void CEditorFrame::OnWindowTools(wxCommandEvent& event)
{
    SetStatusText("Ouverture des Outils...");
    // TODO: Instancier et afficher le dialogue de sélection d'outils ici
}

void CEditorFrame::OnWindowInfos(wxCommandEvent& event)
{
    SetStatusText("Affichage des Informations du document...");
    if(!infoDialog)
    {
		infoDialog = new InfoDialog(this);
        infoDialog->SetFilename(m_activeDocument ? m_activeDocument->GetFileName() : "");  
    }
    infoDialog->Show(true); 
}

void CEditorFrame::OnWindowColor(wxCommandEvent& event)
{
    SetStatusText("Ouverture de la palette de Couleurs...");
	if (!colorDialog)
	{
        colorDialog = new ColorDialog(this);
	}
	colorDialog->Show(true);
}

void CEditorFrame::OnWindowLayer(wxCommandEvent& event)
{
	SetStatusText("Ouverture de la gestion des Couches...");
	if (!layerDialog)
	{
		layerDialog = new LayerDialog(this);
	}
	layerDialog->Show(true);
}

void CEditorFrame::OnWindowParameter(wxCommandEvent& event)
{
    SetStatusText("Ouverture des Paramètres...");
	if (!parameterDialog)
	{
        parameterDialog = new ParameterDialog(this);
	}
    parameterDialog->Show(true);
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
    OnOpen();
}
