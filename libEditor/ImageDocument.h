#pragma once
#include "ScrollbarWnd.h"
#include <BitmapWnd3d.h>
#include "BitmapEditor.h"
#include <ThemeParam.h>
#include <customslider.h>
using namespace Regards::Window;
using namespace Regards::Control;
// --- 1. La Fenêtre "Image" (Dialogue Non-Modal) ---
class ImageDocument : public wxDialog {
public:
    ImageDocument(wxWindow* parent, wxWindowID bitmapViewerId,
        wxWindowID mainViewerId, CBitmapInterface* bitmapInterfaceIn, CThemeParam* config, const wxString& filename);

	wxString GetFileName() const { return m_filename; }
	void ZoomIn();
	void ZoomOut();
	void Shrink();
	void RealSize();

    void OnCrop();
    void OnResize();
    void OnCanvas();
    void OnFlipVertical();
    void OnFlipHorizontal();
    void OnRotate90();
    void OnRotate180();
    void OnRotate270();
private:

    void ResizeImage(int w, int h);
    void OnRefresh(wxCommandEvent& event);
    void OnResize(wxCommandEvent& event);
    void OnEraseBackground(wxEraseEvent& event);
    void OnIdle(wxIdleEvent& evt);
    void OnSize(wxSizeEvent& event);
    void OnActivate(wxActivateEvent& event);
    void OnSliderScroll(wxCommandEvent& event);
    void UpdateStatusText();

    bool needToRefresh = false;
    CScrollbarWnd * scrollbar = nullptr;
    CBitmapEditor* bitmapWindow = nullptr;
    CBitmapWnd3D *     bitmapWindowRender = nullptr;
    CBitmapInterface * bitmapInterface = nullptr;
   
    wxString m_filename;


    wxCustomSlider * m_zoomSlider = nullptr; // Le slider pour le zoom
    wxStatusBar* m_statusBar;
};