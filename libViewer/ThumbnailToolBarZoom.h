#pragma once
#include <WindowMain.h>
using namespace Regards::Window;

namespace Regards::Viewer
{
	class CThumbnailToolBarZoom : public CWindowMain
	{
	public:
		CThumbnailToolBarZoom(wxWindow* parent, wxWindowID id, const CThemeToolBarZoom& theme);
		~CThumbnailToolBarZoom() = default;
		int GetHeight() override;
		void UpdateScreenRatio() override;

	private:
		enum class HoverZone
		{
			None,
			Previous,
			Next
		};

		void on_paint(wxPaintEvent& event);
		void OnLButtonDown(wxMouseEvent& event);
		void OnMouseMove(wxMouseEvent& event);
		void OnMouseLeave(wxMouseEvent& event);
		void Draw(wxDC* dc);
		void DrawPreviousElement(wxDC* dc, const wxString& libelle, const CThemeFont& themeFont);
		void DrawNextElement(wxDC* dc, const wxString& libelle, const CThemeFont& themeFont);
		void DrawElement(wxDC* dc, const wxString& libelle, const CThemeFont& themeFont);
		void DrawChevron(wxDC* dc, int x, int centerY, bool pointLeft);
		void DrawPill(wxDC* dc, const wxRect& rc, bool hover);
		void SetHoverZone(HoverZone zone);

		wxRect positionPrevious;
		wxRect positionNext;
		HoverZone hoverZone = HoverZone::None;

		int typeAffichage;
		CThemeToolBarZoom themeToolbarZoom;
		wxString criteriaPrevious;
		wxString criteriaNext;
		wxString allPhoto;
		wxString byYear;
		wxString byMonth;
		wxString byDay;
		wxString byLocalisation;
	};
}