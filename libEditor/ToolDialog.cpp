#include <header.h>
#include "ToolDialog.h"
#include "ToolbarTools.h"
#include "ColorSelectorWidget.h"
#include <ViewerTheme.h>
#include <ViewerThemeInit.h>
using namespace Regards::Viewer;
using namespace Regards::Editor;
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

wxColour ToolDialog::GetColor1()
{
	return colorSelector->GetColor1();
}

wxColour ToolDialog::GetColor2()
{
	return colorSelector->GetColor2();
}

// Méthode de callback :
void ToolDialog::OnColorChanged(wxCommandEvent& event)
{
	CColorSelectorWidget* selector = decltype(selector)(event.GetEventObject());
	if (selector)
	{
		wxCommandEvent evt(wxEVENT_COLORCHANGE, GetId());
		evt.SetEventObject(this);
		this->GetParent()->ProcessWindowEvent(evt);
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