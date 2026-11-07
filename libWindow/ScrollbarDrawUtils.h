#pragma once
#include <memory>
#include <wx/dc.h>
#include <wx/geometry.h>
#include <wx/graphics.h>

// Primitives de dessin antialiasées partagées par les scrollbars horizontale/verticale.
// Chaque fonction crée un wxGraphicsContext à partir du wxDC reçu, dessine, puis le détruit
// avant tout autre dessin sur le wxDC. Si le contexte ne peut pas être créé, repli sur wxDC
// (sans antialiasing).
namespace Regards::Window::ScrollbarDraw
{
	inline std::unique_ptr<wxGraphicsContext> CreateContext(wxDC* dc)
	{
		std::unique_ptr<wxGraphicsContext> gc(wxGraphicsContext::CreateFromUnknownDC(*dc));
		if (gc)
			gc->SetAntialiasMode(wxANTIALIAS_DEFAULT);
		return gc;
	}

	inline void FillTriangle(wxDC* dc, const wxPoint2DDouble(&pts)[3], const wxColour& color)
	{
		if (auto gc = CreateContext(dc))
		{
			gc->SetPen(*wxTRANSPARENT_PEN);
			gc->SetBrush(wxBrush(color));

			wxGraphicsPath path = gc->CreatePath();
			path.MoveToPoint(pts[0]);
			path.AddLineToPoint(pts[1]);
			path.AddLineToPoint(pts[2]);
			path.CloseSubpath();
			gc->FillPath(path);
			return;
		}

		const wxPoint poly[3] = {
			wxPoint(wxRound(pts[0].m_x), wxRound(pts[0].m_y)),
			wxPoint(wxRound(pts[1].m_x), wxRound(pts[1].m_y)),
			wxPoint(wxRound(pts[2].m_x), wxRound(pts[2].m_y))
		};
		dc->SetPen(*wxTRANSPARENT_PEN);
		dc->SetBrush(wxBrush(color));
		dc->DrawPolygon(3, poly, 0, 0);
		dc->SetBrush(wxNullBrush);
		dc->SetPen(wxNullPen);
	}

	// Coordonnées en double : permet un centrage exact (demi-pixel) grâce à l'antialiasing.
	inline void FillRoundedRect(wxDC* dc, double x, double y, double w, double h, double radius,
		const wxColour& color)
	{
		if (auto gc = CreateContext(dc))
		{
			gc->SetPen(*wxTRANSPARENT_PEN);
			gc->SetBrush(wxBrush(color));
			gc->DrawRoundedRectangle(x, y, w, h, radius);
			return;
		}

		dc->SetPen(*wxTRANSPARENT_PEN);
		dc->SetBrush(wxBrush(color));
		dc->DrawRoundedRectangle(wxRound(x), wxRound(y), wxRound(w), wxRound(h), radius);
		dc->SetBrush(wxNullBrush);
		dc->SetPen(wxNullPen);
	}
}