#pragma once
#include "TreeElement.h"
#include <wx/clrpicker.h> // Requis pour les boîtes de dialogue de couleur

namespace Regards::Window
{
	class CTreeElementColor : public CTreeElement
	{
	public:
		CTreeElementColor();
		virtual ~CTreeElementColor() = default;

		// Surcharges obligatoires de CTreeElement
		void DrawElement(wxDC* deviceContext, const int& x, const int& y) override;
		void ClickElement(wxWindow* window, const int& x, const int& y) override;

		void SetZoneSize(const int& width, const int& height) override;
		int GetWidth() override;
		int GetHeight() override;

		// Accesseurs pour la couleur choisie par l'utilisateur
		void SetColor(const wxColour& color);
		wxColour GetColor() const;

	private:
		wxColour m_selectedColor; // La couleur choisie (dessinée dans le rectangle)
		int m_width;              // Largeur de l'élément graphique
		int m_height;             // Hauteur de l'élément graphique

		static constexpr int DEFAULT_WIDTH = 40;  // Taille par défaut si non spécifiée
		static constexpr int DEFAULT_HEIGHT = 20;
	};
}
#pragma once
