#include "header.h"
#include "TreeElementTexte.h"
#include <WindowUtility.h>
#include <wx/dcmemory.h> // Assurez-vous d'inclure ce header
using namespace Regards::Window;

CTreeElementTexte::CTreeElementTexte()
{
	canUpdate = false;
	isClick = false;
	position = RENDERFONT_LEFT;
	textSize = wxSize(0, 0);
}

CTreeElementTexte& CTreeElementTexte::operator=(const CTreeElementTexte& other)
{
	visible = other.visible;
	xPos = other.xPos;
	yPos = other.yPos;
	numRow = other.numRow;
	numColumn = other.numColumn;
	themeTexte = other.themeTexte;
	canUpdate = other.canUpdate;
	isClick = other.isClick;
	libelle = other.libelle;
	position = other.position;
	textSize = other.textSize; // On copie la taille textuelle stockée
	return *this;
}

void CTreeElementTexte::SetTheme(CThemeTreeTexte* theme)
{
	themeTexte = *theme;
	textSize = GetSizeText(); // Recalcul immédiat et unique lors du changement de thème
}

void CTreeElementTexte::MouseOver(wxDC* deviceContext, const int& x, const int& y, bool& update)
{
	if (canUpdate)
		wxSetCursor(wxCursor(wxCURSOR_IBEAM));
	else
		wxSetCursor(wxCursor(wxCURSOR_HAND));

	update = false;
}

void CTreeElementTexte::SetLibelle(const wxString& libelle)
{
	this->libelle = libelle;

	// OPTIMISATION : On calcule la taille UNE SEULE FOIS ici, pas pendant le dessin
	textSize = GetSizeText();

	if (themeTexte.GetWidth() < textSize.x)
		themeTexte.SetWidth(textSize.x);

	if (themeTexte.GetHeight() < textSize.y)
		themeTexte.SetHeight(textSize.y);
}

wxSize CTreeElementTexte::GetSizeText()
{
	if (libelle.IsEmpty())
		return wxSize(0, 0);

	// SOLUTION POUR LA TAILLE CORRECTE :
	// Au lieu de recréer une bitmap de 250x250 à chaque appel (ce qui consommait de la mémoire),
	// on utilise une bitmap factice de 1x1 pixel. Pour mesurer du texte, la taille de la bitmap n'importe pas,
	// seul le fait d'avoir un contexte de mémoire valide avec la bonne police compte.
	static wxBitmap dummyBitmap(1, 1);
	wxMemoryDC dc(dummyBitmap);

	return CWindowUtility::GetSizeTexte(&dc, libelle, themeTexte.font);
}

void CTreeElementTexte::SetPosition(const int& position)
{
	this->position = position;
}

void CTreeElementTexte::DrawElement(wxDC* deviceContext, const int& x, const int& y)
{
	if (deviceContext == nullptr)
		return;

	// Sécurité si la taille n'a pas encore été initialisée
	if (textSize.x <= 0 || textSize.y <= 0)
		textSize = GetSizeText();

	int xPos = 0;
	int yPos = y + (themeTexte.GetHeight() - textSize.y) / 2;
	switch (position)
	{
	case RENDERFONT_LEFT:
		xPos = x;
		break;

	case RENDERFONT_CENTER:
		xPos = x + (themeTexte.GetWidth() - textSize.x) / 2;
		break;

	case RENDERFONT_RIGHT:
		xPos = x + themeTexte.GetWidth() - textSize.x;
		break;
	default:;
	}

	CWindowUtility::DrawTexte(deviceContext, libelle, xPos, yPos, themeTexte.font);
}

void CTreeElementTexte::SetClick(const bool& value)
{
	isClick = value;
}
