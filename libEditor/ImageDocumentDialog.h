#pragma once

// --- 1. La Fenêtre "Image" (Dialogue Non-Modal) ---
class ImageDocumentDialog : public wxDialog {
public:
    ImageDocumentDialog(wxWindow* parent, const wxString& filename);

private:
    void OnPaint(wxPaintEvent& event);
    wxScrolledCanvas* m_canvas;
};