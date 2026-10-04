#pragma once
#include <ThumbnailViewerEffectWnd.h>

class EffectsDialog : public wxDialog {
public:
    EffectsDialog(wxWindow* parent);
    void SetFilename(const wxString& filename);

private:

    void OnSize(wxSizeEvent& event);
    Regards::Control::CThumbnailViewerEffectWnd* thumbnailEffectWnd;
    wxString m_filename = "";
    wxStatusBar* m_statusBar;
};
