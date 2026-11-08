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
	insetLeft = other.insetLeft;
	insetRight = other.insetRight;
	ellipsize = other.ellipsize;
	hasRowStyle = other.hasRowStyle;
	rowBackground = other.rowBackground;
	rowBorder = other.rowBorder;
	cacheWidth = -1; // le cache est recalcule a la demande
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
	cacheWidth = -1;

	// OPTIMISATION : On calcule la taille UNE SEULE FOIS ici, pas pendant le dessin
	textSize = GetSizeText();

	if (themeTexte.GetWidth() < textSize.x)
		themeTexte.SetWidth(textSize.x);

	if (themeTexte.GetHeight() < textSize.y)
		themeTexte.SetHeight(textSize.y);
}

wxSize CTreeElementTexte::GetSizeText()
{
	return MeasureText(libelle);
}

wxSize CTreeElementTexte::MeasureText(const wxString& text)
{
	if (text.IsEmpty())
		return wxSize(0, 0);

	// SOLUTION POUR LA TAILLE CORRECTE :
	// Au lieu de recréer une bitmap de 250x250 à chaque appel (ce qui consommait de la mémoire),
	// on utilise une bitmap factice de 1x1 pixel. Pour mesurer du texte, la taille de la bitmap n'importe pas,
	// seul le fait d'avoir un contexte de mémoire valide avec la bonne police compte.
	static wxBitmap dummyBitmap(1, 1);
	wxMemoryDC dc(dummyBitmap);

	return CWindowUtility::GetSizeTexte(&dc, text, themeTexte.font);
}

void CTreeElementTexte::SetTextColor(const wxColour& color)
{
	themeTexte.font.SetColorFont(color);
}

void CTreeElementTexte::SetBold(const bool& bold)
{
	const int value = bold ? 1 : 0;
	if (themeTexte.font.GetBold() == value)
		return;

	themeTexte.font.SetBold(value);
	cacheWidth = -1;
	textSize = GetSizeText(); // la largeur change avec la graisse
}

void CTreeElementTexte::SetInsets(const int& left, const int& right)
{
	insetLeft = left;
	insetRight = right;
	cacheWidth = -1;
}

void CTreeElementTexte::SetEllipsize(const bool& value)
{
	ellipsize = value;
	cacheWidth = -1;
}

void CTreeElementTexte::SetRowStyle(const wxColour& background, const wxColour& border)
{
	hasRowStyle = true;
	rowBackground = background;
	rowBorder = border;
}

// Retourne le texte a afficher (tronque par "..." si necessaire) et sa taille
wxString CTreeElementTexte::GetDisplayText(const int& maxWidth, wxSize& displaySize)
{
	if (maxWidth <= 0 || textSize.x <= maxWidth)
	{
		displaySize = textSize;
		return libelle;
	}

	if (cacheWidth == maxWidth)
	{
		displaySize = cacheSize;
		return cacheText;
	}

	const wxString ellipsis = L"\u2026";
	size_t low = 0;
	size_t high = libelle.length();

	// Recherche dichotomique du plus long prefixe qui tient avec "..."
	while (low < high)
	{
		const size_t mid = (low + high + 1) / 2;
		if (MeasureText(libelle.Left(mid) + ellipsis).x <= maxWidth)
			low = mid;
		else
			high = mid - 1;
	}

	cacheText = libelle.Left(low) + ellipsis;
	cacheSize = MeasureText(cacheText);
	cacheWidth = maxWidth;
	displaySize = cacheSize;
	return cacheText;
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

	const int zoneWidth = themeTexte.GetWidth();
	const int zoneHeight = themeTexte.GetHeight();

	// Fond uni + filet de 1 px en bas de la ligne (style moderne)
	if (hasRowStyle)
	{
		deviceContext->SetPen(*wxTRANSPARENT_PEN);
		deviceContext->SetBrush(wxBrush(rowBackground));
		deviceContext->DrawRectangle(x, y, zoneWidth, zoneHeight);

		deviceContext->SetPen(wxPen(rowBorder, 1));
		deviceContext->DrawLine(x, y + zoneHeight - 1, x + zoneWidth, y + zoneHeight - 1);
		deviceContext->SetPen(wxNullPen);
		deviceContext->SetBrush(wxNullBrush);
	}

	const int availableWidth = zoneWidth - insetLeft - insetRight;

	wxSize displaySize = textSize;
	wxString displayText = libelle;
	if (ellipsize)
		displayText = GetDisplayText(availableWidth, displaySize);

	int xPos = 0;
	int yPos = y + (zoneHeight - displaySize.y) / 2;
	switch (position)
	{
	case RENDERFONT_LEFT:
		xPos = x + insetLeft;
		break;

	case RENDERFONT_CENTER:
		xPos = x + insetLeft + (availableWidth - displaySize.x) / 2;
		break;

	case RENDERFONT_RIGHT:
		xPos = x + zoneWidth - insetRight - displaySize.x;
		break;
	default:;
	}

	CWindowUtility::DrawTexte(deviceContext, displayText, xPos, yPos, themeTexte.font);
}

void CTreeElementTexte::SetClick(const bool& value)
{
	isClick = value;
}
