#include <header.h>
#include "ToolDialog.h"
#include <ViewerTheme.h>
#include <ViewerThemeInit.h>
using namespace Regards::Viewer;

ToolDialog::ToolDialog(wxWindow* parent)
    : wxDialog(parent, wxID_ANY, "Tools", wxDefaultPosition, wxSize(120, 280),
        wxDEFAULT_DIALOG_STYLE |  wxSTAY_ON_TOP) // wxSTAY_ON_TOP la garde visible au-dessus des images
{
	CMainTheme* viewerTheme = CMainThemeInit::getInstance();

	if (viewerTheme != nullptr)
	{
		CThemeToolbar theme;
		viewerTheme->GetThumbnailToolbarTheme(theme);
		theme.SetNbCol(2);
		theme.button.SetTailleX(50);
		theme.button.SetTailleY(50);
		toolbarTools = new CToolbarTools(this, wxID_ANY, theme, true);
		toolbarTools->Show(true);
	}

	Connect(wxEVT_SIZE, wxSizeEventHandler(ToolDialog::OnSize));
}

void ToolDialog::OnSize(wxSizeEvent& event)
{
	const wxSize clientSize = this->GetClientSize();
	int _width = clientSize.GetWidth();
	int _height = clientSize.GetHeight();

	if (_width <= 20 && _height <= 20)
	{
		return;
	}


	toolbarTools->SetSize(0, 0, _width, _height);
	toolbarTools->Refresh();
	// scrollbar->Refresh();
}