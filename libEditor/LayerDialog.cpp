#include <header.h>
#include "LayerDialog.h"
#include <ViewerTheme.h>
#include <ViewerThemeInit.h>
using namespace Regards::Viewer;
using namespace Regards::Control;

LayerDialog::LayerDialog(wxWindow* parent)
    : wxDialog(parent, wxID_ANY, "List of Layers", wxDefaultPosition, wxSize(250, 350),
        wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER | wxSTAY_ON_TOP) // wxSTAY_ON_TOP la garde visible au-dessus des images
{

	CMainTheme* viewerTheme = CMainThemeInit::getInstance();

	if (viewerTheme != nullptr)
	{
		listLayer = new CListLayer(this, LISTLAYERID);
		listLayer->Show(true);
	}

	Connect(wxEVT_SIZE, wxSizeEventHandler(LayerDialog::OnSize));

	m_statusBar = new wxStatusBar(this, wxID_ANY);
	m_statusBar->SetFieldsCount(1); // 1 seule section textuelle
	m_statusBar->SetStatusText("");
}


void LayerDialog::SetFile(const wxString& filename)
{
	if (listLayer)
	{
		listLayer->SetFilename(filename);
		listLayer->Show(true);
	}
}


void LayerDialog::OnSize(wxSizeEvent& event)
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


	listLayer->SetSize(0, 0, _width, _height - m_statusBar->GetClientSize().GetHeight());
	listLayer->Refresh();
	// scrollbar->Refresh();

}
