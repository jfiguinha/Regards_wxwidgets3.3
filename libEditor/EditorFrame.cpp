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
#include <ToolDialog.h>
#include <wx/spinctrl.h>
#include <effect_id.h>
#include <FilterData.h>
#include <ResizeDialog.h>
#include <LayerPropertiesDialog.h>
#include <CanvasSizeDialog.h>
using namespace Regards::Dialog;

#define MAX_ZOOM	10.0
#define MIN_ZOOM	0.1

#define SHOWBITMAPVIEWERID 0x01000
#define BITMAPWINDOWVIEWERID 0x00100


#ifndef wxHAS_IMAGES_IN_RESOURCES
#ifdef __WXGTK__
#include "../Resource/sample.xpm"
#elif defined(__APPLE__)
#include "../Resource/sample.xpm"
#else
#include "../../Resource/sample.xpm"
#endif
#endif


static int imageCount = 1;


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
    wxID_ROTATE270,
    wxID_INVERSESELECT,
    wxID_CANCELSELECT
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

    SetIcon(wxICON(sample));

    CFiltreData::CreateFilterList();


    layerDialog = new LayerDialog(this);
    infoDialog = new InfoDialog(this);
    parameterDialog = new ParameterDialog(this);
	historyDialog = new HistoryDialog(this);
    toolDialog = new ToolDialog(this);

    toolDialog->Show(true);
    historyDialog->Show(false);
    layerDialog->Show(false);
	infoDialog->Show(false);    
    parameterDialog->Show(false);


    viewerParam = CMainParamInit::getInstance();
    viewerTheme = CMainThemeInit::getInstance();
    this->mainInterface = mainInterface;

    // 1. Configuration de la barre de menus
    wxMenu* menuFile = new wxMenu;
    menuFile->Append(wxID_NEW, "New ..\tCtrl+N");
    menuFile->Append(wxID_OPEN, "Open...\tCtrl+O");
    menuFile->AppendSeparator();
    menuFile->Append(wxID_SAVE, "Save...\tCtrl+S");
    menuFile->AppendSeparator();
    menuFile->Append(wxID_CLOSE, "Close\tCtrl+Q");
    menuFile->AppendSeparator();
    menuFile->Append(wxID_EXIT, "Exit\tCtrl+Q");

    // --- NOUVEAU : Création du menu Window ---
    wxMenu* menuWindow = new wxMenu;
    menuWindow->Append(ID_WINDOW_TOOLS, "Tools");
    menuWindow->Append(ID_WINDOW_INFOS, "Infos");
    menuWindow->Append(ID_WINDOW_PARAMETER, "Parameter");
    menuWindow->Append(ID_WINDOW_LAYER, "Layer");
    menuWindow->Append(ID_WINDOW_HISTORY, "History");

    wxMenu* menuEdit = new wxMenu;
    menuEdit->Append(wxID_UNDO, "Cancel\tCtrl+Z");
    menuEdit->Append(wxID_REDO, "Redo\tCtrl+Y");
    menuEdit->AppendSeparator();
    menuEdit->Append(wxID_COPY, "Copy\tCtrl+C");
    menuEdit->Append(wxID_CUT, "Cut\tCtrl+Z");
    menuEdit->Append(wxID_PASTE, "Paste\tCtrl+V");
    menuEdit->AppendSeparator();
    menuEdit->Append(wxID_SELECTALL, "Select All");
    menuEdit->Append(wxID_INVERSESELECT, "Inverse Selection");
    menuEdit->Append(wxID_CANCELSELECT, "Cancel Selection");

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
        default:
            switch (numEffect) {
            case IDM_WAVE_EFFECT:
                menuSpecialEffect->Append(numMenu, CFiltreData::GetFilterLabel(numEffect));
                break;
            case IDM_FILTRELENSFLARE:
                menuSpecialEffect->Append(numMenu, CFiltreData::GetFilterLabel(numEffect));
                break;
            case IDM_FILTRELENSCORRECTION:
                menuSpecialEffect->Append(numMenu, CFiltreData::GetFilterLabel(numEffect));
                break;
            }
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
    Bind(wxEVT_MENU, &CEditorFrame::OnOpenImage, this, wxID_OPEN);
    Bind(wxEVT_MENU, &CEditorFrame::OnSave, this, wxID_SAVE);
    Bind(wxEVT_MENU, &CEditorFrame::OnCloseImage, this, wxID_CLOSE);
    Bind(wxEVT_MENU, &CEditorFrame::OnQuit, this, wxID_EXIT);

    // --- NOUVEAU : Liaison des événements du menu Window ---
    Bind(wxEVT_MENU, &CEditorFrame::OnWindowTools, this, ID_WINDOW_TOOLS);
    Bind(wxEVT_MENU, &CEditorFrame::OnWindowInfos, this, ID_WINDOW_INFOS);
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

    Connect(wxEVENT_CREATELAYER, wxCommandEventHandler(CEditorFrame::CreateLayer));
    Connect(wxEVENT_DELETELAYER, wxCommandEventHandler(CEditorFrame::DeleteLayer));
    Connect(wxEVENT_COPYLAYER, wxCommandEventHandler(CEditorFrame::CopyLayer));
    Connect(wxEVENT_FUSIONLAYER, wxCommandEventHandler(CEditorFrame::FusionLayer));
    Connect(wxEVENT_MOVEUPLAYER, wxCommandEventHandler(CEditorFrame::MoveUpLayer));
    Connect(wxEVENT_MOVEDOWNLAYER, wxCommandEventHandler(CEditorFrame::MoveDownLayer));
    Connect(wxEVENT_PROPERTIESLAYER, wxCommandEventHandler(CEditorFrame::PropertiesLayer));

    Connect(wxEVENT_TOOLCHOOSE, wxCommandEventHandler(CEditorFrame::OnToolsEffect));
    Connect(wxEVENT_COLORCHANGE, wxCommandEventHandler(CEditorFrame::OnColorChange));
    this->Maximize();
}


void CEditorFrame::DeleteLayer(wxCommandEvent& event)
{
    if (m_activeDocument && layerDialog)
    {
        int numSelect = layerDialog->GetActifLayer();
        if (numSelect != -1)
        {
            const wxString msg = "Do you want delete this layer ?";
            const wxString info = CLibResource::LoadStringFromResource(L"labelInformations", 1);
            if (wxMessageBox(msg, info, wxYES_NO | wxICON_WARNING) == wxYES)
            {
                m_activeDocument->DeleteLayer(numSelect);
                layerDialog->RefreshList();
            }
        }
    }


}

void CEditorFrame::CopyLayer(wxCommandEvent& event)
{
    if (m_activeDocument)
    {
        int numSelect = layerDialog->GetActifLayer();
        if (numSelect != -1)
        {
            m_activeDocument->CopyLayer(numSelect);
            layerDialog->RefreshList();
        }
    }
}

void CEditorFrame::CreateLayer(wxCommandEvent& event)
{
    if (m_activeDocument)
    {
        m_activeDocument->CreateLayer("");
        layerDialog->RefreshList();
    }
}

void CEditorFrame::FusionLayer(wxCommandEvent& event)
{
    if (m_activeDocument)
    {
        vector<int> listLayer = layerDialog->GetSelectLayer();
        if (listLayer.size() > 0)
        {
            m_activeDocument->FusionLayer(&listLayer);
            layerDialog->RefreshList();
        }

    }
}

void CEditorFrame::MoveUpLayer(wxCommandEvent& event)
{
    if (m_activeDocument)
    {
        int numSelect = layerDialog->GetActifLayer();
        if (numSelect != -1)
        {
            m_activeDocument->MoveUpLayer(numSelect);
            layerDialog->RefreshList();
        }
    }
}

void CEditorFrame::MoveDownLayer(wxCommandEvent& event)
{
    if (m_activeDocument)
    {
        int numSelect = layerDialog->GetActifLayer();
        if (numSelect != -1)
        {
            m_activeDocument->MoveDownLayer(numSelect);
            layerDialog->RefreshList();
        }
    }
}

void CEditorFrame::PropertiesLayer(wxCommandEvent& event)
{
    if (m_activeDocument)
    {
        int numLayer = layerDialog->GetActifLayer();
        if (numLayer != -1)
        {
            CLayerPropertiesDialog dialog(
                this,
                "Mon calque",
                LayerBlendMode::Normal,
                80);

            if (dialog.ShowModal() == wxID_OK)
            {
                const wxString layerName = dialog.GetLayerName();
                const auto blendMode = dialog.GetBlendMode();
                const int opacity = dialog.GetOpacity();

                m_activeDocument->SetPropertiesLayer(numLayer, layerName, blendMode, opacity);
                layerDialog->RefreshList();
            }
        }
    }
}

void CEditorFrame::OnSelectEffect(wxCommandEvent& event)
{
    if (m_activeDocument)
    {
        if (parameterDialog)
        {
            int numEffect = event.GetId() - wxID_HIGHEST - 100;
            parameterDialog->SetTitle(CFiltreData::GetFilterLabel(numEffect));
           
            parameterDialog->SetTypeFiltre(TYPE_EFFECT);
            parameterDialog->SetFiltre(numEffect, historyDialog->GetHistoryEffectWnd(), m_activeDocument->GetFileName(), m_activeDocument->GetBitmapViewerId(), m_activeDocument->GetMainViewerId());
           
            parameterDialog->SetColor(toolDialog->GetColor1(), toolDialog->GetColor2());
            parameterDialog->Show(true);
        }
        //m_activeDocument->Crop();
    }
}

void CEditorFrame::OnColorChange(wxCommandEvent& event)
{
    if (m_activeDocument)
    {
        if (parameterDialog)
        {
            parameterDialog->SetColor(toolDialog->GetColor1(), toolDialog->GetColor2());
        }
        //m_activeDocument->Crop();
    }
}


void CEditorFrame::OnToolsEffect(wxCommandEvent& event)
{
    if (m_activeDocument)
    {
        if (parameterDialog)
        {
            int numEffect = event.GetInt();
            parameterDialog->SetTitle(CFiltreData::GetFilterLabel(numEffect));
            
            parameterDialog->SetTypeFiltre(TYPE_DRAWING);
            parameterDialog->SetFiltre(numEffect, historyDialog->GetHistoryEffectWnd(), m_activeDocument->GetFileName(), m_activeDocument->GetBitmapViewerId(), m_activeDocument->GetMainViewerId());
           
            parameterDialog->SetColor(toolDialog->GetColor1(), toolDialog->GetColor2());
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
        ResizeParameter resizeParam;
        resizeParam.height = m_activeDocument->GetBitmapHeight();
        resizeParam.width = m_activeDocument->GetBitmapWidth();
        resizeParam.interpolation = 0;

        CResizeDialog resizeDlg(this, resizeParam);
        resizeDlg.ShowModal();
        if (resizeDlg.IsOk())
        {
            resizeParam = resizeDlg.GetParameter();
            m_activeDocument->Resize(resizeParam.width, resizeParam.height, resizeParam.interpolation);
        }
    }
}

void CEditorFrame::OnWindowCanvas(wxCommandEvent& event)
{
    if (m_activeDocument)
    {
        CanvasSizeParameter canvasSize;
        canvasSize.height = m_activeDocument->GetBitmapHeight();
        canvasSize.width = m_activeDocument->GetBitmapWidth();
        CCanvasSizeDialog canvasDlg(this, canvasSize);
        canvasDlg.ShowModal();
        if (canvasDlg.IsOk())
        {
            canvasSize = canvasDlg.GetParameter();
            //m_activeDocument->Resize(resizeParam.width, resizeParam.height, resizeParam.interpolation);
        }
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

    imageCount++;


    CMainTheme* viewerTheme = CMainThemeInit::getInstance();
    ImageDocument* imgDoc = new ImageDocument(this, SHOWBITMAPVIEWERID + imageCount, BITMAPWINDOWVIEWERID + imageCount, nullptr, viewerTheme, openFileDialog.GetPath());
    imgDoc->Show(true);

    STImageDoc imageDoc;
    imageDoc.doc = imgDoc;
	imageDoc.mainId = SHOWBITMAPVIEWERID + imageCount;
	imageDoc.bitmapId = BITMAPWINDOWVIEWERID + imageCount;
    m_openedDocuments[imageDoc.mainId] = imageDoc;

    m_activeDocument = imgDoc;
    historyDialog->SetHistoryControl(m_activeDocument->GetHistoryPt());
    infoDialog->SetFilename(m_activeDocument->GetFileName());
    layerDialog->SetLayerList(m_activeDocument->GetListOfLayer());
}

void CEditorFrame::SetActiveDocument(ImageDocument* doc)
{
    if (doc)
    {
        m_activeDocument = doc;
        wxString filename = m_activeDocument->GetFileName();
        if(filename != historyDialog->GetFilename())
            historyDialog->SetHistoryControl(doc->GetHistoryPt());

        if (filename != infoDialog->GetFilename())
            infoDialog->SetFilename(m_activeDocument->GetFileName());

        if (filename != infoDialog->GetFilename())
            layerDialog->SetLayerList(m_activeDocument->GetListOfLayer());
       // layerDialog->SetBitmapEditor(m_activeDocument->GetBitmapEditor());
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
    if (!toolDialog)
    {
        toolDialog = new ToolDialog(this);
        // infoDialog->SetFilename(m_activeDocument ? m_activeDocument->GetFileName() : "");  
    }
    toolDialog->Show(true);
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
    wxDialog dlg;
    // Chargement de l'interface depuis le fichier XRC
    if (wxXmlResource::Get()->LoadDialog(&dlg, this, "OpenDialog"))
    {
        wxSize screenSize = wxGetDisplaySize();
        int largeur = screenSize.GetWidth();
        int hauteur = screenSize.GetHeight();


        // Récupération des contrôles par leur nom XRC
        wxSpinCtrl* spinPixelWidth = XRCCTRL(dlg, "m_spinPixelWidth", wxSpinCtrl);
        wxSpinCtrl* spinPixelHeight = XRCCTRL(dlg, "m_spinPixelHeight", wxSpinCtrl);

        // Initialisation des valeurs par défaut
        if (spinPixelWidth) spinPixelWidth->SetValue(largeur);
        if (spinPixelHeight) spinPixelHeight->SetValue(hauteur);


        // Liaison dynamique des fonctions sur modification (Bind)
        // dlg.Bind(wxEVT_SPINCTRL, &ImageDocument::OnPixelWidthChange, this...);

        if (dlg.ShowModal() == wxID_OK)
        {
            int width = spinPixelWidth->GetValue();
            int height = spinPixelHeight->GetValue();

            cv::Mat matRGBA(hauteur, largeur, CV_8UC4);
            CImageLoadingFormat* pictureLocal = new CImageLoadingFormat();
            pictureLocal->SetFilename("Test 1");
            pictureLocal->SetPicture(matRGBA);


            imageCount++;


            CMainTheme* viewerTheme = CMainThemeInit::getInstance();
            ImageDocument* imgDoc = new ImageDocument(this, SHOWBITMAPVIEWERID + imageCount, BITMAPWINDOWVIEWERID + imageCount, nullptr, viewerTheme, pictureLocal);
            imgDoc->Show(true);

            STImageDoc imageDoc;
            imageDoc.doc = imgDoc;
            imageDoc.mainId = SHOWBITMAPVIEWERID + imageCount;
            imageDoc.bitmapId = BITMAPWINDOWVIEWERID + imageCount;
            m_openedDocuments[imageDoc.mainId] = imageDoc;
        }
    }
}

void CEditorFrame::OnOpenImage(wxCommandEvent& event)
{
    OnOpen();
}

void CEditorFrame::OnSave(wxCommandEvent& event)
{
    if (m_activeDocument)
    {
        m_activeDocument->Save();
    }
}

void CEditorFrame::OnCloseImage(wxCommandEvent& event)
{
    if (m_activeDocument)
    {
        m_activeDocument->Close();

    }
}