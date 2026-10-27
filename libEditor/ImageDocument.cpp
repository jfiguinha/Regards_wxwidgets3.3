#include <header.h>
#include "ImageDocument.h"
#include <libPicture.h>
#include <ModificationManager.h>
#include <FileUtility.h>
#include <EditorFrame.h>
#include <LibResource.h>
#include <ImageLoadingFormat.h>
#include <SavePicture.h>
#include <ViewerTheme.h>
#include <ParamInit.h>
#include <ViewerThemeInit.h>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>

using namespace Regards::Viewer;
using namespace Regards::Picture;


// --- Valeurs de zoom partagées ---
const std::vector<int> zoomValues = {
    1, 2, 3, 4, 5, 6, 8, 12, 16, 25, 33, 50, 66, 75, 100,
    133, 150, 166, 200, 300, 400, 500, 600, 700, 800, 1200, 1600
};

namespace
{
    // Retourne le parent de la fenêtre s'il s'agit d'un CEditorFrame, nullptr sinon
    CEditorFrame* GetEditorFrame(const wxWindow* window)
    {
        return dynamic_cast<CEditorFrame*>(window->GetParent());
    }

    // Demande à l'utilisateur le chemin du projet (.rgd).
    // Retourne une chaîne vide si l'utilisateur annule.
    wxString AskProjectSavePath(wxWindow* parent, const wxString& currentFilename)
    {
        const wxFileName inputFilename(currentFilename);

        // Filtre explicite au format wxWidgets pour que l'extension soit appliquée
        wxFileDialog saveFileDialog(parent,
            CLibResource::LoadStringFromResource("LBLSAVEFILE", 1), // Titre de la boîte
            wxEmptyString,
            inputFilename.GetName(),
            "Regards Project (*.rgd)|*.rgd",
            wxFD_SAVE | wxFD_OVERWRITE_PROMPT);

        const wxString documentPath = CFileUtility::GetDocumentFolderPath();
        if (!documentPath.empty())
            saveFileDialog.SetDirectory(documentPath);

        if (saveFileDialog.ShowModal() != wxID_OK)
            return wxEmptyString;

        // Chemin complet sélectionné par l'utilisateur
        wxString fullSavePath = saveFileDialog.GetPath();

        // Sécurité : s'assurer que le fichier possède bien l'extension .rgd
        wxFileName saveFile(fullSavePath);
        if (saveFile.GetExt().Lower() != "rgd")
        {
            saveFile.SetExt("rgd");
            fullSavePath = saveFile.GetFullPath();
        }

        return fullSavePath;
    }
}  // namespace


// ---------------------------------------------------------
// Helpers d'accès à l'éditeur / aux calques
// ---------------------------------------------------------

// Exécute fn(CBitmapEditor&) si l'éditeur existe
template <typename Fn>
void ImageDocument::WithEditor(Fn&& fn)
{
    if (bitmapWindow)
        fn(*bitmapWindow);
}

// Idem WithEditor, puis met à jour la barre de statut
template <typename Fn>
void ImageDocument::WithEditorAndStatus(Fn&& fn)
{
    WithEditor(std::forward<Fn>(fn));
    UpdateStatusText();
}

// Exécute fn(CLayerList&) si la liste de calques existe
template <typename Fn>
void ImageDocument::WithLayers(Fn&& fn)
{
    if (CLayerList* layers = GetListOfLayer())
        fn(*layers);
}


// ---------------------------------------------------------
// Construction
// ---------------------------------------------------------

ImageDocument::ImageDocument(wxWindow* parent, const int64_t& layerId, wxWindowID bitmapViewerId,
    wxWindowID mainViewerId, CBitmapInterface* bitmapInterfaceIn, CThemeParam* config, const wxString& filename)
    : wxDialog(parent, wxID_ANY, filename, wxDefaultPosition, wxSize(600, 450),
        wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER | wxMAXIMIZE_BOX | wxMINIMIZE_BOX)
{
    CLibPicture libPicture;
    CImageLoadingFormat* pictureLocal = libPicture.LoadPicture(filename);

    InitDocument(layerId, bitmapViewerId, mainViewerId, bitmapInterfaceIn, config, pictureLocal, filename);
}

ImageDocument::ImageDocument(wxWindow* parent, const int64_t& layerId, wxWindowID bitmapViewerId,
    wxWindowID mainViewerId, CBitmapInterface* bitmapInterfaceIn, CThemeParam* config, CImageLoadingFormat* pictureLocal)
    : wxDialog(parent, wxID_ANY, pictureLocal->GetFilename(), wxDefaultPosition, wxSize(600, 450),
        wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER | wxMAXIMIZE_BOX | wxMINIMIZE_BOX)
{
    InitDocument(layerId, bitmapViewerId, mainViewerId, bitmapInterfaceIn, config, pictureLocal,
        pictureLocal->GetFilename());
}

// Initialisation commune aux deux constructeurs
void ImageDocument::InitDocument(const int64_t& layerId, wxWindowID bitmapViewerId, wxWindowID mainViewerId,
    CBitmapInterface* bitmapInterfaceIn, CThemeParam* config, CImageLoadingFormat* pictureLocal,
    const wxString& filename)
{
    wxString historyLibelle = CLibResource::LoadStringFromResource(L"LBLHISTORY", 1);
    // --- GÉNÉRATION D'UN ID UNIQUE POUR LE RÉPERTOIRE D'HISTORIQUE ---
    boost::uuids::uuid localid = boost::uuids::random_generator()();
    std::string uuid_str = boost::uuids::to_string(localid);
    wxString folder = wxString::FromUTF8(uuid_str.c_str());
    modificationManager = std::make_unique<CModificationManager>(folder);

    id = bitmapViewerId;
    this->mainviewerid = bitmapViewerId;

    CThemeBitmapWindow themeBitmap;
    if (config)
    {
        config->GetBitmapWindowTheme(&themeBitmap);
    }

    m_filename = filename;

    this->bitmapInterface = bitmapInterfaceIn;

    bitmapWindow = new CBitmapEditor(nullptr, mainViewerId, themeBitmap, bitmapInterface);
    bitmapWindow->SetTabValue(zoomValues);

    bitmapWindowRender = new CBitmapWnd3D(this, bitmapViewerId);
    bitmapWindowRender->SetBitmapRenderInterface(bitmapWindow);

    scrollbar = new CScrollbarWnd(this, bitmapWindowRender, wxID_ANY, "BitmapScroll");

    scrollbar->Show(true);
    bitmapWindowRender->Show(true);

    bitmapWindow->SetBitmap(pictureLocal);

    Connect(wxEVT_SIZE, wxSizeEventHandler(ImageDocument::OnSize));
    Connect(wxEVENT_UPDATESTATUSTEXT, wxCommandEventHandler(ImageDocument::OnUpdateStatusText));
    Connect(wxEVENT_REFRESH, wxCommandEventHandler(ImageDocument::OnRefresh));
    Connect(wxEVENT_RESIZE, wxCommandEventHandler(ImageDocument::OnResize));
    Connect(wxEVT_ERASE_BACKGROUND, wxEraseEventHandler(ImageDocument::OnEraseBackground));
    Connect(wxEVT_IDLE, wxIdleEventHandler(ImageDocument::OnIdle));
    Connect(wxEVT_ACTIVATE, wxActivateEventHandler(ImageDocument::OnActivate));

    CreateStatusBarControls();

    historyEffect = std::make_unique<CInfoEffect>(nullptr, modificationManager.get(), bitmapViewerId, m_filename);

    // Initialisation avec la liste des calques actuelle du visualiseur
    historyEffect->Init(pictureLocal, bitmapWindow->GetListOfLayer(), layerId, m_filename, historyLibelle);

    UpdateStatusText();
}

// Création manuelle de la status bar (taille, zoom) et du slider de zoom
void ImageDocument::CreateStatusBarControls()
{
    m_statusBar = new wxStatusBar(this, wxID_ANY);

    int widths[] = { -1, 120, 200 };
    m_statusBar->SetFieldsCount(3);
    m_statusBar->SetStatusWidths(3, widths);

    const int zoomCount = static_cast<int>(zoomValues.size());
    m_zoomSlider = new wxCustomSlider(m_statusBar, wxID_ANY, zoomCount, 0, zoomCount);

    // Index réel calculé après l'affectation du bitmap
    m_zoomSlider->SetValue(bitmapWindow->GetPosRatio());

    m_zoomSlider->Bind(wxEVT_CUSTOM_SLIDER_CHANGED, &ImageDocument::OnSliderScroll, this);
}

CInfoEffect* ImageDocument::GetHistoryPt()
{
    return historyEffect.get();
}


// ---------------------------------------------------------
// Historique
// ---------------------------------------------------------

void ImageDocument::Undo()
{
    if (modificationManager && modificationManager->Undo())
        RestoreLayersFromHistory();
}

void ImageDocument::Redo()
{
    if (modificationManager && modificationManager->Redo())
        RestoreLayersFromHistory();
}

// Applique à l'éditeur la liste de calques de la modification courante
void ImageDocument::RestoreLayersFromHistory()
{
    CLayerList* restoredLayers =
        modificationManager->GetLayersForModification(modificationManager->GetNumModification());

    if (!restoredLayers || !bitmapWindow)
        return;

    // Attribution de la nouvelle liste de calques à l'éditeur graphique
    bitmapWindow->SetListOfLayer(restoredLayers);
    needToRefresh = true;
    UpdateStatusText();

    // Notification vers CEditorFrame pour synchroniser l'arbre des calques du LayerDialog
    if (CEditorFrame* frame = GetEditorFrame(this))
    {
        wxCommandEvent evt(wxEVENT_UPDATELAYERPICTURE);
        frame->GetEventHandler()->AddPendingEvent(evt);
    }
}


// ---------------------------------------------------------
// Presse-papier / sélection
// ---------------------------------------------------------

bool ImageDocument::Copy()
{
    return bitmapWindow->Copy();
}

bool ImageDocument::Cut()
{
    return bitmapWindow->Cut();
}

bool ImageDocument::Paste()
{
    return bitmapWindow->Paste();
}

void ImageDocument::SelectAll()
{
    WithEditor([](auto& editor) { editor.SelectAll(); });
}

void ImageDocument::InverseAll()
{
    WithEditor([](auto& editor) { editor.InverseAll(); });
}

void ImageDocument::CancelSelect()
{
    WithEditor([](auto& editor) { editor.CancelSelect(); });
}


// ---------------------------------------------------------
// Calques
// ---------------------------------------------------------

void ImageDocument::SetPropertiesLayer(int numLayer, wxString layerName, LayerBlendMode blendMode, int opacity)
{
    WithLayers([&](CLayerList& layers) { layers.SetPropertiesLayer(numLayer, layerName, blendMode, opacity); });
}

void ImageDocument::SetActifLayer(const int& numLayer)
{
    WithEditor([&](auto& editor) { editor.SetActifLayer(numLayer); });
}

void ImageDocument::DeleteLayer(int numLayer)
{
    WithLayers([&](CLayerList& layers) { layers.erase(numLayer); });
}

void ImageDocument::CopyLayer(int numLayer)
{
    WithLayers([&](CLayerList& layers) { layers.copy(numLayer); });
}

void ImageDocument::CreateLayer(wxString layerName)
{
    WithLayers([&](CLayerList& layers)
        {
            CRegardsConfigParam* regardsParam = CParamInit::getInstance();
            const bool openCLAvailable = cv::ocl::haveOpenCL() && regardsParam->GetIsOpenCLSupport();
            layers.CreateLayer(layerName, openCLAvailable ? BackendType::UMat : BackendType::OpenGL);
        });
}

void ImageDocument::FusionLayer(vector<int>* listLayers)
{
    WithLayers([&](CLayerList& layers) { layers.FusionLayers(listLayers); });
}

void ImageDocument::MoveUpLayer(int numLayer)
{
    WithLayers([&](CLayerList& layers) { layers.MoveUpLayer(numLayer); });
}

void ImageDocument::MoveDownLayer(int numLayer)
{
    WithLayers([&](CLayerList& layers) { layers.MoveDownLayer(numLayer); });
}

CLayerList* ImageDocument::GetListOfLayer()
{
    return bitmapWindow ? bitmapWindow->GetListOfLayer() : nullptr;
}


// ---------------------------------------------------------
// Outils
// ---------------------------------------------------------

void ImageDocument::SetZoomTool()
{
    WithEditor([](auto& editor) { editor.SetZoomTool(); });
}

void ImageDocument::SetSelectMoveTool()
{
    WithEditor([](auto& editor) { editor.SetSelectMoveTool(); });
}

void ImageDocument::SetMoveTool()
{
    WithEditor([](auto& editor) { editor.SetMoveTool(); });
}


// ---------------------------------------------------------
// Fichier
// ---------------------------------------------------------

// Met à jour le nom du document (titre de la fenêtre et éditeur compris)
void ImageDocument::SetDocumentFilename(const wxString& filename)
{
    m_filename = filename;
    SetTitle(m_filename);
    WithEditor([&](auto& editor) { editor.SetFilename(m_filename); });
}

void ImageDocument::Save()
{
    CLayerList* layerList = GetListOfLayer();
    if (!layerList)
        return;

    wxString fullSavePath = AskProjectSavePath(this, m_filename);
    if (fullSavePath.empty())
        return;

    // On passe le chemin complet du fichier et non le dossier
    if (layerList->SaveToFile(fullSavePath))
    {
        SetDocumentFilename(fullSavePath);

        const wxString savecompleted =
            CLibResource::LoadStringFromResource("LBLSAVEFILECOMPLETED", 1);

        const wxString infos =
            CLibResource::LoadStringFromResource("LBLINFORMATIONS", 1);

        wxMessageBox(savecompleted, infos, wxOK | wxICON_INFORMATION, this);
    }
    else
    {
        // Gestion d'erreur si la sérialisation OpenCV échoue
        wxMessageBox("Erreur lors de la sauvegarde du fichier.", "Erreur", wxOK | wxICON_ERROR, this);
    }
}

void ImageDocument::Export()
{
    std::unique_ptr<CImageLoadingFormat> imageLoading;
    imageLoading.reset(bitmapWindow->GetBitmap(true));
    wxString filename = CSavePicture::SavePicture(nullptr, imageLoading.get(), bitmapWindow->GetFilename());

    if (!filename.empty())
        SetDocumentFilename(filename);
}


// ---------------------------------------------------------
// Transformations
// ---------------------------------------------------------

void ImageDocument::OnFlipVertical()
{
    WithEditor([](auto& editor) { editor.FlipVertical(); });
}

void ImageDocument::OnFlipHorizontal()
{
    WithEditor([](auto& editor) { editor.FlipHorizontal(); });
}

void ImageDocument::OnRotate90()
{
    WithEditorAndStatus([](auto& editor) { editor.Rotate90(); });
}

void ImageDocument::OnRotate180()
{
    WithEditorAndStatus([](auto& editor) { editor.Rotate180(); });
}

void ImageDocument::OnRotate270()
{
    WithEditorAndStatus([](auto& editor) { editor.Rotate270(); });
}

void ImageDocument::Crop()
{
    WithEditorAndStatus([](auto& editor) { editor.Crop(); });
}

void ImageDocument::Resize(int newWidth, int newHeight, int interpolation)
{
    WithEditorAndStatus([&](auto& editor) { editor.Resize(newWidth, newHeight, interpolation); });
}

void ImageDocument::CanvasResize(CanvasSizeParameter canvasSize)
{
    WithEditorAndStatus([&](auto& editor) { editor.CanvasResize(canvasSize); });
}


// ---------------------------------------------------------
// Zoom / barre de statut
// ---------------------------------------------------------

void ImageDocument::ZoomIn()
{
    WithEditorAndStatus([](auto& editor) { editor.ZoomOn(); });
}

void ImageDocument::ZoomOut()
{
    WithEditorAndStatus([](auto& editor) { editor.ZoomOut(); });
}

void ImageDocument::Shrink()
{
    WithEditorAndStatus([](auto& editor)
        {
            const bool shrink = editor.GetShrinkImage();
            editor.SetShrinkImage(!shrink);
            if (!shrink)
                editor.ShrinkImage(true);
        });
}

void ImageDocument::RealSize()
{
    WithEditorAndStatus([](auto& editor) { editor.SetRealSize(); });
}

int ImageDocument::GetBitmapWidth() const
{
    return bitmapWindow ? bitmapWindow->GetBitmapWidth() : 0;
}

int ImageDocument::GetBitmapHeight() const
{
    return bitmapWindow ? bitmapWindow->GetBitmapHeight() : 0;
}

void ImageDocument::UpdateStatusText()
{
    if (!m_statusBar || !bitmapWindow) return;

    const int imgW = bitmapWindow->GetBitmapWidth();
    const int imgH = bitmapWindow->GetBitmapHeight();
    const int zoomPos = bitmapWindow->GetPosRatio(); // Position du zoom réel

    m_statusBar->SetStatusText(wxString::Format("%dx%d", imgW, imgH), 0);
    m_statusBar->SetStatusText(wxString::Format("Zoom : %d", zoomValues[zoomPos]), 1);

    // Met à jour la position visuelle du slider pour suivre les changements internes
    if (m_zoomSlider)
    {
        m_zoomSlider->SetValue(zoomPos);
    }
}

void ImageDocument::OnUpdateStatusText(wxCommandEvent& event)
{
    UpdateStatusText();
}

void ImageDocument::OnSliderScroll(wxCommandEvent& event)
{
    if (!bitmapWindow) return;

    // Index sélectionné sur le slider (0 à zoomValues.size() - 1)
    const int zoomIndex = event.GetInt();

    // Forcer le rafraîchissement de l'affichage
    needToRefresh = true;

    bitmapWindow->SetZoomPosition(zoomIndex);

    UpdateStatusText();
}


// ---------------------------------------------------------
// Événements fenêtre
// ---------------------------------------------------------

// Implémentation de la fonction OnActivate
void ImageDocument::OnActivate(wxActivateEvent& event)
{
    // Si la fenêtre devient active, on transmet notre pointeur au CEditorFrame parent
    if (event.GetActive())
    {
        if (CEditorFrame* frame = GetEditorFrame(this))
        {
            frame->SetActiveDocument(this);
            frame->SetStatusText("Document actif changé : " + this->GetTitle());
        }
    }
    event.Skip(); // IMPORTANT : laisser wxWidgets propager l'événement normalement
}

void ImageDocument::OnResize(wxCommandEvent& event)
{
    // this->Resize();
}

void ImageDocument::OnRefresh(wxCommandEvent& event)
{
    needToRefresh = true;
}


void ImageDocument::OnEraseBackground(wxEraseEvent& event)
{}


void ImageDocument::OnIdle(wxIdleEvent& evt)
{
    if (needToRefresh)
    {
        this->Refresh();
        needToRefresh = false;
    }
}

void ImageDocument::ResizeImage(int w, int h)
{
    scrollbar->SetSize(0, 0, w, h);

    UpdateStatusText();

    scrollbar->Refresh();
}

void ImageDocument::OnSize(wxSizeEvent& event)
{
    const wxSize clientSize = this->GetClientSize();
    int _width = clientSize.GetWidth();
    int _height = clientSize.GetHeight();

    if (_width <= 20 && _height <= 20)
    {
        return;
    }
    if (m_statusBar)
    {
        int statusHeight = m_statusBar->GetSize().GetHeight();
        m_statusBar->SetSize(0, _height - statusHeight, _width, statusHeight);

        // --- REPOSITIONNEMENT DU SLIDER DANS LE CHAMP 2 ---
        if (m_zoomSlider)
        {
            wxRect rect;
            // Récupère les coordonnées exactes du champ d'index 2
            if (m_statusBar->GetFieldRect(2, rect))
            {
                // On ajuste légèrement pour centrer verticalement et laisser des marges
                m_zoomSlider->SetSize(rect.x + 5, rect.y + 2, rect.width - 10, rect.height - 4);
            }
        }

        _height -= statusHeight;
    }


    ResizeImage(_width, _height);


}