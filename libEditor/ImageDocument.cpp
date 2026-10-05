#include <header.h>
#include "ImageDocument.h"
#include <libPicture.h>
#include <ModificationManager.h>
#include <FileUtility.h>
#include <EditorFrame.h>
#include <LibResource.h>
#include <ImageLoadingFormat.h>
using namespace Regards::Picture;


// --- Valeurs de zoom partagées ---
const std::vector<int> zoomValues = {
    1, 2, 3, 4, 5, 6, 8, 12, 16, 25, 33, 50, 66, 75, 100,
    133, 150, 166, 200, 300, 400, 500, 600, 700, 800, 1200, 1600
};


ImageDocument::ImageDocument(wxWindow* parent, wxWindowID bitmapViewerId,
    wxWindowID mainViewerId, CBitmapInterface* bitmapInterfaceIn, CThemeParam* config, const wxString& filename)
    : wxDialog(parent, wxID_ANY, filename, wxDefaultPosition, wxSize(600, 450),
        wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER | wxMAXIMIZE_BOX | wxMINIMIZE_BOX)
{
    CLibPicture libPicture;
    CImageLoadingFormat* pictureLocal = libPicture.LoadPicture(filename);

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
    Connect(wxEVENT_REFRESH, wxCommandEventHandler(ImageDocument::OnRefresh));
    Connect(wxEVENT_RESIZE, wxCommandEventHandler(ImageDocument::OnResize));
    Connect(wxEVT_ERASE_BACKGROUND, wxEraseEventHandler(ImageDocument::OnEraseBackground));
    Connect(wxEVT_IDLE, wxIdleEventHandler(ImageDocument::OnIdle));
    Connect(wxEVT_ACTIVATE, wxActivateEventHandler(ImageDocument::OnActivate));
    // --- CORRECTION : Création manuelle de la Status Bar ---
    m_statusBar = new wxStatusBar(this, wxID_ANY);
     
    // En passant 'this' en parent, le wxStatusBar se place automatiquement tout en bas du dialogue
    // Définition de 3 champs : 
    // Champ 0 (texte libre/chemin/etc.) -> Taille variable (-1)
    // Champ 1 (Dimensions de l'image)  -> Taille fixe (120px)
    // Champ 2 (Emplacement du Slider)  -> Taille fixe (200px)
    int widths[] = { -1, 120, 200 };
    m_statusBar->SetFieldsCount(3);
    m_statusBar->SetStatusWidths(3, widths);

    // Création du Slider sur le Champ index 2 de la StatusBar
    // On prend la plage d'index du vecteur zoomValues (de 0 à zoomValues.size() - 1)
    int maxZoomIndex = static_cast<int>(zoomValues.size()) - 1;
    m_zoomSlider = new wxCustomSlider(m_statusBar, wxID_ANY, zoomValues.size(), 0, zoomValues.size());

    // Initialiser le slider à la position actuelle (ex: 100%)
    // Si 100% est la 15ème valeur (index 14) :
    m_zoomSlider->SetValue(14);

    // Connexion de l'événement de défilement du Slider
    m_zoomSlider->Bind(wxEVT_CUSTOM_SLIDER_CHANGED, &ImageDocument::OnSliderScroll, this);

    wxString historyLibelle = CLibResource::LoadStringFromResource(L"LBLHISTORY", 1);
    wxString folder = CFileUtility::GetDocumentFolderPath();
    modificationManager = std::make_unique<CModificationManager>(folder);
    historyEffect = std::make_unique<CInfoEffect>(nullptr, modificationManager.get(), bitmapViewerId, filename);

    historyEffect->Init(pictureLocal, filename, historyLibelle);
    // Mettre à jour les textes pour la première fois
    UpdateStatusText();
}



ImageDocument::ImageDocument(wxWindow* parent, wxWindowID bitmapViewerId,
    wxWindowID mainViewerId, CBitmapInterface* bitmapInterfaceIn, CThemeParam* config, CImageLoadingFormat* pictureLocal)
    : wxDialog(parent, wxID_ANY, pictureLocal->GetFilename(), wxDefaultPosition, wxSize(600, 450),
        wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER | wxMAXIMIZE_BOX | wxMINIMIZE_BOX)
{
    CLibPicture libPicture;


    id = bitmapViewerId;
    this->mainviewerid = bitmapViewerId;

    CThemeBitmapWindow themeBitmap;
    if (config)
    {
        config->GetBitmapWindowTheme(&themeBitmap);
    }

    m_filename = pictureLocal->GetFilename();

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
    Connect(wxEVENT_REFRESH, wxCommandEventHandler(ImageDocument::OnRefresh));
    Connect(wxEVENT_RESIZE, wxCommandEventHandler(ImageDocument::OnResize));
    Connect(wxEVT_ERASE_BACKGROUND, wxEraseEventHandler(ImageDocument::OnEraseBackground));
    Connect(wxEVT_IDLE, wxIdleEventHandler(ImageDocument::OnIdle));
    Connect(wxEVT_ACTIVATE, wxActivateEventHandler(ImageDocument::OnActivate));
    // --- CORRECTION : Création manuelle de la Status Bar ---
    m_statusBar = new wxStatusBar(this, wxID_ANY);

    // En passant 'this' en parent, le wxStatusBar se place automatiquement tout en bas du dialogue
    // Définition de 3 champs : 
    // Champ 0 (texte libre/chemin/etc.) -> Taille variable (-1)
    // Champ 1 (Dimensions de l'image)  -> Taille fixe (120px)
    // Champ 2 (Emplacement du Slider)  -> Taille fixe (200px)
    int widths[] = { -1, 120, 200 };
    m_statusBar->SetFieldsCount(3);
    m_statusBar->SetStatusWidths(3, widths);

    // Création du Slider sur le Champ index 2 de la StatusBar
    // On prend la plage d'index du vecteur zoomValues (de 0 à zoomValues.size() - 1)
    int maxZoomIndex = static_cast<int>(zoomValues.size()) - 1;
    m_zoomSlider = new wxCustomSlider(m_statusBar, wxID_ANY, zoomValues.size(), 0, zoomValues.size());

    // Initialiser le slider à la position actuelle (ex: 100%)
    // Si 100% est la 15ème valeur (index 14) :
    m_zoomSlider->SetValue(14);

    // Connexion de l'événement de défilement du Slider
    m_zoomSlider->Bind(wxEVT_CUSTOM_SLIDER_CHANGED, &ImageDocument::OnSliderScroll, this);

    wxString historyLibelle = CLibResource::LoadStringFromResource(L"LBLHISTORY", 1);
    wxString folder = CFileUtility::GetDocumentFolderPath();
    modificationManager = std::make_unique<CModificationManager>(folder);
    historyEffect = std::make_unique<CInfoEffect>(nullptr, modificationManager.get(), bitmapViewerId, m_filename);

    historyEffect->Init(pictureLocal, m_filename, historyLibelle);
    // Mettre à jour les textes pour la première fois
    UpdateStatusText();
}


CInfoEffect* ImageDocument::GetHistoryPt()
{
    return historyEffect.get();
}

void ImageDocument::OnCrop()
{

}

void ImageDocument::OnResize()
{

}

void ImageDocument::OnCanvas()
{

}


void ImageDocument::OnFlipVertical()
{
    if (bitmapWindow)
        bitmapWindow->FlipVertical();
}

void ImageDocument::OnFlipHorizontal()
{
    if (bitmapWindow)
        bitmapWindow->FlipHorizontal();
}

void ImageDocument::OnRotate90()
{
    if (bitmapWindow)
        bitmapWindow->Rotate90();
}

void ImageDocument::OnRotate180()
{
    if (bitmapWindow)
        bitmapWindow->Rotate180();
}

void ImageDocument::OnRotate270()
{
    if (bitmapWindow)
        bitmapWindow->Rotate270();
}

void ImageDocument::UpdateStatusText()
{
    if (!m_statusBar || !bitmapWindow) return;

    // 1. Récupération de la taille de l'image depuis votre interface
    int imgW = 0, imgH = 0, zoomPos = 0;
    if (bitmapWindow)
    {
        imgW = bitmapWindow->GetBitmapWidth();
        imgH = bitmapWindow->GetBitmapHeight();
		zoomPos = bitmapWindow->GetPosRatio(); // Récupère la position du zoom
    }

    wxString sizeText = wxString::Format("%dx%d", imgW, imgH);
    m_statusBar->SetStatusText(sizeText, 0); // Écrit dans le champ index 1

    sizeText = wxString::Format("Zoom : %d", zoomValues[zoomPos]);
    m_statusBar->SetStatusText(sizeText, 1); // Écrit dans le champ index 2

    // 2. Récupération du facteur de zoom pour l'afficher au début du statut
    // (Ajustez selon les méthodes réelles de CBitmapEditor pour obtenir le zoom actuel)
    // Exemple : m_statusBar->SetStatusText("Zoom actuel: 100%", 0);
   // m_statusBar->SetStatusText("Prêt.", 0);
}

void ImageDocument::OnSliderScroll(wxCommandEvent& event)
{
    if (!bitmapWindow) return;

    // Récupérer l'index sélectionné sur le slider (0 à 26)
    int zoomIndex = event.GetInt();

    // Appliquer le niveau de zoom correspondant à l'index via votre CBitmapEditor
    // Note : Votre bitmapWindow possède déjà 'SetTabValue(zoomValues)'
    // Vérifiez si votre classe possède une méthode semblable à SetZoomIndex(int index)
    // bitmapWindow->SetZoomIndex(zoomIndex); 

    // Forcer le rafraîchissement de l'affichage
    needToRefresh = true;

    bitmapWindow->SetZoomPosition(zoomIndex);

    // Mettre à jour le texte du statut si nécessaire
    UpdateStatusText();
}


int ImageDocument::GetBitmapWidth() const
{
	if (bitmapWindow)
	{
		return bitmapWindow->GetBitmapWidth();
	}
	return 0;
}

int ImageDocument::GetBitmapHeight() const
{
	if (bitmapWindow)
	{
		return bitmapWindow->GetBitmapHeight();
	}
	return 0;
}

void ImageDocument::Resize(int newWidth, int newHeight, int interpolation)
{
    if (bitmapWindow)
    {
       // return bitmapWindow->GetBitmapHeight();
    }
}

void ImageDocument::ZoomIn()
{
    bitmapWindow->ZoomOn();
}

void ImageDocument::ZoomOut()
{
    bitmapWindow->ZoomOut();
}
void ImageDocument::Shrink()
{
    bool shrink = bitmapWindow->GetShrinkImage();
    if (!shrink)
    {
        bitmapWindow->SetShrinkImage(!shrink);
        bitmapWindow->ShrinkImage(true);
    }
    else
	    bitmapWindow->SetShrinkImage(!shrink);
}

void ImageDocument::RealSize()
{
	bitmapWindow->SetRealSize();
}

// Implémentation de la fonction OnActivate
void ImageDocument::OnActivate(wxActivateEvent& event)
{
    // Si la fenêtre devient active
    if (event.GetActive())
    {
        // On récupère le parent (CEditorFrame) et on lui transmet notre pointeur
        CEditorFrame* frame = dynamic_cast<CEditorFrame*>(GetParent());
        if (frame)
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

