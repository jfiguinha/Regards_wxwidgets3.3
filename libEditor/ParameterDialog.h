#pragma once
#include <FiltreEffectWnd.h>
#include <InfoEffectWnd.h>
using namespace Regards::Control;


class ParameterDialog : public wxDialog {
public:
    ParameterDialog(wxWindow* parent);
    void SetFiltre(const int& numFiltre, CInfoEffectWnd* historyEffectWnd, const wxString& filename);

private:
    void OnSize(wxSizeEvent& event);

    CFiltreEffectScrollWnd* filtreEffectWnd;

    

    wxStatusBar* m_statusBar;
};
