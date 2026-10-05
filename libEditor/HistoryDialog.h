#pragma once
#include <InfoEffectWnd.h>

class HistoryDialog : public wxDialog {
public:
    HistoryDialog(wxWindow* parent);
	Regards::Control::CInfoEffectWnd* GetHistoryEffectWnd() const { return historyEffectWnd; }
private:
    void SetFilename(const wxString& filename);

	void OnSize(wxSizeEvent& event);

    wxStatusBar* m_statusBar;
    Regards::Control::CInfoEffectWnd* historyEffectWnd;
};
