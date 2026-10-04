#pragma once
#include "ScrollbarWnd.h"
#include <BitmapWnd3d.h>
#include "BitmapEditor.h"
#include <ThemeParam.h>
using namespace Regards::Window;
using namespace Regards::Control;
// --- 1. La Fenêtre "Image" (Dialogue Non-Modal) ---
class ImageDocument : public wxDialog {
public:
    ImageDocument(wxWindow* parent, wxWindowID bitmapViewerId,
        wxWindowID mainViewerId, CBitmapInterface* bitmapInterfaceIn, CThemeParam* config, const wxString& filename);

	wxString GetFileName() const { return m_filename; }

private:

    void ResizeImage(int w, int h);
    void OnRefresh(wxCommandEvent& event);
    void OnResize(wxCommandEvent& event);
    void OnEraseBackground(wxEraseEvent& event);
    void OnIdle(wxIdleEvent& evt);
    void OnSize(wxSizeEvent& event);
    void OnActivate(wxActivateEvent& event);
    bool needToRefresh = false;
    CScrollbarWnd * scrollbar = nullptr;
    CBitmapEditor* bitmapWindow = nullptr;
    CBitmapWnd3D *     bitmapWindowRender = nullptr;
    CBitmapInterface * bitmapInterface = nullptr;
    wxStatusBar* m_statusBar;
    wxString m_filename;
};