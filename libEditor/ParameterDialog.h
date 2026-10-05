#pragma once
#include <FiltreEffectWnd.h>
#include <InfoEffectWnd.h>
#include <FiltreToolbar.h>
using namespace Regards::Control;


class ParameterDialog : public wxDialog {
public:
    ParameterDialog(wxWindow* parent);
    void SetFiltre(const int& numFiltre, CInfoEffectWnd* historyEffectWnd, const wxString& filename, const int& bitmapViewerId, const int& mainViewerId);

private:
    void OnSize(wxSizeEvent& event);
    void OnFiltreOk(wxCommandEvent& event);
    void OnFiltreCancel(wxCommandEvent& event);

    CInfoEffectWnd* historyEffectWnd = nullptr;
    CFiltreEffectScrollWnd* filtreEffectWnd = nullptr;
    CFiltreToolbar* filtreToolbar = nullptr;
    
    int numFiltre = 0;
    wxStatusBar* m_statusBar;
};
