#pragma once
#include <FiltreEffectWnd.h>
#include <InfoEffectWnd.h>
#include <FiltreToolbar.h>
using namespace Regards::Control;

#define TYPE_DRAWING 1
#define TYPE_EFFECT 2


class ParameterDialog : public wxDialog {
public:
    ParameterDialog(wxWindow* parent);
    void SetFiltre(const int& numFiltre, const int64_t& numLayerId, CInfoEffectWnd* historyEffectWnd, const wxString& filename, const int& bitmapViewerId, const int& mainViewerId);
    void SetTypeFiltre(const int& typeFiltre);
    void SetColor(const wxColour& color1, const wxColour& color2);
    void SetActifLayer(const int& numLayer);
    void SetActifLayerId(const int64_t& numLayerId);
private:
    void OnSize(wxSizeEvent& event);
    void OnFiltreOk(wxCommandEvent& event);
    void OnFiltreCancel(wxCommandEvent& event);

    CInfoEffectWnd* historyEffectWnd = nullptr;
    CFiltreEffectScrollWnd* filtreEffectWnd = nullptr;
    CFiltreToolbar* filtreToolbar = nullptr;

    int numLayer = 0;
    int64_t numLayerId = 0;
    int numFiltre = 0;
    wxStatusBar* m_statusBar;
    int typeFiltre = 0;
    wxColour color1;
    wxColour color2;

};
