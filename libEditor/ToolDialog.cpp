#include <header.h>
#include "ToolDialog.h"
#include "ColorSelectorWidget.h"
#include <ViewerTheme.h>
#include <ViewerThemeInit.h>
using namespace Regards::Viewer;

ToolDialog::ToolDialog(wxWindow* parent)
    : wxDialog(parent, wxID_ANY, "Tools", wxDefaultPosition, wxSize(140, 400),
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
		toolbarTools = new CToolbarTools(this, wxID_ANY, theme, this->GetParent(), true);
		toolbarTools->Show(true);
	}
	colorSelector = new CColorSelectorWidget(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, *wxRED, *wxBLUE);
	Connect(wxEVT_SIZE, wxSizeEventHandler(ToolDialog::OnSize));
	Bind(wxEVT_COLOR_SELECTOR_CHANGED, &ToolDialog::OnColorChanged, this, colorSelector->GetId());
}

// Méthode de callback :
void ToolDialog::OnColorChanged(wxCommandEvent& event)
{
	CColorSelectorWidget* selector = decltype(selector)(event.GetEventObject());
	if (selector)
	{
		wxColour c1 = selector->GetColor1();
		wxColour c2 = selector->GetColor2();
		// Faites ce que vous voulez avec vos nouvelles couleurs !
	}
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



	//280
	toolbarTools->SetSize(0, 0, _width, 280);
	toolbarTools->Refresh();
	colorSelector->SetSize(0, 280, _width, 120);
	colorSelector->Refresh();
}