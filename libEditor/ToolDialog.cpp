#include <header.h>
#include "ToolDialog.h"

ToolDialog::ToolDialog(wxWindow* parent)
    : wxDialog(parent, wxID_ANY, "Outils", wxDefaultPosition, wxSize(250, 350),
        wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER | wxSTAY_ON_TOP) // wxSTAY_ON_TOP la garde visible au-dessus des images
{

}