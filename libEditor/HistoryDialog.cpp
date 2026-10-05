#include <header.h>
#include "HistoryDialog.h"

#include <EditorTheme.h>
#include <EditorThemeInit.h>
using namespace Regards::Editor;
using namespace Regards::Control;

HistoryDialog::HistoryDialog(wxWindow* parent)
    : wxDialog(parent, wxID_ANY, "History", wxDefaultPosition, wxSize(250, 350),
        wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER | wxSTAY_ON_TOP) // wxSTAY_ON_TOP la garde visible au-dessus des images
{
	CMainTheme* viewerTheme = CMainThemeInit::getInstance();

	if (viewerTheme != nullptr)
	{
		CThemeScrollBar themeScroll;
		viewerTheme->GetScrollTheme(&themeScroll);

		CThemeTree themeTree;
		viewerTheme->GetTreeTheme(&themeTree);
		historyEffectWnd = new CInfoEffectWnd(this, wxID_ANY, themeScroll, themeTree, BITMAPWINDOWVIEWERID, false);
		historyEffectWnd->Show(true);
	}


	Connect(wxEVT_SIZE, wxSizeEventHandler(HistoryDialog::OnSize));

	m_statusBar = new wxStatusBar(this, wxID_ANY);
	m_statusBar->SetFieldsCount(1); // 1 seule section textuelle
	m_statusBar->SetStatusText("");
}

void HistoryDialog::SetHistoryControl(CInfoEffect* infoEffect)
{
	historyEffectWnd->SetHistoryEffect(infoEffect);
}

void HistoryDialog::OnSize(wxSizeEvent& event)
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


	historyEffectWnd->SetSize(0, 0, _width, _height);
	historyEffectWnd->Refresh();
	// scrollbar->Refresh();

}
