#include <header.h>
#include "EffectsDialog.h"

EffectsDialog::EffectsDialog(wxWindow* parent)
    : wxDialog(parent, wxID_ANY, "Effets d'image", wxDefaultPosition, wxSize(250, 350),
        wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER | wxSTAY_ON_TOP) // wxSTAY_ON_TOP la garde visible au-dessus des images
{

}

void EffectsDialog::SetFilename(const wxString& filename)
{
    m_filename = filename;
}