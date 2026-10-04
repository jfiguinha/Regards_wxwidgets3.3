#include <header.h>
#include "ParameterDialog.h"

ParameterDialog::ParameterDialog(wxWindow* parent)
    : wxDialog(parent, wxID_ANY, "Paramètres", wxDefaultPosition, wxSize(250, 350),
        wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER | wxSTAY_ON_TOP) // wxSTAY_ON_TOP la garde visible au-dessus des images
{

}