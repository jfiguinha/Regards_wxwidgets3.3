#include <header.h>
#include "InfoDialog.h"

#include <EditorTheme.h>
#include <EditorThemeInit.h>
using namespace Regards::Editor;


InfoDialog::InfoDialog(wxWindow* parent)
    : wxDialog(parent, wxID_ANY, "Information", wxDefaultPosition, wxSize(250, 350),
        wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER | wxSTAY_ON_TOP) // wxSTAY_ON_TOP la garde visible au-dessus des images
{

	CMainTheme* viewerTheme = CMainThemeInit::getInstance();

	if (viewerTheme != nullptr)
	{
		CThemeScrollBar themeScroll;
		viewerTheme->GetScrollTheme(&themeScroll);

		CThemeTree theme;
		viewerTheme->GetTreeTheme(&theme);

		infosFileWnd = new CInfosFileWnd(this, wxID_ANY, themeScroll, theme);

		infosFileWnd->Show(true);
	}

	Connect(wxEVT_SIZE, wxSizeEventHandler(InfoDialog::OnSize));

	m_statusBar = new wxStatusBar(this, wxID_ANY);
	m_statusBar->SetFieldsCount(1); // 1 seule section textuelle
	m_statusBar->SetStatusText("");
}
void InfoDialog::SetFilename(const wxString& filename)
{
    m_filename = filename;
	infosFileWnd->InfosUpdate(filename);
	infosFileWnd->Show(true);
}


void InfoDialog::OnSize(wxSizeEvent& event)
{
    const wxSize clientSize = this->GetClientSize();
    int _width = clientSize.GetWidth();
    int _height = clientSize.GetHeight();

    if (_width <= 20 && _height <= 20)
    {
        return;
    }

	if (m_statusBar)
	{
		int statusHeight = m_statusBar->GetSize().GetHeight();
		m_statusBar->SetSize(0, _height - statusHeight, _width, statusHeight);


		_height -= statusHeight;
	}


	infosFileWnd->SetSize(0, 0, _width, _height);
	infosFileWnd->Refresh();
	// scrollbar->Refresh();

}
