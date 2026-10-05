#pragma once
#include "ScrollbarWnd.h"
#include <BitmapWnd3d.h>
#include "BitmapEditor.h"
#include <ThemeParam.h>
#include <customslider.h>
#include <InfoEffect.h>
#include <InfoEffectWnd.h>
using namespace Regards::Window;
using namespace Regards::Control;

class CModificationManager;

// --- 1. La Fenêtre "Image" (Dialogue Non-Modal) ---
class ImageDocument : public wxDialog {
public:
    ImageDocument(wxWindow* parent, wxWindowID bitmapViewerId,
        wxWindowID mainViewerId, CBitmapInterface* bitmapInterfaceIn, CThemeParam* config, const wxString& filename);

    ImageDocument(wxWindow* parent, wxWindowID bitmapViewerId,
        wxWindowID mainViewerId, CBitmapInterface* bitmapInterfaceIn, CThemeParam* config, CImageLoadingFormat* pictureLocal);

    CInfoEffect * GetHistoryPt();
	wxString GetFileName() const { return m_filename; }

	int GetBitmapViewerId() const { return id; }
	int GetMainViewerId() const { return mainviewerid; }

	int GetBitmapWidth() const;
	int GetBitmapHeight() const;

    void Save();

	void ZoomIn();
	void ZoomOut();
	void Shrink();
	void RealSize();

	void Resize(int newWidth, int newHeight, int interpolation);

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
    std::unique_ptr<CModificationManager> modificationManager;
    std::unique_ptr<CInfoEffect> historyEffect;

    wxString m_filename;
    int id = 0;
	int mainviewerid = 0;
    wxCustomSlider * m_zoomSlider = nullptr; // Le slider pour le zoom
    wxStatusBar* m_statusBar;
};