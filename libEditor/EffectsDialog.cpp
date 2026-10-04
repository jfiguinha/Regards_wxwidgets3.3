#include <header.h>
#include "EffectsDialog.h"
#include <EditorTheme.h>
#include <EditorThemeInit.h>
using namespace Regards::Editor;
using namespace Regards::Control;

EffectsDialog::EffectsDialog(wxWindow* parent)
    : wxDialog(parent, wxID_ANY, "List Effects", wxDefaultPosition, wxSize(250, 350),
        wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER | wxSTAY_ON_TOP) // wxSTAY_ON_TOP la garde visible au-dessus des images
{
	CMainTheme* viewerTheme = CMainThemeInit::getInstance();

	if (viewerTheme != nullptr)
	{
		bool checkValidity = false;

		CThemeScrollBar themeScroll;
		viewerTheme->GetScrollTheme(&themeScroll);

		CThemeThumbnail themeThumbnail;
		viewerTheme->GetThumbnailTheme(&themeThumbnail);

		thumbnailEffectWnd = new CThumbnailViewerEffectWnd(this, wxID_ANY, themeScroll, themeThumbnail, PANELINFOSWNDID,
			false);

		thumbnailEffectWnd->Show(true);

	}

	Connect(wxEVT_SIZE, wxSizeEventHandler(EffectsDialog::OnSize));


	m_statusBar = new wxStatusBar(this, wxID_ANY);
	m_statusBar->SetFieldsCount(1); // 1 seule section textuelle
	m_statusBar->SetStatusText("");
}

void EffectsDialog::SetFilename(const wxString& filename)
{
	m_filename = filename;
	thumbnailEffectWnd->SetFile(filename);
	thumbnailEffectWnd->Show(true);
}


void EffectsDialog::OnSize(wxSizeEvent& event)
{
	const wxSize clientSize = this->GetClientSize();
	int _width = clientSize.GetWidth();
	int _height = clientSize.GetHeight();

	if (_width <= 20 && _height <= 20)
	{
		return;
	}

	thumbnailEffectWnd->SetSize(0, 0, _width, _height);
	thumbnailEffectWnd->Refresh();
	// scrollbar->Refresh();

}
