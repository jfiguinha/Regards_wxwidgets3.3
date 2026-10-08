#pragma once
#include <InfoEffectWnd.h>



class HistoryDialog : public wxDialog {
public:
    HistoryDialog(wxWindow* parent);
    void SetHistoryControl(Regards::Control::CInfoEffect* infoEffect);
    void SetFilename(const wxString& filename);
    wxString GetFilename();
	Regards::Control::CInfoEffectWnd* GetHistoryEffectWnd() const { return historyEffectWnd; }

private:
	void OnSize(wxSizeEvent& event);

    wxStatusBar* m_statusBar;
    Regards::Control::CInfoEffectWnd* historyEffectWnd;
    wxString filename;
};
