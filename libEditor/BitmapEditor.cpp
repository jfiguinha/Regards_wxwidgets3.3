#include <header.h>
#include "BitmapEditor.h"
#include <ViewerTheme.h>
#include <ViewerThemeInit.h>
using namespace Regards::Viewer;

CBitmapEditor::CBitmapEditor(CSliderInterface* slider, wxWindowID mainViewerId, const CThemeBitmapWindow& theme,
	CBitmapInterface* bitmapInterface) : Regards::Control::CBitmapWndViewer(slider, mainViewerId, theme, bitmapInterface)
{
	fixArrow = false;
}

CBitmapEditor::~CBitmapEditor(void)
{}

void CBitmapEditor::SetRealSize()
{
	ratio = 1.0f;
	this->RefreshWindow();
}

void CBitmapEditor::SetFilename(const wxString& filename)
{
	this->filename = filename;
}