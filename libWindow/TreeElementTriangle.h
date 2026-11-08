#pragma once
#include "TreeElement.h"
#include "TreeElementDelete.h"

namespace Regards::Window
{
	class CTreeElementTriangle : public CTreeElement
	{
	public:
		CTreeElementTriangle();
		~CTreeElementTriangle() = default;

		void SetOpen(const bool& open);
		bool GetOpen();
		void DrawElement(wxDC* deviceContextc, const int& x, const int& y) override;
		void ClickElement(wxWindow* window, const int& x, const int& y) override;
		void SetTheme(CThemeTreeTriangle* theme);

		// Style moderne : chevron (v ouvert, > ferme) au lieu du triangle plein
		void SetChevronStyle(const bool& value)
		{
			chevronStyle = value;
		}

		// Fond uni + filet 1 px en bas (pour que la ligne de titre soit continue)
		void SetRowStyle(const wxColour& background, const wxColour& border)
		{
			hasRowStyle = true;
			rowBackground = background;
			rowBorder = border;
		}

		void SetZoneSize(const int& width, const int& height) override
		{
			themeTriangle.SetWidth(width);
			themeTriangle.SetHeight(height);
		}

		int GetWidth() override
		{
			return themeTriangle.GetWidth();
		}

		int GetHeight() override
		{
			return themeTriangle.GetHeight();
		}

	private:
		void DrawTriangleClose(wxDC* deviceContext, const int& x, const int& y);
		void DrawTriangleOpen(wxDC* deviceContext, const int& x, const int& y);
		void DrawChevron(wxDC* deviceContext, const int& x, const int& y);

		bool open;
		bool chevronStyle = false;
		bool hasRowStyle = false;
		wxColour rowBackground;
		wxColour rowBorder;
		CThemeTreeTriangle themeTriangle;
	};
}
