#pragma once
#include <InfoEffectWnd.h>



class HistoryDialog : public wxDialog {
public:
    HistoryDialog(wxWindow* parent);
    void SetHistoryControl(Regards::Control::CInfoEffect* infoEffect);
    void SetFilename(const wxString& filename);
    wxString GetFilename();
	Regards::Control::CInfoEffectWnd* GetHistoryEffectWnd() const { return historyEffectWnd; }

    void Redo();
    void Undo();
    void CancelHistory();


private:
	void OnSize(wxSizeEvent& event);
    void OnRefreshHistory(wxCommandEvent& event);
    void OnUndo(wxCommandEvent& event);
    void OnRedo(wxCommandEvent& event);
    void OnCancelHistory(wxCommandEvent& event);
    wxStatusBar* m_statusBar;
    Regards::Control::CInfoEffectWnd* historyEffectWnd;
    wxString filename;

};
