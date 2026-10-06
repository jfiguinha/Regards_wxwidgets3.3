#pragma once
#include "ToolbarTools.h"
using namespace Regards::Editor;

class ToolDialog : public wxDialog {
public:
    ToolDialog(wxWindow* parent);

private:

    void OnSize(wxSizeEvent& event);
    CToolbarTools* toolbarTools;

};
