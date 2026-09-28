#include "header.h"
#include "TreeElementColor.h"
#include <wx/colordlg.h> // Pour utiliser wxColourDialog

using namespace Regards::Window;

CTreeElementColor::CTreeElementColor()
{
	m_selectedColor = wxColour(255, 0, 0); // Rouge par défaut au démarrage
	m_width = DEFAULT_WIDTH;
	m_height = DEFAULT_HEIGHT;
}

void CTreeElementColor::SetColor(const wxColour& color)
{
	m_selectedColor = color;
}

wxColour CTreeElementColor::GetColor() const
{
	return m_selectedColor;
}

void CTreeElementColor::SetZoneSize(const int& width, const int& height)
{
	m_width = width;
	m_height = height;
}

int CTreeElementColor::GetWidth()
{
	return m_width;
}

int CTreeElementColor::GetHeight()
{
	return m_height;
}

void CTreeElementColor::ClickElement(wxWindow* window, const int& x, const int& y)
{
	if (window == nullptr) return;

	// Initialisation des données de configuration de la boîte de dialogue avec la couleur courante
	wxColourData colorData;
	colorData.SetChooseFull(true); // Permet d'avoir le sélecteur complet (teinte, saturation, luminosité)
	colorData.SetColour(m_selectedColor);

	// Instanciation et affichage de la boîte de dialogue de sélection de couleur native
	wxColourDialog dialog(window, &colorData);

	if (dialog.ShowModal() == wxID_OK)
	{
		// Si l'utilisateur clique sur "OK", on extrait la nouvelle couleur choisie
		wxColourData retData = dialog.GetColourData();
		m_selectedColor = retData.GetColour();

		// Demande le rafraîchissement graphique immédiat de la fenêtre hôte (l'arbre de propriétés)
		window->Refresh();
	}
}

void CTreeElementColor::DrawElement(wxDC* deviceContext, const int& x, const int& y)
{
	if (deviceContext == nullptr) return;

	// 1. Dessin du fond optionnel fourni par la ligne de l'arbre (backcolor)
	if (backcolor.IsOk())
	{
		wxPen backPen(backcolor, 1, wxPENSTYLE_SOLID);
		wxBrush backBrush(backcolor, wxBRUSHSTYLE_SOLID);
		deviceContext->SetPen(backPen);
		deviceContext->SetBrush(backBrush);
		deviceContext->DrawRectangle(x, y, m_width, m_height);
	}

	// 2. Alignement et calcul de la boîte du rectangle de couleur
	// On applique une petite marge interne pour que le rectangle soit esthétique dans la cellule
	int margeX = 4;
	int margeY = 2;
	int rectWidth = m_width - (margeX * 2);
	int rectHeight = m_height - (margeY * 2);

	int posX = x + margeX;
	int posY = y + margeY;

	// 3. Dessin du rectangle coloré choisi par l'utilisateur
	// On applique une fine bordure gris foncé/noire autour du rectangle pour qu'il reste visible si la couleur choisie est blanche
	wxPen borderPen(wxColour(60, 60, 60), 1, wxPENSTYLE_SOLID);
	wxBrush colorBrush(m_selectedColor, wxBRUSHSTYLE_SOLID);

	deviceContext->SetPen(borderPen);
	deviceContext->SetBrush(colorBrush);

	// Dessine le rectangle plein affichant la couleur
	deviceContext->DrawRectangle(posX, posY, rectWidth, rectHeight);

	// 4. Nettoyage du contexte de périphérique (Restauration des objets système par défaut)
	deviceContext->SetBrush(wxNullBrush);
	deviceContext->SetPen(wxNullPen);
}
