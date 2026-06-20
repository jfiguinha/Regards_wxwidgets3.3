#pragma once
#include "TreeElement.h"
#include "TreeElementDelete.h"

namespace Regards::Window
{
	class CTreeElementTexte : public CTreeElement
	{
	public:
		CTreeElementTexte();
		~CTreeElementTexte() = default;

		CTreeElementTexte& operator=(const CTreeElementTexte& other);

		void IsUpdatable(const bool& value)
		{
			canUpdate = value;
		};

		void SetZoneSize(const int& width, const int& height) override
		{
			themeTexte.SetWidth(width);
			themeTexte.SetHeight(height);
		}

		void MouseOver(wxDC* deviceContext, const int& x, const int& y, bool& update) override;
		void DrawElement(wxDC* deviceContext, const int& x, const int& y) override;
		void SetLibelle(const wxString& libelle);

		void SetTheme(CThemeTreeTexte* theme);
		void SetPosition(const int& position);

		int GetWidth() override
		{
			return themeTexte.GetWidth();
		}

		int GetHeight() override
		{
			return themeTexte.GetHeight();
		}

	protected:
		void DrawText(wxDC* dc, const int& xPos, const int& yPos);
		wxSize GetSizeText();
		void GenerateFont();
		bool textSizeValid;
		bool canUpdate;
		wxString libelle;
		int position;
		wxFont font;
		bool isFontOk = false;
		wxSize cachedTextSize;
		CThemeTreeTexte themeTexte;
	};
}
