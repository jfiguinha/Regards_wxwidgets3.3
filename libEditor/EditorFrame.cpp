#include <header.h>
#include "EditorFrame.h"
#include <window_id.h>
#include "ImageDocument.h"
#include <LibResource.h>
#include <wx/filename.h>
#include <wx/artprov.h> // Pour utiliser des icônes système par défaut
#include <wx/xrc/xmlres.h>
#include <InfoDialog.h>
#include <LayerDialog.h>
#include <ParameterDialog.h>
#include <HistoryDialog.h>
#include <wx/spinctrl.h>
#include <effect_id.h>
#include <FilterData.h>
#define MAX_ZOOM	10.0
#define MIN_ZOOM	0.1

// --- Énumération locale pour les IDs du menu Window ---
enum {
    ID_WINDOW_EFFECTS = wxID_HIGHEST + 1,
    ID_WINDOW_TOOLS,
    ID_WINDOW_INFOS,
    ID_WINDOW_COLOR,
    ID_WINDOW_PARAMETER,
	ID_WINDOW_LAYER,
    ID_WINDOW_HISTORY,
    wxID_ZOOMIN,
	wxID_ZOOMOUT,
    wxID_SHRINK,
    wxID_REALSIZE,
    wxID_CROP,
    wxID_RESIZE,
    wxID_CANVASSIZE,
    wxID_FLIPVERTICAL,
    wxID_FLIPHORIZONTAL,
    wxID_ROTATE90,
    wxID_ROTATE180,
    wxID_ROTATE270
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

    colorDialog = new CColorPickerDialog(this);
    layerDialog = new LayerDialog(this);
    infoDialog = new InfoDialog(this);
    parameterDialog = new ParameterDialog(this);
	historyDialog = new HistoryDialog(this);

    historyDialog->Show(false);
    colorDialog->Show(false);
    layerDialog->Show(false);
	infoDialog->Show(false);    
    parameterDialog->Show(false);


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
    menuWindow->Append(ID_WINDOW_HISTORY, "History");

    wxMenu* menuEdit = new wxMenu;
    menuEdit->Append(wxID_UNDO, "Cancel\tCtrl+Z");
    menuEdit->Append(wxID_REDO, "Redo\tCtrl+Y");

    wxMenu* menuDisplay = new wxMenu;
    menuDisplay->Append(wxID_ZOOMIN, "Zoom In\tCtrl++");
    menuDisplay->Append(wxID_ZOOMOUT, "Zoom Out\tCtrl+-");
    menuDisplay->Append(wxID_SHRINK, "Shrink");
    menuDisplay->Append(wxID_REALSIZE, "Real Size");

    wxMenu* menuPicture = new wxMenu;
    menuPicture->Append(wxID_CROP, "Crop");
    menuPicture->Append(wxID_RESIZE, "Resize");
    menuPicture->Append(wxID_CANVASSIZE, "Canvas Size");
    menuPicture->AppendSeparator();
    menuPicture->Append(wxID_FLIPVERTICAL, "Flip Vertical");
    menuPicture->Append(wxID_FLIPHORIZONTAL, "Flip Horizontal");
    menuPicture->AppendSeparator();
    menuPicture->Append(wxID_ROTATE90, "Rotate 90");
    menuPicture->Append(wxID_ROTATE180, "Rotate 180");
    menuPicture->Append(wxID_ROTATE270, "Rotate 270");


    wxString colorEffect;
    wxString convolutionEffect;
    wxString specialEffect;
    wxString histogramEffect;
    colorEffect = CLibResource::LoadStringFromResource("LBLCOLOREFFECT", 1);
    convolutionEffect = CLibResource::LoadStringFromResource("LBLCONVOLUTIONEFFECT", 1);
    specialEffect = CLibResource::LoadStringFromResource("LBLSPECIALEFFECT", 1);
    histogramEffect = CLibResource::LoadStringFromResource("LBLHISTOGRAMEFFECT", 1);


    wxMenu* menuEffect = new wxMenu;
    wxMenu* menuColor = new wxMenu;
    wxMenu* menuConvolution = new wxMenu;
    wxMenu* menuSpecialEffect = new wxMenu;
    wxMenu* menuHistogramEffect = new wxMenu;


    for (int numEffect = FILTER_START; numEffect < FILTER_END; numEffect++)
    {
		int numMenu = wxID_HIGHEST + numEffect + 100;
        int typeEffect = CFiltreData::GetTypeEffect(numEffect);
        switch (typeEffect) {
        case SPECIAL_EFFECT:
            menuSpecialEffect->Append(numMenu, CFiltreData::GetFilterLabel(numEffect));
            break;
        case COLOR_EFFECT:
            menuColor->Append(numMenu, CFiltreData::GetFilterLabel(numEffect));
            break;
        case CONVOLUTION_EFFECT:
            menuConvolution->Append(numMenu, CFiltreData::GetFilterLabel(numEffect));
            break;
        case HISTOGRAM_EFFECT:
            menuHistogramEffect->Append(numMenu, CFiltreData::GetFilterLabel(numEffect));
            break;
        }

        Bind(wxEVT_MENU, &CEditorFrame::OnSelectEffect, this, numMenu);
    }

    menuEffect->Append(0, "Color", menuColor);
    menuEffect->Append(1, "Convolution", menuConvolution);
    menuEffect->Append(2, "Special Effect", menuSpecialEffect);
    menuEffect->Append(3, "Histogram", menuHistogramEffect);


    wxMenuBar* menuBar = new wxMenuBar;
    menuBar->Append(menuFile, "&Files");
    menuBar->Append(menuEdit, "&Edit");
    menuBar->Append(menuDisplay, "&Display");
    menuBar->Append(menuPicture, "&Picture");
 //  menuBar->Append(menuEdit, "&Ajustment");
    menuBar->Append(menuEffect, "&Effect");
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
    Bind(wxEVT_MENU, &CEditorFrame::OnWindowTools, this, ID_WINDOW_TOOLS);
    Bind(wxEVT_MENU, &CEditorFrame::OnWindowInfos, this, ID_WINDOW_INFOS);
    Bind(wxEVT_MENU, &CEditorFrame::OnWindowColor, this, ID_WINDOW_COLOR);
    Bind(wxEVT_MENU, &CEditorFrame::OnWindowParameter, this, ID_WINDOW_PARAMETER);
    Bind(wxEVT_MENU, &CEditorFrame::OnWindowLayer, this, ID_WINDOW_LAYER);
    Bind(wxEVT_MENU, &CEditorFrame::OnWindowHistory, this, ID_WINDOW_HISTORY);


    Bind(wxEVT_MENU, &CEditorFrame::OnWindowZoomIn, this, wxID_ZOOMIN);
    Bind(wxEVT_MENU, &CEditorFrame::OnWindowZoomOut, this, wxID_ZOOMOUT);
    Bind(wxEVT_MENU, &CEditorFrame::OnWindowShrink, this, wxID_SHRINK);
    Bind(wxEVT_MENU, &CEditorFrame::OnWindowRealSize, this, wxID_REALSIZE);

    Bind(wxEVT_MENU, &CEditorFrame::OnWindowCrop, this, wxID_CROP);
    Bind(wxEVT_MENU, &CEditorFrame::OnWindowResize, this, wxID_RESIZE);
    Bind(wxEVT_MENU, &CEditorFrame::OnWindowCanvas, this, wxID_CANVASSIZE);
    Bind(wxEVT_MENU, &CEditorFrame::OnWindowFlipVertical, this, wxID_FLIPVERTICAL);
    Bind(wxEVT_MENU, &CEditorFrame::OnWindowFlipHorizontal, this, wxID_FLIPHORIZONTAL);
    Bind(wxEVT_MENU, &CEditorFrame::OnWindowRotate90, this, wxID_ROTATE90);
    Bind(wxEVT_MENU, &CEditorFrame::OnWindowRotate180, this, wxID_ROTATE180);
    Bind(wxEVT_MENU, &CEditorFrame::OnWindowRotate270, this, wxID_ROTATE270);

    this->Maximize();
}

void CEditorFrame::OnSelectEffect(wxCommandEvent& event)
{
    if (m_activeDocument)
    {
        if (parameterDialog)
        {
            parameterDialog->SetFiltre(event.GetId() - wxID_HIGHEST - 100, historyDialog->GetHistoryEffectWnd(), m_activeDocument->GetFileName());
            parameterDialog->Show(true);
        }
        //m_activeDocument->Crop();
    }
}

void CEditorFrame::OnWindowCrop(wxCommandEvent& event)
{
    if (m_activeDocument)
    {
        //m_activeDocument->Crop();
    }
}

void CEditorFrame::OnWindowResize(wxCommandEvent& event)
{
    if (m_activeDocument)
    {
        wxDialog dlg;
        // Chargement de l'interface depuis le fichier XRC
        if (wxXmlResource::Get()->LoadDialog(&dlg, this, "ResizeDialog"))
        {
            // Récupération des contrôles par leur nom XRC
            wxSpinCtrl* spinPixelWidth = XRCCTRL(dlg, "m_spinPixelWidth", wxSpinCtrl);
            wxSpinCtrl* spinPixelHeight = XRCCTRL(dlg, "m_spinPixelHeight", wxSpinCtrl);
            wxComboBox* comboInterpolation = XRCCTRL(dlg, "m_comboInterpolation", wxComboBox);

            // Initialisation des valeurs par défaut
            if (spinPixelWidth) spinPixelWidth->SetValue(m_activeDocument->GetBitmapWidth());
            if (spinPixelHeight) spinPixelHeight->SetValue(m_activeDocument->GetBitmapHeight());
            if (comboInterpolation) comboInterpolation->SetSelection(1); // Bilinéaire par défaut

            // Liaison dynamique des fonctions sur modification (Bind)
            // dlg.Bind(wxEVT_SPINCTRL, &ImageDocument::OnPixelWidthChange, this...);

            if (dlg.ShowModal() == wxID_OK)
            {
                // Récupérer les valeurs finales saisies par l'utilisateur
                // Appliquer le redimensionnement...
            }
        }
    }
}

void CEditorFrame::OnWindowCanvas(wxCommandEvent& event)
{
    if (m_activeDocument)
    {
        //m_activeDocument->Canvas();
    }
}

void CEditorFrame::OnWindowFlipVertical(wxCommandEvent& event)
{
    if (m_activeDocument)
    {
        m_activeDocument->OnFlipVertical();
    }
}

void CEditorFrame::OnWindowFlipHorizontal(wxCommandEvent& event)
{
    if (m_activeDocument)
    {
        m_activeDocument->OnFlipHorizontal();
    }
}

void CEditorFrame::OnWindowRotate90(wxCommandEvent& event)
{
    if (m_activeDocument)
    {
        m_activeDocument->OnRotate90();
    }
}
void CEditorFrame::OnWindowRotate180(wxCommandEvent& event)
{
    if (m_activeDocument)
    {
        m_activeDocument->OnRotate180();
    }
}
void CEditorFrame::OnWindowRotate270(wxCommandEvent& event)
{
    if (m_activeDocument)
    {
        m_activeDocument->OnRotate270();
    }
}

void CEditorFrame::OnWindowZoomIn(wxCommandEvent& event)
{
    if (m_activeDocument)
    {
		m_activeDocument->ZoomIn();
    }
}

void CEditorFrame::OnWindowZoomOut(wxCommandEvent& event)
{
    if (m_activeDocument)
    {
        m_activeDocument->ZoomOut();
    }
}

void CEditorFrame::OnWindowShrink(wxCommandEvent& event)
{
    if (m_activeDocument)
    {
        m_activeDocument->Shrink();
    }
}

void CEditorFrame::OnWindowRealSize(wxCommandEvent& event)
{
    if (m_activeDocument)
    {
        m_activeDocument->RealSize();
    }
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

void CEditorFrame::SetActiveDocument(ImageDocument* doc)
{
    if (doc)
    {
        m_activeDocument = doc;
        infoDialog->SetFilename(m_activeDocument->GetFileName());
    }
}

void CEditorFrame::OnWindowHistory(wxCommandEvent& event)
{
    SetStatusText("Ouverture de l'Histoire...");
    if (!historyDialog)
    {
        historyDialog = new HistoryDialog(this);
    }
    historyDialog->Show(true);
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
       // infoDialog->SetFilename(m_activeDocument ? m_activeDocument->GetFileName() : "");  
    }
    infoDialog->Show(true); 
}

void CEditorFrame::OnWindowColor(wxCommandEvent& event)
{
    SetStatusText("Ouverture de la palette de Couleurs...");
	if (!colorDialog)
	{
        colorDialog = new CColorPickerDialog(this, wxColour(255, 128, 0, 255));
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
