#include <header.h>
#include "ResizeDialog.h"
#include <wx/xrc/xmlres.h>
#include <wx/slider.h>
#include <wx/combobox.h>
#include <wx/textctrl.h>
#include <wx/stattext.h>
#include <wx/spinctrl.h>

using namespace Regards::Dialog;

BEGIN_EVENT_TABLE(CResizeDialog, wxDialog)
//(*EventTable(ConfigRegards)
//*)
END_EVENT_TABLE()

CResizeDialog::CResizeDialog(
    wxWindow* parent,
    const ResizeParameter& parameter)
{
    wxXmlResource::Get()->LoadDialog(
        this,
        parent,
        "ResizeDialog");

    // Récupération des contrôles par leur nom XRC
    spinPixelWidth = XRCCTRL(*this, "m_spinPixelWidth", wxSpinCtrl);
    spinPixelHeight = XRCCTRL(*this, "m_spinPixelHeight", wxSpinCtrl);
    comboInterpolation = XRCCTRL(*this, "m_comboInterpolation", wxComboBox);

    // Initialisation des valeurs par défaut
    if (spinPixelWidth) spinPixelWidth->SetValue(parameter.width);
    if (spinPixelHeight) spinPixelHeight->SetValue(parameter.height);
    if (comboInterpolation) comboInterpolation->SetSelection(0); // Bilinéaire par défaut


}

ResizeParameter CResizeDialog::GetParameter() const
{
    ResizeParameter parameter;
    parameter.width = spinPixelWidth->GetValue();
    parameter.height = spinPixelHeight->GetValue();
    parameter.interpolation = comboInterpolation->GetSelection();

    return parameter;
}

void CResizeDialog::OnWidthChanged(wxSpinEvent& event)
{

}

void CResizeDialog::OnHeightChanged(wxSpinEvent& event)
{

}
