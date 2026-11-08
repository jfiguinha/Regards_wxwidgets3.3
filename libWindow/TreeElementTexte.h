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
		void SetClick(const bool& value);
		void SetTheme(CThemeTreeTexte* theme);
		void SetPosition(const int& position);

		// --- Style "moderne" (tous optionnels : par defaut le rendu reste inchange) ---
		void SetTextColor(const wxColour& color);          // couleur du texte (libelle attenue, valeur, ...)
		void SetBold(const bool& bold);                    // gras (titres de section)
		void SetInsets(const int& left, const int& right); // marges internes gauche/droite du texte dans la zone
		void SetEllipsize(const bool& value);              // tronque avec "..." si le texte depasse la zone
		void SetRowStyle(const wxColour& background, const wxColour& border); // fond uni + filet 1 px en bas
		const wxString& GetLibelle() const { return libelle; }

		int GetWidth() override
		{
			return themeTexte.GetWidth();
		}

		int GetHeight() override
		{
			return themeTexte.GetHeight();
		}

	protected:

		wxSize GetSizeText();
		wxSize MeasureText(const wxString& text);
		wxString GetDisplayText(const int& maxWidth, wxSize& displaySize);


		wxSize textSize = {};
		bool canUpdate;
		bool isClick;
		wxString libelle;
		int position;

		// Style moderne
		int insetLeft = 0;
		int insetRight = 0;
		bool ellipsize = false;
		bool hasRowStyle = false;
		wxColour rowBackground;
		wxColour rowBorder;

		// Cache de la troncature
		int cacheWidth = -1;
		wxString cacheText;
		wxSize cacheSize = {};

		CThemeTreeTexte themeTexte;
	};
}
