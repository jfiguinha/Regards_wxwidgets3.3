#include <header.h>
#include <wx/button.h>
#include <wx/colour.h>
#include <wx/gbsizer.h>
#include <wx/sizer.h>
#include <wx/slider.h>
#include <wx/spinctrl.h>
#include <wx/statbox.h>
#include <wx/statline.h>
#include <wx/stattext.h>

#include <algorithm>
#include <cmath>

#include "ColorPickerDialog.h"
#include "ColorSLPickerCtrl.h"
#include "ColorWheelCtrl.h"

namespace {
    void RGBToHSL(int r, int g, int b, double& h, double& s, double& l) {
        const double rd = static_cast<double>(r) / 255.0;

        const double gd = static_cast<double>(g) / 255.0;

        const double bd = static_cast<double>(b) / 255.0;

        const double maxValue = std::max({ rd, gd, bd });

        const double minValue = std::min({ rd, gd, bd });

        l = (maxValue + minValue) * 0.5;

        if (std::abs(maxValue - minValue) < 0.000001) {
            h = 0.0;
            s = 0.0;
            return;
        }

        const double delta = maxValue - minValue;

        s = l > 0.5 ? delta / (2.0 - maxValue - minValue)
            : delta / (maxValue + minValue);

        if (maxValue == rd) {
            h = (gd - bd) / delta;

            if (gd < bd) h += 6.0;
        }
        else if (maxValue == gd) {
            h = (bd - rd) / delta + 2.0;
        }
        else {
            h = (rd - gd) / delta + 4.0;
        }

        h *= 60.0;
    }

    wxColour HSLToColour(double h, double s, double l, unsigned char alpha) {
        h = std::fmod(h, 360.0);

        if (h < 0.0) h += 360.0;

        s = std::clamp(s, 0.0, 1.0);
        l = std::clamp(l, 0.0, 1.0);

        if (s <= 0.0) {
            const unsigned char value =
                static_cast<unsigned char>(std::round(l * 255.0));

            return wxColour(value, value, value, alpha);
        }

        const double q = l < 0.5 ? l * (1.0 + s) : l + s - l * s;

        const double p = 2.0 * l - q;

        auto hueToRGB = [](double p, double q, double t) {
            if (t < 0.0) t += 1.0;

            if (t > 1.0) t -= 1.0;

            if (t < 1.0 / 6.0) return p + (q - p) * 6.0 * t;

            if (t < 1.0 / 2.0) return q;

            if (t < 2.0 / 3.0) return p + (q - p) * (2.0 / 3.0 - t) * 6.0;

            return p;
            };

        const double hk = h / 360.0;

        const double rd = hueToRGB(p, q, hk + 1.0 / 3.0);

        const double gd = hueToRGB(p, q, hk);

        const double bd = hueToRGB(p, q, hk - 1.0 / 3.0);

        return wxColour(static_cast<unsigned char>(std::round(rd * 255.0)),
            static_cast<unsigned char>(std::round(gd * 255.0)),
            static_cast<unsigned char>(std::round(bd * 255.0)), alpha);
    }
}  // namespace

wxBEGIN_EVENT_TABLE(CColorPickerDialog, wxDialog)

EVT_COMMAND(wxID_ANY, wxEVT_COMMAND_SLIDER_UPDATED,
    CColorPickerDialog::OnWheelChanged)

    EVT_SPINCTRL(wxID_ANY, CColorPickerDialog::OnRGBChanged)

    wxEND_EVENT_TABLE()

    CColorPickerDialog::CColorPickerDialog(wxWindow* parent,
        const wxColour& colour,
        wxWindowID id,
        const wxString& title,
        const wxPoint& pos,
        const wxSize& size)
    : wxDialog(parent, id, title, pos, size,
        wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER) {
    CreateControls();

    SetColour(colour);

    CentreOnParent();
}

void CColorPickerDialog::CreateControls() {
    auto* mainSizer = new wxBoxSizer(wxVERTICAL);

    auto* contentSizer = new wxBoxSizer(wxHORIZONTAL);

    // ---------------------------------------------------------
    // Partie gauche
    // ---------------------------------------------------------

    auto* pickerSizer = new wxBoxSizer(wxVERTICAL);

    m_colorWheel =
        new CColorWheelCtrl(this, wxID_ANY, wxDefaultPosition, wxSize(220, 220));

    pickerSizer->Add(m_colorWheel, 0, wxALL | wxALIGN_CENTER, 10);

    m_slPicker = new CColorSLPickerCtrl(this, wxID_ANY, wxDefaultPosition,
        wxSize(260, 170));

    pickerSizer->Add(m_slPicker, 1, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 10);

    contentSizer->Add(pickerSizer, 1, wxEXPAND);

    // ---------------------------------------------------------
    // Partie droite
    // ---------------------------------------------------------

    auto* valuesSizer = new wxBoxSizer(wxVERTICAL);

    // ---------------------------------------------------------
    // Aperçu
    // ---------------------------------------------------------

    auto* previewBox = new wxStaticBoxSizer(wxVERTICAL, this, "Aperçu");

    m_preview = new wxPanel(this, wxID_ANY, wxDefaultPosition, wxSize(180, 60));

    previewBox->Add(m_preview, 1, wxEXPAND | wxALL, 5);

    valuesSizer->Add(previewBox, 0, wxEXPAND | wxALL, 5);

    // ---------------------------------------------------------
    // RGB
    // ---------------------------------------------------------

    auto* rgbBox = new wxStaticBoxSizer(wxVERTICAL, this, "RGB");

    auto* rgbGrid = new wxFlexGridSizer(3, 2, 6, 8);

    rgbGrid->AddGrowableCol(1);

    rgbGrid->Add(new wxStaticText(this, wxID_ANY, "Rouge"), 0,
        wxALIGN_CENTER_VERTICAL);

    m_red = new wxSpinCtrl(this, wxID_ANY, wxEmptyString, wxDefaultPosition,
        wxDefaultSize, wxSP_ARROW_KEYS, 0, 255, 0);

    rgbGrid->Add(m_red, 1, wxEXPAND);

    rgbGrid->Add(new wxStaticText(this, wxID_ANY, "Vert"), 0,
        wxALIGN_CENTER_VERTICAL);

    m_green = new wxSpinCtrl(this, wxID_ANY, wxEmptyString, wxDefaultPosition,
        wxDefaultSize, wxSP_ARROW_KEYS, 0, 255, 0);

    rgbGrid->Add(m_green, 1, wxEXPAND);

    rgbGrid->Add(new wxStaticText(this, wxID_ANY, "Bleu"), 0,
        wxALIGN_CENTER_VERTICAL);

    m_blue = new wxSpinCtrl(this, wxID_ANY, wxEmptyString, wxDefaultPosition,
        wxDefaultSize, wxSP_ARROW_KEYS, 0, 255, 0);

    rgbGrid->Add(m_blue, 1, wxEXPAND);

    rgbBox->Add(rgbGrid, 1, wxEXPAND | wxALL, 8);

    valuesSizer->Add(rgbBox, 0, wxEXPAND | wxALL, 5);

    // ---------------------------------------------------------
    // TSL
    // ---------------------------------------------------------

    auto* hslBox = new wxStaticBoxSizer(wxVERTICAL, this, "TSL");

    auto* hslGrid = new wxFlexGridSizer(3, 2, 6, 8);

    hslGrid->AddGrowableCol(1);

    hslGrid->Add(new wxStaticText(this, wxID_ANY, "Teinte"), 0,
        wxALIGN_CENTER_VERTICAL);

    m_hueCtrl = new wxSpinCtrl(this, wxID_ANY, wxEmptyString, wxDefaultPosition,
        wxDefaultSize, wxSP_ARROW_KEYS, 0, 360, 0);

    hslGrid->Add(m_hueCtrl, 1, wxEXPAND);

    hslGrid->Add(new wxStaticText(this, wxID_ANY, "Saturation"), 0,
        wxALIGN_CENTER_VERTICAL);

    m_saturationCtrl =
        new wxSpinCtrl(this, wxID_ANY, wxEmptyString, wxDefaultPosition,
            wxDefaultSize, wxSP_ARROW_KEYS, 0, 100, 0);

    hslGrid->Add(m_saturationCtrl, 1, wxEXPAND);

    hslGrid->Add(new wxStaticText(this, wxID_ANY, "Luminosité"), 0,
        wxALIGN_CENTER_VERTICAL);

    m_lightnessCtrl =
        new wxSpinCtrl(this, wxID_ANY, wxEmptyString, wxDefaultPosition,
            wxDefaultSize, wxSP_ARROW_KEYS, 0, 100, 0);

    hslGrid->Add(m_lightnessCtrl, 1, wxEXPAND);

    hslBox->Add(hslGrid, 1, wxEXPAND | wxALL, 8);

    valuesSizer->Add(hslBox, 0, wxEXPAND | wxALL, 5);

    // ---------------------------------------------------------
    // Alpha
    // ---------------------------------------------------------

    auto* alphaBox = new wxStaticBoxSizer(wxVERTICAL, this, "Opacité");

    auto* alphaSizer = new wxBoxSizer(wxHORIZONTAL);

    m_alphaCtrl = new wxSlider(this, wxID_ANY, 255, 0, 255, wxDefaultPosition,
        wxDefaultSize, wxSL_HORIZONTAL);

    alphaSizer->Add(m_alphaCtrl, 1, wxEXPAND | wxRIGHT, 8);

    m_alphaValue = new wxStaticText(this, wxID_ANY, "255");

    alphaSizer->Add(m_alphaValue, 0, wxALIGN_CENTER_VERTICAL);

    alphaBox->Add(alphaSizer, 0, wxEXPAND | wxALL, 8);

    valuesSizer->Add(alphaBox, 0, wxEXPAND | wxALL, 5);

    contentSizer->Add(valuesSizer, 0, wxEXPAND | wxALL, 5);

    mainSizer->Add(contentSizer, 1, wxEXPAND | wxALL, 5);

    // ---------------------------------------------------------
    // Boutons
    // ---------------------------------------------------------

    mainSizer->Add(new wxStaticLine(this, wxID_ANY), 0,
        wxEXPAND | wxLEFT | wxRIGHT, 10);

    auto* buttonSizer = new wxStdDialogButtonSizer();

    auto* okButton = new wxButton(this, wxID_OK, "OK");

    auto* cancelButton = new wxButton(this, wxID_CANCEL, "Annuler");

    buttonSizer->AddButton(okButton);
    buttonSizer->AddButton(cancelButton);
    buttonSizer->Realize();

    mainSizer->Add(buttonSizer, 0, wxEXPAND | wxALL, 10);

    SetSizer(mainSizer);

    // ---------------------------------------------------------
    // Événements
    // ---------------------------------------------------------

    m_colorWheel->Bind(wxEVT_COMMAND_SLIDER_UPDATED,
        &CColorPickerDialog::OnWheelChanged, this);

    m_slPicker->Bind(wxEVT_COMMAND_SLIDER_UPDATED,
        &CColorPickerDialog::OnSLChanged, this);

    m_red->Bind(wxEVT_SPINCTRL, &CColorPickerDialog::OnRGBChanged, this);

    m_green->Bind(wxEVT_SPINCTRL, &CColorPickerDialog::OnRGBChanged, this);

    m_blue->Bind(wxEVT_SPINCTRL, &CColorPickerDialog::OnRGBChanged, this);

    m_hueCtrl->Bind(wxEVT_SPINCTRL, &CColorPickerDialog::OnHSLChanged, this);

    m_saturationCtrl->Bind(wxEVT_SPINCTRL, &CColorPickerDialog::OnHSLChanged,
        this);

    m_lightnessCtrl->Bind(wxEVT_SPINCTRL, &CColorPickerDialog::OnHSLChanged,
        this);

    m_alphaCtrl->Bind(wxEVT_SLIDER, &CColorPickerDialog::OnAlphaChanged, this);

    okButton->Bind(wxEVT_BUTTON, &CColorPickerDialog::OnOK, this);
}

void CColorPickerDialog::SetColour(const wxColour& colour) {
    if (!colour.IsOk()) return;

    m_colour = colour;

    UpdateControlsFromColour();
}

wxColour CColorPickerDialog::GetColour() const { return m_colour; }

void CColorPickerDialog::UpdateControlsFromColour() {
    if (!m_colour.IsOk()) return;

    m_updating = true;

    RGBToHSL(m_colour.Red(), m_colour.Green(), m_colour.Blue(), m_hue,
        m_saturation, m_lightness);

    m_colorWheel->SetHue(m_hue);

    m_slPicker->SetHSL(m_hue, m_saturation, m_lightness);

    m_red->SetValue(m_colour.Red());
    m_green->SetValue(m_colour.Green());
    m_blue->SetValue(m_colour.Blue());

    m_hueCtrl->SetValue(static_cast<int>(std::round(m_hue)));

    m_saturationCtrl->SetValue(
        static_cast<int>(std::round(m_saturation * 100.0)));

    m_lightnessCtrl->SetValue(static_cast<int>(std::round(m_lightness * 100.0)));

    m_alphaCtrl->SetValue(m_colour.Alpha());

    m_alphaValue->SetLabel(wxString::Format("%d", m_colour.Alpha()));

    UpdatePreview();

    m_updating = false;
}

void CColorPickerDialog::UpdateColourFromHSL() {
    if (m_updating) return;

    m_updating = true;

    m_hue = static_cast<double>(m_hueCtrl->GetValue());

    m_saturation = static_cast<double>(m_saturationCtrl->GetValue()) / 100.0;

    m_lightness = static_cast<double>(m_lightnessCtrl->GetValue()) / 100.0;

    m_colour = HSLToColour(m_hue, m_saturation, m_lightness,
        static_cast<unsigned char>(m_alphaCtrl->GetValue()));

    m_colorWheel->SetHue(m_hue);

    m_slPicker->SetHSL(m_hue, m_saturation, m_lightness);

    m_red->SetValue(m_colour.Red());
    m_green->SetValue(m_colour.Green());
    m_blue->SetValue(m_colour.Blue());

    UpdatePreview();

    m_updating = false;
}

void CColorPickerDialog::UpdateColourFromRGB() {
    if (m_updating) return;

    m_updating = true;

    const int r = m_red->GetValue();
    const int g = m_green->GetValue();
    const int b = m_blue->GetValue();

    RGBToHSL(r, g, b, m_hue, m_saturation, m_lightness);

    m_colour.Set(static_cast<unsigned char>(r), static_cast<unsigned char>(g),
        static_cast<unsigned char>(b),
        static_cast<unsigned char>(m_alphaCtrl->GetValue()));

    m_hueCtrl->SetValue(static_cast<int>(std::round(m_hue)));

    m_saturationCtrl->SetValue(
        static_cast<int>(std::round(m_saturation * 100.0)));

    m_lightnessCtrl->SetValue(static_cast<int>(std::round(m_lightness * 100.0)));

    m_colorWheel->SetHue(m_hue);

    m_slPicker->SetHSL(m_hue, m_saturation, m_lightness);

    UpdatePreview();

    m_updating = false;
}

void CColorPickerDialog::UpdatePreview() {
    if (!m_preview) return;

    m_preview->SetBackgroundColour(m_colour);

    m_preview->Refresh();
    m_preview->Update();
}

void CColorPickerDialog::OnWheelChanged(wxCommandEvent& event) {
    if (m_updating) return;

    if (event.GetEventObject() != m_colorWheel) return;

    m_updating = true;

    m_hue = m_colorWheel->GetHue();

    m_hueCtrl->SetValue(static_cast<int>(std::round(m_hue)));

    m_slPicker->SetHue(m_hue);

    m_colour = HSLToColour(m_hue, m_saturation, m_lightness,
        static_cast<unsigned char>(m_alphaCtrl->GetValue()));

    m_red->SetValue(m_colour.Red());
    m_green->SetValue(m_colour.Green());
    m_blue->SetValue(m_colour.Blue());

    UpdatePreview();

    m_updating = false;
}

void CColorPickerDialog::OnSLChanged(wxCommandEvent& event) {
    if (m_updating) return;

    if (event.GetEventObject() != m_slPicker) return;

    m_updating = true;

    m_saturation = m_slPicker->GetSaturation();

    m_lightness = m_slPicker->GetLightness();

    m_saturationCtrl->SetValue(
        static_cast<int>(std::round(m_saturation * 100.0)));

    m_lightnessCtrl->SetValue(static_cast<int>(std::round(m_lightness * 100.0)));

    m_colour = HSLToColour(m_hue, m_saturation, m_lightness,
        static_cast<unsigned char>(m_alphaCtrl->GetValue()));

    m_red->SetValue(m_colour.Red());
    m_green->SetValue(m_colour.Green());
    m_blue->SetValue(m_colour.Blue());

    UpdatePreview();

    m_updating = false;
}

void CColorPickerDialog::OnRGBChanged(wxSpinEvent&) { UpdateColourFromRGB(); }

void CColorPickerDialog::OnHSLChanged(wxSpinEvent&) { UpdateColourFromHSL(); }

void CColorPickerDialog::OnAlphaChanged(
    wxCommandEvent&)
{
    if (m_updating)
        return;

    const unsigned char alpha =
        static_cast<unsigned char>(
            m_alphaCtrl->GetValue());

    m_colour = wxColour(
        m_colour.Red(),
        m_colour.Green(),
        m_colour.Blue(),
        alpha);

    m_alphaValue->SetLabel(
        wxString::Format(
            "%d",
            static_cast<int>(alpha)));

    UpdatePreview();
}

void CColorPickerDialog::OnOK(wxCommandEvent&) { EndModal(wxID_OK); }