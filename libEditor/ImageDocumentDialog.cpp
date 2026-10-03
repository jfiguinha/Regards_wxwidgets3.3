#include <header.h>
#include "ImageDocumentDialog.h"



ImageDocumentDialog::ImageDocumentDialog(wxWindow* parent, const wxString& filename)
    : wxDialog(parent, wxID_ANY, filename, wxDefaultPosition, wxSize(600, 450),
        wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER | wxMAXIMIZE_BOX | wxMINIMIZE_BOX)
{
    // Zone de dessin défilante pour l'image
    m_canvas = new wxScrolledCanvas(this, wxID_ANY);
    m_canvas->SetScrollbars(10, 10, 100, 100);
    m_canvas->SetBackgroundColour(*wxLIGHT_GREY);

    // Liaison de l'événement de dessin (Paint)
    m_canvas->Bind(wxEVT_PAINT, &ImageDocumentDialog::OnPaint, this);

    // Layout de base
    wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);
    sizer->Add(m_canvas, 1, wxEXPAND);
    SetSizer(sizer);
}


void ImageDocumentDialog::OnPaint(wxPaintEvent& event) {
    wxPaintDC dc(m_canvas);
    m_canvas->PrepareDC(dc);

    // Exemple : Dessin d'un placeholder à la place de l'image
    dc.SetBrush(*wxWHITE_BRUSH);
    dc.DrawRectangle(50, 50, 400, 300);
    dc.DrawText("Zone d'édition de l'image", 70, 70);
}
