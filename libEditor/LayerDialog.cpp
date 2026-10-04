#include <header.h>
#include "LayerDialog.h"

LayerDialog::LayerDialog(wxWindow* parent)
    : wxDialog(parent, wxID_ANY, "Effets d'image", wxDefaultPosition, wxSize(250, 350),
        wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER | wxSTAY_ON_TOP) // wxSTAY_ON_TOP la garde visible au-dessus des images
{

}