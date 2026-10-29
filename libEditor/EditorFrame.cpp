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
#include <wx/display.h>
#include <algorithm>
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
    wxID_CANCELSELECT,
    wxID_EXPORT,
    WXPRINT_PAGE_SETUP,
    WXPRINT_PAGE_SETUP_PS,
#ifdef __WXMAC__
    WXPRINT_PAGE_MARGINS
#endif
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

    printService_ = std::make_unique<Regards::Control::CPrintService>(this);
    layerDialog = new LayerDialog(this);
    infoDialog = new InfoDialog(this);
    parameterDialog = new ParameterDialog(this);
    historyDialog = new HistoryDialog(this);
    toolDialog = new ToolDialog(this);

    // La frame n'a pas encore sa géométrie finale (Maximize() en fin de constructeur) :
    // la boîte à outils est ancrée à gauche une fois la frame affichée.
    CallAfter([this]() { ShowDocked(toolDialog, DockSide::Left); });
    historyDialog->Show(false);
    layerDialog->Show(false);
    infoDialog->Show(false);
    parameterDialog->Show(false);


    viewerParam = CMainParamInit::getInstance();
    viewerTheme = CMainThemeInit::getInstance();
    this->mainInterface = mainInterface;

    const wxString labelPageSetup = CLibResource::LoadStringFromResource(L"labelPageSetup", 1);
    const wxString labelPageSetup_link = CLibResource::LoadStringFromResource(L"labelPageSetup_link", 1);
    const wxString labelPageMargins = CLibResource::LoadStringFromResource(L"labelPageMargins", 1); // L"Page margins";
    const wxString labelPageMargins_link = CLibResource::LoadStringFromResource(L"labelPageMargins_link", 1);
    // 1. Configuration de la barre de menus
    wxMenu* menuFile = new wxMenu;
    menuFile->Append(wxID_NEW, "New ..\tCtrl+N");
    menuFile->Append(wxID_OPEN, "Open...\tCtrl+O");
    menuFile->AppendSeparator();
    menuFile->Append(wxID_SAVE, "Save...\tCtrl+S");
    menuFile->Append(wxID_EXPORT, "Export...\tCtrl+S");
    menuFile->AppendSeparator();
    menuFile->Append(WXPRINT_PAGE_SETUP, labelPageSetup_link, labelPageSetup);
#ifdef __WXMAC__
    menuFile->Append(WXPRINT_PAGE_MARGINS, labelPageMargins_link, labelPageMargins);
#endif
    menuFile->Append(wxID_PRINT, wxT("&Print..."), wxT("Print"));
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
    Bind(wxEVT_MENU, &CEditorFrame::OnExport, this, wxID_EXPORT);
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

    Bind(wxEVT_MENU, &CEditorFrame::OnUndo, this, wxID_UNDO);
    Bind(wxEVT_MENU, &CEditorFrame::OnRedo, this, wxID_REDO);

    Bind(wxEVT_MENU, &CEditorFrame::OnCopy, this, wxID_COPY);
    Bind(wxEVT_MENU, &CEditorFrame::OnPaste, this, wxID_PASTE);
    Bind(wxEVT_MENU, &CEditorFrame::OnCut, this, wxID_CUT);

    Bind(wxEVT_MENU, &CEditorFrame::OnSelectAll, this, wxID_SELECTALL);
    Bind(wxEVT_MENU, &CEditorFrame::OnInverseAll, this, wxID_INVERSESELECT);
    Bind(wxEVT_MENU, &CEditorFrame::OnCancelSelect, this, wxID_CANCELSELECT);


    Bind(wxEVT_MENU, &CEditorFrame::OnPrint, this, wxID_PRINT);
    Bind(wxEVT_MENU, &CEditorFrame::OnPageSetup, this, WXPRINT_PAGE_SETUP);
    Connect(wxEVENT_CREATELAYER, wxCommandEventHandler(CEditorFrame::CreateLayer));
    Connect(wxEVENT_DELETELAYER, wxCommandEventHandler(CEditorFrame::DeleteLayer));
    Connect(wxEVENT_COPYLAYER, wxCommandEventHandler(CEditorFrame::CopyLayer));
    Connect(wxEVENT_FUSIONLAYER, wxCommandEventHandler(CEditorFrame::FusionLayer));
    Connect(wxEVENT_MOVEUPLAYER, wxCommandEventHandler(CEditorFrame::MoveUpLayer));
    Connect(wxEVENT_MOVEDOWNLAYER, wxCommandEventHandler(CEditorFrame::MoveDownLayer));
    Connect(wxEVENT_PROPERTIESLAYER, wxCommandEventHandler(CEditorFrame::PropertiesLayer));
    Connect(wxEVENT_UPDATELAYERPICTURE, wxCommandEventHandler(CEditorFrame::UpdateLayerPicture));
    Connect(wxEVENT_TOOLCHOOSE, wxCommandEventHandler(CEditorFrame::OnToolsEffect));
    Connect(wxEVENT_COLORCHANGE, wxCommandEventHandler(CEditorFrame::OnColorChange));
    Connect(wxEVENT_CHANGELAYERPICTURE, wxCommandEventHandler(CEditorFrame::OnLayerChange));
    Connect(wxEVENT_REFRESHLAYERLIST, wxCommandEventHandler(CEditorFrame::OnRefreshList));
    this->Maximize();

    UpdateMenusState();
}

void CEditorFrame::OnRefreshList(wxCommandEvent& event)
{
    if (m_activeDocument && layerDialog)
        layerDialog->SetLayerList(m_activeDocument->GetListOfLayer());
}

void CEditorFrame::OnSelectAll(wxCommandEvent& event)
{
    if (m_activeDocument)
        m_activeDocument->SelectAll();
}

void CEditorFrame::OnInverseAll(wxCommandEvent& event)
{
    if (m_activeDocument)
        m_activeDocument->InverseAll();
}

void CEditorFrame::UpdateMenusState()
{
    bool hasImage = (m_activeDocument != nullptr);
    bool canUndo = false;
    bool canRedo = false;
    bool hasSelection = false;
    bool canPaste = false;

    // 1. Récupération des états depuis le document actif et son éditeur graphique
    if (hasImage)
    {
        // États de l'historique (Undo / Redo) via le ModificationManager

        CModificationManager* modManager = m_activeDocument->GetModificationManager();
        if (modManager)
        {
            canUndo = modManager->CanUndo();
            canRedo = modManager->CanRedo();
        }

        // États de la sélection et du collage via CBitmapEditor
        CBitmapEditor* bitmapEditor = m_activeDocument->GetBitmapEditor();
        if (bitmapEditor)
        {
            hasSelection = bitmapEditor->IsSelectionEnable();
            canPaste = bitmapEditor->IsPasteEnable();
        }
    }

    // 2. Parcours et mise à jour de la barre de menus
    wxMenuBar* menuBar = GetMenuBar();
    if (menuBar)
    {
        for (size_t i = 0; i < menuBar->GetMenuCount(); ++i)
        {
            wxMenu* menu = menuBar->GetMenu(i);
            if (!menu) continue;

            wxMenuItemList& items = menu->GetMenuItems();
            for (wxMenuItemList::iterator it = items.begin(); it != items.end(); ++it)
            {
                wxMenuItem* item = *it;
                if (!item || item->IsSeparator()) continue;

                int id = item->GetId();

                if (item->IsSubMenu())
                {
                    item->Enable(hasImage);
                }
                else
                {
                    switch (id)
                    {
                    case wxID_OPEN:
                    case wxID_EXIT:
                        item->Enable(true);
                        break;

                    case wxID_UNDO:
                        item->Enable(canUndo);
                        break;

                    case wxID_REDO:
                        item->Enable(canRedo);
                        break;

                    case wxID_COPY:
                    case wxID_CUT:
                    case wxID_INVERSESELECT: // NOUVEAU : Activé si une sélection existe
                    case wxID_CANCELSELECT:  // NOUVEAU : Activé si une sélection existe
                        item->Enable(hasSelection);
                        break;

                    case wxID_PASTE:
                        item->Enable(canPaste);
                        break;

                    default:
                        item->Enable(hasImage);
                        break;
                    }
                }
            }
        }
    }

    // 3. Mise à jour de la barre d'outils (Toolbar)
    wxToolBar* toolBar = GetToolBar();
    if (toolBar)
    {
        toolBar->EnableTool(wxID_NEW, hasImage);
        toolBar->EnableTool(wxID_EXIT, true);
        toolBar->Refresh();
    }
}


void CEditorFrame::OnUndo(wxCommandEvent& event)
{
    if (m_activeDocument && historyDialog)
        historyDialog->Undo();
}

void CEditorFrame::OnRedo(wxCommandEvent& event)
{
    if (m_activeDocument && historyDialog)
        historyDialog->Redo();
}

void CEditorFrame::OnCancelSelect(wxCommandEvent& event)
{
    if (m_activeDocument)
        m_activeDocument->CancelSelect();
}

void CEditorFrame::UpdateLayerPicture(wxCommandEvent& event)
{
    if (m_activeDocument)
    {
        layerDialog->UpdateLayerList();

    }
}


void CEditorFrame::OnCopy(wxCommandEvent& event)
{
    if (m_activeDocument)
        m_activeDocument->Copy();
}

void CEditorFrame::OnPaste(wxCommandEvent& event)
{
    if (m_activeDocument)
        m_activeDocument->Paste();
}

void CEditorFrame::OnCut(wxCommandEvent& event)
{
    if (m_activeDocument)
        m_activeDocument->Cut();
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
                layerDialog->UpdateLayerList();
            }
        }
    }
}



void CEditorFrame::PrintPreview(const wxString& filename)
{
    printService_->ShowImagePreview(filename);
}

void CEditorFrame::PrintImagePreview(cv::Mat& picture)
{
    printService_->ShowMatrixPreview(picture);
}


void CEditorFrame::CopyLayer(wxCommandEvent& event)
{
    if (m_activeDocument)
    {
        int numSelect = layerDialog->GetActifLayer();
        if (numSelect != -1)
        {
            m_activeDocument->CopyLayer(numSelect);
            layerDialog->UpdateLayerList();
        }
    }
}

void CEditorFrame::CreateLayer(wxCommandEvent& event)
{
    if (m_activeDocument)
    {

        m_activeDocument->CreateLayer("My New Layer ");
        layerDialog->UpdateLayerList();
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
            layerDialog->UpdateLayerList();
        }

    }
}

void CEditorFrame::MoveUpLayer(wxCommandEvent& event)
{
    if (m_activeDocument)
    {
        CLayerList* list = m_activeDocument->GetListOfLayer();
        int numSelect = layerDialog->GetActifLayer();
        if (numSelect != -1 && numSelect < list->size() - 1)
        {
            m_activeDocument->MoveUpLayer(numSelect);
            layerDialog->UpdateLayerList();
            layerDialog->SetActifLayer(numSelect + 1);
        }
    }
}

void CEditorFrame::MoveDownLayer(wxCommandEvent& event)
{
    if (m_activeDocument)
    {
        int numSelect = layerDialog->GetActifLayer();
        if (numSelect > 0)
        {
            m_activeDocument->MoveDownLayer(numSelect);
            layerDialog->UpdateLayerList();
            layerDialog->SetActifLayer(numSelect - 1);
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
            int numLayerId = layerDialog->GetActifLayerId();

            CLayerList * list = m_activeDocument->GetListOfLayer();
            if (list)
            {
                LayerElement* layerElement = list->FindElementByLayerId(numLayerId);
                if (layerElement)
                {
                    CLayerPropertiesDialog dialog(
                        this,
                        layerElement->layerName,
                        LayerBlendMode::Normal,
                        80);

                    if (dialog.ShowModal() == wxID_OK)
                    {
                        const wxString layerName = dialog.GetLayerName();
                        const auto blendMode = dialog.GetBlendMode();
                        const int opacity = dialog.GetOpacity();

                        m_activeDocument->SetPropertiesLayer(numLayer, layerName, blendMode, opacity);
                        layerDialog->UpdateLayerList();
                    }
                }
            }
           


        }
    }
}

void CEditorFrame::OnSelectEffect(wxCommandEvent& event)
{
    if (m_activeDocument && layerDialog)
    {
        int numLayer = layerDialog->GetActifLayer();
        int64_t numLayerId = layerDialog->GetActifLayerId();
        if (parameterDialog)
        {
            int numEffect = event.GetId() - wxID_HIGHEST - 100;
            parameterDialog->SetTitle(CFiltreData::GetFilterLabel(numEffect));
            parameterDialog->SetActifLayer(numLayer);
            parameterDialog->SetActifLayerId(numLayerId);
            parameterDialog->SetTypeFiltre(TYPE_EFFECT);
            parameterDialog->SetFiltre(numEffect, numLayerId, historyDialog->GetHistoryEffectWnd(), m_activeDocument->GetFileName(), m_activeDocument->GetBitmapViewerId(), m_activeDocument->GetMainViewerId());

            parameterDialog->SetColor(toolDialog->GetColor1(), toolDialog->GetColor2());
            ShowDocked(parameterDialog, DockSide::Right);
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
        if (parameterDialog && layerDialog)
        {
            int numLayer = layerDialog->GetActifLayer();
            int64_t numLayerId = layerDialog->GetActifLayerId();
            if (numLayer != -1)
            {
                int numEffect = event.GetInt();
                m_activeDocument->SetActifLayer(numLayer);
                switch (numEffect)
                {
                case IDM_PAINTCROP:
                {
                    numEffect = IDM_CROP;
                    parameterDialog->SetTitle(CFiltreData::GetFilterLabel(numEffect));
                    parameterDialog->SetActifLayer(numLayer);
                    parameterDialog->SetActifLayerId(numLayerId);
                    parameterDialog->SetListLayer(m_activeDocument->GetListOfLayer());
                    parameterDialog->SetTypeFiltre(IDM_CROP);
                    parameterDialog->SetFiltre(numEffect, numLayerId, historyDialog->GetHistoryEffectWnd(), m_activeDocument->GetFileName(), m_activeDocument->GetBitmapViewerId(), m_activeDocument->GetMainViewerId());
                    parameterDialog->SetColor(toolDialog->GetColor1(), toolDialog->GetColor2());
                    parameterDialog->Show(false);
                }
                break;

                case IDM_LASSOSELECT:
                case IDM_ELLIPSESELECT:
                case IDM_RECTANGLESELECT:
                {
                    parameterDialog->SetTitle(CFiltreData::GetFilterLabel(numEffect));
                    parameterDialog->SetActifLayer(numLayer);
                    parameterDialog->SetListLayer(m_activeDocument->GetListOfLayer());
                    parameterDialog->SetActifLayerId(numLayerId);
                    parameterDialog->SetTypeFiltre(numEffect);
                    parameterDialog->SetFiltre(numEffect, numLayerId, historyDialog->GetHistoryEffectWnd(), m_activeDocument->GetFileName(), m_activeDocument->GetBitmapViewerId(), m_activeDocument->GetMainViewerId());
                    parameterDialog->SetColor(toolDialog->GetColor1(), toolDialog->GetColor2());
                    parameterDialog->Show(false);
                    break;
                }
                case WM_SELECTZOOM:
                    m_activeDocument->SetZoomTool();
                    break;
                case WM_MOVEPICTURE:
                    m_activeDocument->SetMoveTool();
                    break;

                case WM_SELECTMOVE:
                    m_activeDocument->SetSelectMoveTool();
                    break;

                default:
                    parameterDialog->SetTitle(CFiltreData::GetFilterLabel(numEffect));
                    parameterDialog->SetListLayer(m_activeDocument->GetListOfLayer());
                    parameterDialog->SetActifLayer(numLayer);
                    parameterDialog->SetActifLayerId(numLayerId);
                    parameterDialog->SetTypeFiltre(TYPE_DRAWING);
                    parameterDialog->SetFiltre(numEffect, numLayerId, historyDialog->GetHistoryEffectWnd(), m_activeDocument->GetFileName(), m_activeDocument->GetBitmapViewerId(), m_activeDocument->GetMainViewerId());
                    parameterDialog->SetColor(toolDialog->GetColor1(), toolDialog->GetColor2());
                    ShowDocked(parameterDialog, DockSide::Right);
                }


            }
        }
        //m_activeDocument->Crop();
    }
}

void CEditorFrame::OnWindowCrop(wxCommandEvent& event)
{
    if (m_activeDocument)
    {
        m_activeDocument->Crop();
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

void CEditorFrame::OnLayerChange(wxCommandEvent& event)
{
    if (m_activeDocument)
    {
        int numLayer = event.GetInt();
        m_activeDocument->SetActifLayer(numLayer);
        parameterDialog->SetActifLayer(numLayer);
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
    ImageDocument* imgDoc = new ImageDocument(this, LayerElement::GetLayerId(), SHOWBITMAPVIEWERID + imageCount, BITMAPWINDOWVIEWERID + imageCount, nullptr, viewerTheme, openFileDialog.GetPath());
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

    UpdateMenusState();
}

void CEditorFrame::SetActiveDocument(ImageDocument* doc)
{
    if (doc)
    {
        m_activeDocument = doc;

        wxString filename = m_activeDocument->GetFileName();
        if (filename != historyDialog->GetFilename())
            historyDialog->SetHistoryControl(doc->GetHistoryPt());

        if (filename != infoDialog->GetFilename())
            infoDialog->SetFilename(m_activeDocument->GetFileName());

        if (filename != infoDialog->GetFilename())
            layerDialog->SetLayerList(m_activeDocument->GetListOfLayer());

        m_activeDocument->SetActifLayer(layerDialog->GetActifLayer());

        UpdateMenusState();
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
    ShowDocked(historyDialog, DockSide::Right);
}


void CEditorFrame::OnWindowTools(wxCommandEvent& event)
{
    SetStatusText("Ouverture des Outils...");
    if (!toolDialog)
    {
        toolDialog = new ToolDialog(this);
        // infoDialog->SetFilename(m_activeDocument ? m_activeDocument->GetFileName() : "");  
    }
    ShowDocked(toolDialog, DockSide::Left);
}

void CEditorFrame::OnWindowInfos(wxCommandEvent& event)
{
    SetStatusText("Affichage des Informations du document...");
    if (!infoDialog)
    {
        infoDialog = new InfoDialog(this);
        // infoDialog->SetFilename(m_activeDocument ? m_activeDocument->GetFileName() : "");  
    }
    ShowDocked(infoDialog, DockSide::Right);
}

void CEditorFrame::OnWindowLayer(wxCommandEvent& event)
{
    SetStatusText("Ouverture de la gestion des Couches...");
    if (!layerDialog)
    {
        layerDialog = new LayerDialog(this);
    }
    ShowDocked(layerDialog, DockSide::Right);
}

void CEditorFrame::OnWindowParameter(wxCommandEvent& event)
{
    SetStatusText("Ouverture des Paramètres...");
    if (!parameterDialog)
    {
        parameterDialog = new ParameterDialog(this);
    }
    ShowDocked(parameterDialog, DockSide::Right);
}

void CEditorFrame::ShowDocked(wxWindow* dialog, DockSide side)
{
    if (!dialog)
        return;

    // Placement uniquement à la première ouverture : on respecte ensuite
    // la position choisie par l'utilisateur.
    if (m_dockedDialogs.insert(dialog).second)
    {
        // Origine de la zone cliente (sous la barre d'outils), en coordonnées écran
        const wxPoint origin = ClientToScreen(wxPoint(0, 0));
        const wxSize clientSize = GetClientSize();
        const wxSize dialogSize = dialog->GetSize();

        int x = (side == DockSide::Left)
            ? origin.x
            : origin.x + clientSize.GetWidth() - dialogSize.GetWidth();
        int y = origin.y;

        // Rester dans la zone utile de l'écran
        int displayIndex = wxDisplay::GetFromWindow(this);
        if (displayIndex == wxNOT_FOUND)
            displayIndex = 0;
        const wxRect area = wxDisplay(displayIndex).GetClientArea();

        x = std::max(area.GetLeft(), std::min(x, area.GetRight() - dialogSize.GetWidth() + 1));
        y = std::max(area.GetTop(), std::min(y, area.GetBottom() - dialogSize.GetHeight() + 1));

        dialog->Move(x, y);
    }

    dialog->Show(true);
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
            ImageDocument* imgDoc = new ImageDocument(this, LayerElement::GetLayerId(), SHOWBITMAPVIEWERID + imageCount, BITMAPWINDOWVIEWERID + imageCount, nullptr, viewerTheme, pictureLocal);
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

            UpdateMenusState();
        }
    }
}

void CEditorFrame::OnOpenImage(wxCommandEvent& event)
{
    OnOpen();
}

void CEditorFrame::OnExport(wxCommandEvent& event)
{
    if (m_activeDocument)
    {
        m_activeDocument->Export();
    }
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


void CEditorFrame::OnPrint(wxCommandEvent&)
{
    if (m_activeDocument)
    {
        CLayerList* listLayer = m_activeDocument->GetListOfLayer();
        if (listLayer)
        {
            CImageLoadingFormat* image = listLayer->GetPictureToShow();
            printService_->ShowMatrixPreview(image->GetMatImage());
        }
    }

}

void CEditorFrame::OnPageSetup(wxCommandEvent&)
{
    printService_->ShowPageSetup();
}

#ifdef __WXMAC__
void CViewerFrame::OnPageMargins(wxCommandEvent&)
{
    printService_->ShowPageMargins();
}
#endif