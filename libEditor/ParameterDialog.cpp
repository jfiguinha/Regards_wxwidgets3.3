#include <header.h>
#include "ParameterDialog.h"
#include <ViewerTheme.h>
#include <ViewerThemeInit.h>
using namespace Regards::Viewer;
using namespace Regards::Control;

ParameterDialog::ParameterDialog(wxWindow* parent)
    : wxDialog(parent, wxID_ANY, "Paramètres", wxDefaultPosition, wxSize(250, 350),
        wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER | wxSTAY_ON_TOP) // wxSTAY_ON_TOP la garde visible au-dessus des images
{
	CMainTheme* viewerTheme = CMainThemeInit::getInstance();

	if (viewerTheme != nullptr)
	{
		CThemeScrollBar themeScroll;
		viewerTheme->GetScrollTheme(&themeScroll);

		CThemeTree themeTree;
		viewerTheme->GetTreeTheme(&themeTree);

		filtreEffectWnd = new CFiltreEffectScrollWnd(this, wxID_ANY, themeScroll, themeTree, BITMAPWINDOWVIEWERID);
		filtreEffectWnd->Show(true);
	}

	if (viewerTheme != nullptr)
	{
		CThemeToolbar theme;
		viewerTheme->GetFiltreToolbarTheme(&theme);
		filtreToolbar = new CFiltreToolbar(this, wxID_ANY, theme, false);
		filtreToolbar->Show(true);
	}


	
		
	Connect(wxEVT_SIZE, wxSizeEventHandler(ParameterDialog::OnSize));
	Connect(wxEVENT_FILTREOK, wxCommandEventHandler(ParameterDialog::OnFiltreOk));
	Connect(wxEVENT_FILTRECANCEL, wxCommandEventHandler(ParameterDialog::OnFiltreCancel));

	m_statusBar = new wxStatusBar(this, wxID_ANY);
	m_statusBar->SetFieldsCount(1); // 1 seule section textuelle
	m_statusBar->SetStatusText("");
}


void ParameterDialog::SetTypeFiltre(const int& typeFiltre)
{
	this->typeFiltre = typeFiltre;
}

void ParameterDialog::SetColor(const wxColour& color1, const wxColour& color2)
{
	this->color1 = color1;
	this->color2 = color2;

	if (filtreEffectWnd)
	{
		filtreEffectWnd->SetColor(color1, color2);
	}
}

void ParameterDialog::OnFiltreOk(wxCommandEvent& event)
{
	if(filtreEffectWnd)
		filtreEffectWnd->OnFiltreOk(numFiltre, historyEffectWnd);

	this->Show(false);
}

void ParameterDialog::OnFiltreCancel(wxCommandEvent& event)
{
	if(filtreEffectWnd)
		filtreEffectWnd->OnFiltreCancel();

	this->Show(false);
}

void ParameterDialog::SetFiltre(const int& numFiltre, CInfoEffectWnd* historyEffectWnd, const wxString &filename, const int& bitmapViewerId, const int& mainViewerId)
{
	if (filtreEffectWnd)
	{
		this->historyEffectWnd = historyEffectWnd;
		this->numFiltre = numFiltre;
		filtreToolbar->SetNumFiltre(numFiltre);
		filtreEffectWnd->SetParentBitmapId(mainViewerId);
		
		filtreEffectWnd->ApplyEffect(numFiltre, historyEffectWnd, filename, false, PANELINFOSWNDID,
			bitmapViewerId);

		filtreEffectWnd->Show(true);
	}
}


void ParameterDialog::OnSize(wxSizeEvent& event)
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

		if (typeFiltre == TYPE_EFFECT)
		{
			statusHeight = filtreToolbar->GetHeight();
			filtreToolbar->SetSize(0, _height - statusHeight, _width, statusHeight);
			_height -= statusHeight;
		}

	}

	filtreEffectWnd->SetSize(0, 0, _width, _height);
	filtreEffectWnd->Refresh();
	// scrollbar->Refresh();

}
