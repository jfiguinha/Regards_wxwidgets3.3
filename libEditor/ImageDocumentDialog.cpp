#include <header.h>
#include "ImageDocumentDialog.h"
#include <libPicture.h>

using namespace Regards::Picture;

ImageDocumentDialog::ImageDocumentDialog(wxWindow* parent, wxWindowID bitmapViewerId,
    wxWindowID mainViewerId, CBitmapInterface* bitmapInterfaceIn, CThemeParam* config, const wxString& filename)
    : wxDialog(parent, wxID_ANY, filename, wxDefaultPosition, wxSize(600, 450),
        wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER | wxMAXIMIZE_BOX | wxMINIMIZE_BOX)
{
    CLibPicture libPicture;
    CImageLoadingFormat* pictureLocal = libPicture.LoadPicture(filename);

    // --- Valeurs de zoom partagées ---
    const std::vector<int> zoomValues = {
        1, 2, 3, 4, 5, 6, 8, 12, 16, 25, 33, 50, 66, 75, 100,
        133, 150, 166, 200, 300, 400, 500, 600, 700, 800, 1200, 1600
    };

    CThemeBitmapWindow themeBitmap;
    if (config)
    {
        config->GetBitmapWindowTheme(&themeBitmap);
    }

    this->bitmapInterface = bitmapInterfaceIn;

    bitmapWindow = new CBitmapEditor(nullptr, mainViewerId, themeBitmap, bitmapInterface);
    bitmapWindow->SetTabValue(zoomValues);

    bitmapWindowRender = new CBitmapWnd3D(this, bitmapViewerId);
    bitmapWindowRender->SetBitmapRenderInterface(bitmapWindow);
	
    scrollbar = new CScrollbarWnd(this, bitmapWindowRender, wxID_ANY, "BitmapScroll");

    scrollbar->Show(true);
    bitmapWindowRender->Show(true);

	bitmapWindow->SetBitmap(pictureLocal);

    Connect(wxEVT_SIZE, wxSizeEventHandler(ImageDocumentDialog::OnSize));
    Connect(wxEVENT_REFRESH, wxCommandEventHandler(ImageDocumentDialog::OnRefresh));
    Connect(wxEVENT_RESIZE, wxCommandEventHandler(ImageDocumentDialog::OnResize));
    Connect(wxEVT_ERASE_BACKGROUND, wxEraseEventHandler(ImageDocumentDialog::OnEraseBackground));
    Connect(wxEVT_IDLE, wxIdleEventHandler(ImageDocumentDialog::OnIdle));

    // --- CORRECTION : Création manuelle de la Status Bar ---
  // En passant 'this' en parent, le wxStatusBar se place automatiquement tout en bas du dialogue
    m_statusBar = new wxStatusBar(this, wxID_ANY);
    m_statusBar->SetFieldsCount(1); // 1 seule section textuelle
    m_statusBar->SetStatusText("Prêt.");
}


void ImageDocumentDialog::OnResize(wxCommandEvent& event)
{
   // this->Resize();
}

void ImageDocumentDialog::OnRefresh(wxCommandEvent& event)
{
    needToRefresh = true;
}


void ImageDocumentDialog::OnEraseBackground(wxEraseEvent& event)
{}


void ImageDocumentDialog::OnIdle(wxIdleEvent& evt)
{
    if (needToRefresh)
    {
        this->Refresh();
        needToRefresh = false;
    }
}

void ImageDocumentDialog::ResizeImage(int w, int h)
{
    scrollbar->SetSize(0, 0, w, h);
    scrollbar->Refresh();
}

void ImageDocumentDialog::OnSize(wxSizeEvent& event)
{
    const wxSize clientSize = this->GetClientSize();
    int _width = clientSize.GetWidth();
    int _height = clientSize.GetHeight();

    if (_width <= 20 && _height <= 20)
    {
        return;
    }

    // --- CORRECTION : Repositionner la barre et soustraire sa hauteur ---
    if (m_statusBar)
    {
        // Récupérer la hauteur idéale/par défaut de la barre de statut
        int statusHeight = m_statusBar->GetSize().GetHeight();

        // Positionner manuellement la barre tout en bas du dialogue sur toute la largeur
        m_statusBar->SetSize(0, _height - statusHeight, _width, statusHeight);

        // Réduire la hauteur disponible pour l'image afin de ne pas masquer la scrollbar
        _height -= statusHeight;
    }

    ResizeImage(_width, _height);
    

}

