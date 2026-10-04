#pragma once
#include <InfoEffectWnd.h>

class HistoryDialog : public wxDialog {
public:
    HistoryDialog(wxWindow* parent);

private:
    void SetFilename(const wxString& filename);

	void OnSize(wxSizeEvent& event);

    wxStatusBar* m_statusBar;
    Regards::Control::CInfoEffectWnd* historyEffectWnd;
};
