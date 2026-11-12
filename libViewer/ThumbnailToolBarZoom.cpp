#include <header.h>
#include "ThumbnailToolBarZoom.h"
#include <LibResource.h>
#include "ListPicture.h"
#include <window_id.h>
#include "ViewerParam.h"
#include "ViewerParamInit.h"
#include "MainWindow.h"
#include <wx/dcbuffer.h>
#include <algorithm>
using namespace Regards::Viewer;

namespace
{
	// Fond sombre ou clair : sert a choisir la couleur des chevrons et du separateur
	bool IsDarkColor(const wxColour& c)
	{
		return (c.Red() * 299 + c.Green() * 587 + c.Blue() * 114) / 1000 < 128;
	}
}

CThumbnailToolBarZoom::CThumbnailToolBarZoom(wxWindow* parent, wxWindowID id, const CThemeToolBarZoom& theme)
	: CWindowMain("CThumbnailToolBarZoom", parent, id)
{
	typeAffichage = 0;

	positionPrevious = wxRect(0, 0, 0, 0);
	positionNext = wxRect(0, 0, 0, 0);
	themeToolbarZoom = theme;
	criteriaPrevious = CLibResource::LoadStringFromResource(L"LBLCriteriaPrevious", 1);
	criteriaNext = CLibResource::LoadStringFromResource(L"LBLCriteriaNext", 1);
	allPhoto = CLibResource::LoadStringFromResource(L"LBLAllPhoto", 1);
	byYear = CLibResource::LoadStringFromResource(L"LBLByYear", 1);
	byMonth = CLibResource::LoadStringFromResource(L"LBLByMonth", 1);
	byDay = CLibResource::LoadStringFromResource(L"LBLByDay", 1);
	byLocalisation = CLibResource::LoadStringFromResource(L"LBLByLocalisation", 1);

	CMainParam* config = CMainParamInit::getInstance();
	if (config != nullptr)
	{
		typeAffichage = config->GetTypeAffichage();
	}

	Connect(wxEVT_PAINT, wxPaintEventHandler(CThumbnailToolBarZoom::on_paint));
	Connect(wxEVT_LEFT_DOWN, wxMouseEventHandler(CThumbnailToolBarZoom::OnLButtonDown));
	Connect(wxEVT_MOTION, wxMouseEventHandler(CThumbnailToolBarZoom::OnMouseMove));
	Connect(wxEVT_LEAVE_WINDOW, wxMouseEventHandler(CThumbnailToolBarZoom::OnMouseLeave));
}

int CThumbnailToolBarZoom::GetHeight()
{
	return themeToolbarZoom.GetHeight();
}

void CThumbnailToolBarZoom::SetHoverZone(HoverZone zone)
{
	if (zone == hoverZone)
		return;

	hoverZone = zone;
	SetCursor(wxCursor(zone == HoverZone::None ? wxCURSOR_ARROW : wxCURSOR_HAND));
	needToRefresh = true;
}

void CThumbnailToolBarZoom::OnMouseMove(wxMouseEvent& event)
{
	const wxPoint pt = event.GetPosition();

	if (positionPrevious.Contains(pt))
		SetHoverZone(HoverZone::Previous);
	else if (positionNext.Contains(pt))
		SetHoverZone(HoverZone::Next);
	else
		SetHoverZone(HoverZone::None);
}

void CThumbnailToolBarZoom::OnMouseLeave(wxMouseEvent& event)
{
	SetHoverZone(HoverZone::None);
}

void CThumbnailToolBarZoom::OnLButtonDown(wxMouseEvent& event)
{
	bool update = false;
	const wxPoint pt = event.GetPosition();

	if (positionPrevious.Contains(pt))
	{
		switch (typeAffichage)
		{
		case 1: //By Year
		case 2: //By Month
		case 3: //By Day
		case 4: //By Localisation
			typeAffichage--;
			update = true;
			break;
		}
	}
	else if (positionNext.Contains(pt))
	{
		switch (typeAffichage)
		{
		case 0: //All photo
		case 1: //By Year
		case 2: //By Month
		case 3: //By Day
			typeAffichage++;
			update = true;
			break;
		}
	}

	if (update)
	{
		wxWindow* mainWindow = static_cast<CMainWindow*>(this->FindWindowById(CENTRALVIEWERWINDOWID));
		if (mainWindow != nullptr)
		{
			wxCommandEvent evt(wxEVENT_CHANGETYPEAFFICHAGE);
			evt.SetExtraLong(typeAffichage);
			mainWindow->GetEventHandler()->AddPendingEvent(evt);
		}

		// Les zones changent avec le mode : on repart sans survol
		hoverZone = HoverZone::None;
		needToRefresh = true;
	}
}

// Chevron dessine en polyligne : taille proportionnelle a la hauteur de la barre
void CThumbnailToolBarZoom::DrawChevron(wxDC* dc, int x, int centerY, bool pointLeft)
{
	const int h = GetWindowHeight();
	const int cw = std::max(4, h / 6);
	const int ch = std::max(8, h / 3);

	wxPen pen(IsDarkColor(themeToolbarZoom.colorBack) ? wxColour(210, 210, 210) : wxColour(60, 60, 60),
		std::max(2, h / 16));
	pen.SetCap(wxCAP_ROUND);
	pen.SetJoin(wxJOIN_ROUND);
	dc->SetPen(pen);

	wxPoint pts[3];
	if (pointLeft)
	{
		pts[0] = wxPoint(x + cw, centerY - ch / 2);
		pts[1] = wxPoint(x, centerY);
		pts[2] = wxPoint(x + cw, centerY + ch / 2);
	}
	else
	{
		pts[0] = wxPoint(x, centerY - ch / 2);
		pts[1] = wxPoint(x + cw, centerY);
		pts[2] = wxPoint(x, centerY + ch / 2);
	}
	dc->DrawLines(3, pts);
	dc->SetPen(wxNullPen);
}

// Pilule de fond, visible uniquement au survol
void CThumbnailToolBarZoom::DrawPill(wxDC* dc, const wxRect& rc, bool hover)
{
	if (!hover)
		return;

	const wxColour fill = IsDarkColor(themeToolbarZoom.colorBack)
		? themeToolbarZoom.colorBack.ChangeLightness(135)
		: themeToolbarZoom.colorBack.ChangeLightness(92);
	dc->SetPen(*wxTRANSPARENT_PEN);
	dc->SetBrush(wxBrush(fill));
	dc->DrawRoundedRectangle(rc, rc.height / 2.0);
	dc->SetBrush(wxNullBrush);
	dc->SetPen(wxNullPen);
}

void CThumbnailToolBarZoom::DrawPreviousElement(wxDC* dc, const wxString& libelle, const CThemeFont& themeFont)
{
	const int h = GetWindowHeight();
	const wxSize element = GetSizeTexte(dc, libelle, themeFont);

	const int marginX = h / 4;
	const int marginY = h / 8;
	const int padX = h * 3 / 10;
	const int cw = std::max(4, h / 6);
	const int gap = padX / 2;

	positionPrevious = wxRect(marginX, marginY, padX + cw + gap + element.x + padX, h - 2 * marginY);

	DrawPill(dc, positionPrevious, hoverZone == HoverZone::Previous);
	DrawChevron(dc, positionPrevious.x + padX, h / 2, true);
	DrawTexte(dc, libelle, positionPrevious.x + padX + cw + gap, (h - element.y) / 2, themeFont);
}

void CThumbnailToolBarZoom::DrawNextElement(wxDC* dc, const wxString& libelle, const CThemeFont& themeFont)
{
	const int h = GetWindowHeight();
	const wxSize element = GetSizeTexte(dc, libelle, themeFont);

	const int marginX = h / 4;
	const int marginY = h / 8;
	const int padX = h * 3 / 10;
	const int cw = std::max(4, h / 6);
	const int gap = padX / 2;

	const int pillWidth = padX + element.x + gap + cw + padX;
	positionNext = wxRect(GetWindowWidth() - marginX - pillWidth, marginY, pillWidth, h - 2 * marginY);

	DrawPill(dc, positionNext, hoverZone == HoverZone::Next);
	DrawTexte(dc, libelle, positionNext.x + padX, (h - element.y) / 2, themeFont);
	DrawChevron(dc, positionNext.x + padX + element.x + gap, h / 2, false);
}

void CThumbnailToolBarZoom::DrawElement(wxDC* dc, const wxString& libelle, const CThemeFont& themeFont)
{
	wxSize element = GetSizeTexte(dc, libelle, themeFont);
	int posY = (GetWindowHeight() - element.y) / 2;
	int posX_center = (GetWindowWidth() - element.x) / 2;
	DrawTexte(dc, libelle, posX_center, posY, themeFont);
}

void CThumbnailToolBarZoom::UpdateScreenRatio()
{
	this->Resize();
}


void CThumbnailToolBarZoom::Draw(wxDC* dc)
{
	FillRect(dc, GetWindowRect(), themeToolbarZoom.colorBack);

	// Zones cliquables remises a zero : seules celles dessinees ci-dessous sont actives
	positionPrevious = wxRect(0, 0, 0, 0);
	positionNext = wxRect(0, 0, 0, 0);

	// Les libelles precedent / suivant n'utilisent plus criteriaPrevious / criteriaNext
	// (remplaces par les chevrons dessines)
	switch (typeAffichage)
	{
	case 0: //All photo
		DrawElement(dc, allPhoto, themeToolbarZoom.font);
		DrawNextElement(dc, byYear, themeToolbarZoom.font);
		break;

	case 1: //By Year
		DrawPreviousElement(dc, allPhoto, themeToolbarZoom.font);
		DrawElement(dc, byYear, themeToolbarZoom.font);
		DrawNextElement(dc, byMonth, themeToolbarZoom.font);
		break;

	case 2: //By Month
		DrawPreviousElement(dc, byYear, themeToolbarZoom.font);
		DrawElement(dc, byMonth, themeToolbarZoom.font);
		DrawNextElement(dc, byDay, themeToolbarZoom.font);
		break;

	case 3: //By Day
		DrawPreviousElement(dc, byMonth, themeToolbarZoom.font);
		DrawElement(dc, byDay, themeToolbarZoom.font);
		DrawNextElement(dc, byLocalisation, themeToolbarZoom.font);
		break;

	case 4: //By Localisation
		DrawPreviousElement(dc, byDay, themeToolbarZoom.font);
		DrawElement(dc, byLocalisation, themeToolbarZoom.font);
		break;
	}

	// Fin separateur en bas de la barre
	const bool dark = IsDarkColor(themeToolbarZoom.colorBack);
	dc->SetPen(wxPen(themeToolbarZoom.colorBack.ChangeLightness(dark ? 150 : 85), 1));
	dc->DrawLine(0, GetWindowHeight() - 1, GetWindowWidth(), GetWindowHeight() - 1);
	dc->SetPen(wxNullPen);
}


void CThumbnailToolBarZoom::on_paint(wxPaintEvent& event)
{
	int width = GetWindowWidth();
	int height = GetWindowHeight();
	if (width <= 0 || height <= 0)
		return;

	wxBufferedPaintDC dc(this);
	Draw(&dc);
}