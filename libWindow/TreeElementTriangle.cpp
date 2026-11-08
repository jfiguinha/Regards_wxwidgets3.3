#include "header.h"
#include "TreeElementTriangle.h"
using namespace Regards::Window;

CTreeElementTriangle::CTreeElementTriangle()
{
	open = false;
}

void CTreeElementTriangle::SetTheme(CThemeTreeTriangle* theme)
{
	themeTriangle = *theme;
}

void CTreeElementTriangle::SetOpen(const bool& open)
{
	this->open = open;
}

bool CTreeElementTriangle::GetOpen()
{
	return open;
}

void CTreeElementTriangle::DrawElement(wxDC* deviceContext, const int& x, const int& y)
{
	if (deviceContext == nullptr)
		return;

	if (chevronStyle)
	{
		DrawChevron(deviceContext, x, y);
		return;
	}

	if (open)
	{
		DrawTriangleOpen(deviceContext, x, y);
	}
	else
	{
		DrawTriangleClose(deviceContext, x, y);
	}
}

void CTreeElementTriangle::DrawTriangleClose(wxDC* dc, const int& x, const int& y)
{
	int marge = (themeTriangle.GetHeight() - themeTriangle.GetElementHeight()) / 2;

	// OPTIMISATION : Assigner la brosse avec la couleur du thème directement sans instancier de wxBrush local sur le tas
	dc->SetPen(*wxTRANSPARENT_PEN);
	dc->SetBrush(wxBrush(themeTriangle.color));

	wxPoint star[3];
	star[0] = wxPoint(x + themeTriangle.GetMarge(), y + marge);
	star[1] = wxPoint(x + themeTriangle.GetMarge(), y + marge + themeTriangle.GetElementHeight());
	star[2] = wxPoint(x + themeTriangle.GetElementWidth() + themeTriangle.GetMarge(),
		y + marge + (themeTriangle.GetElementHeight() / 2));

	dc->DrawPolygon(WXSIZEOF(star), star, 0, 0);
	dc->SetBrush(wxNullBrush);
}

void CTreeElementTriangle::ClickElement(wxWindow* window, const int& x, const int& y)
{
	open = !open;
}

void CTreeElementTriangle::DrawTriangleOpen(wxDC* dc, const int& x, const int& y)
{
	int marge = (themeTriangle.GetHeight() - themeTriangle.GetElementHeight()) / 2;

	// OPTIMISATION : Assigner la brosse avec la couleur du thème directement sans instancier de wxBrush local sur le tas
	dc->SetPen(*wxTRANSPARENT_PEN);
	dc->SetBrush(wxBrush(themeTriangle.color));

	wxPoint star[3];
	star[0] = wxPoint(x + themeTriangle.GetMarge(), y + marge + themeTriangle.GetElementHeight());
	star[1] = wxPoint(x + themeTriangle.GetMarge() + themeTriangle.GetElementWidth(), y + marge);
	star[2] = wxPoint(x + themeTriangle.GetElementWidth() + themeTriangle.GetMarge(),
		y + marge + themeTriangle.GetElementHeight());

	dc->DrawPolygon(WXSIZEOF(star), star, 0, 0);
	dc->SetBrush(wxNullBrush);
}

void CTreeElementTriangle::DrawChevron(wxDC* dc, const int& x, const int& y)
{
	if (hasRowStyle)
	{
		dc->SetPen(*wxTRANSPARENT_PEN);
		dc->SetBrush(wxBrush(rowBackground));
		dc->DrawRectangle(x, y, themeTriangle.GetWidth(), themeTriangle.GetHeight());

		dc->SetPen(wxPen(rowBorder, 1));
		dc->DrawLine(x, y + themeTriangle.GetHeight() - 1, x + themeTriangle.GetWidth(), y + themeTriangle.GetHeight() - 1);
		dc->SetBrush(wxNullBrush);
	}

	// Chevron centre dans la zone de l'element
	const int cx = x + themeTriangle.GetWidth() / 2;
	const int cy = y + themeTriangle.GetHeight() / 2;
	int s = wxMin(themeTriangle.GetElementHeight(), themeTriangle.GetHeight() / 2) / 2;
	s = wxMax(3, wxMin(s, 6));

	wxPen pen(themeTriangle.color, 2);
	pen.SetCap(wxCAP_ROUND);
	pen.SetJoin(wxJOIN_ROUND);
	dc->SetPen(pen);

	if (open)
	{
		// v
		dc->DrawLine(cx - s, cy - s / 2, cx, cy + s / 2);
		dc->DrawLine(cx, cy + s / 2, cx + s, cy - s / 2);
	}
	else
	{
		// >
		dc->DrawLine(cx - s / 2, cy - s, cx + s / 2, cy);
		dc->DrawLine(cx + s / 2, cy, cx - s / 2, cy + s);
	}

	dc->SetPen(wxNullPen);
}
