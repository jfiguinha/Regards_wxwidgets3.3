#include <header.h>
#include <CanvasSizeDialog.h>
#include <wx/xrc/xmlres.h>
#include <wx/slider.h>
#include <wx/combobox.h>
#include <wx/textctrl.h>
#include <wx/stattext.h>
#include <wx/spinctrl.h>
using namespace Regards::Dialog;

BEGIN_EVENT_TABLE(CCanvasSizeDialog, wxDialog)
//(*EventTable(ConfigRegards)
//*)
END_EVENT_TABLE()

CCanvasSizeDialog::CCanvasSizeDialog(
    wxWindow* parent,
    const CanvasSizeParameter& parameter)
{
    wxXmlResource::Get()->LoadDialog(
        this,
        parent,
        "ID_LAYER_PROPERTIES_DIALOG");

    m_width = XRCCTRL(*this, "ID_CANVAS_WIDTH", wxSpinCtrl);
    m_height = XRCCTRL(*this, "ID_CANVAS_HEIGHT", wxSpinCtrl);
    m_keepRatio = XRCCTRL(*this, "ID_KEEP_RATIO", wxCheckBox);
    m_position = XRCCTRL(*this, "ID_CANVAS_POSITION", wxRadioBox);
}

CanvasSizeParameter CCanvasSizeDialog::GetParameter() const
{
    CanvasSizeParameter canvasSize;
    return canvasSize;
}


void CCanvasSizeDialog::OnWidthChanged(wxSpinEvent& event)
{

}
    

void CCanvasSizeDialog::OnHeightChanged(wxSpinEvent& event)
{

}