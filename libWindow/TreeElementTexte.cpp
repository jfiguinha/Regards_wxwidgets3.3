#include "header.h"
#include "TreeElementTexte.h"
using namespace Regards::Window;


CTreeElementTexte::CTreeElementTexte()
	: canUpdate(false),
	position(RENDERFONT_LEFT),
	textSizeValid(false)
{}

CTreeElementTexte& CTreeElementTexte::operator=(const CTreeElementTexte& other)
{
	visible = other.visible;
	xPos = other.xPos;
	yPos = other.yPos;
	numRow = other.numRow;
	textSizeValid = other.textSizeValid;
	numColumn = other.numColumn;
	themeTexte = other.themeTexte;
	canUpdate = other.canUpdate;
	libelle = other.libelle;
	position = other.position;
	return *this;
}


void CTreeElementTexte::SetTheme(CThemeTreeTexte* theme)
{
	themeTexte = *theme;
	textSizeValid = false;
	isFontOk = false;
	GenerateFont();
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
	textSizeValid = false;
	wxSize size = GetSizeText();
	if (themeTexte.GetWidth() < size.x)
		themeTexte.SetWidth(size.x);

	if (themeTexte.GetHeight() < size.y)
		themeTexte.SetHeight(size.y);
}

void CTreeElementTexte::GenerateFont()
{
	if (!isFontOk)
	{
		font = wxFont(themeTexte.font.GetFontSize(), wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL);
		isFontOk = true;
	}
}

wxSize CTreeElementTexte::GetSizeText()
{
	if (textSizeValid)
		return cachedTextSize;

	wxBitmap bitmap(1, 1);
	wxMemoryDC dc(bitmap);

	dc.SetFont(font);
	cachedTextSize = dc.GetTextExtent(libelle);

	dc.SelectObject(wxNullBitmap);

	textSizeValid = true;
	return cachedTextSize;
}

void CTreeElementTexte::DrawText(wxDC* dc, const int& xPos, const int& yPos)
{
	dc->SetFont(font);
	dc->SetTextForeground(themeTexte.font.GetColorFont());
	dc->DrawText(libelle, xPos, yPos);
}

void CTreeElementTexte::SetPosition(const int& position)
{
	this->position = position;
}

void CTreeElementTexte::DrawElement(wxDC* deviceContext, const int& x, const int& y)
{
	wxSize size = GetSizeText();


	int xPos = 0;
	int yPos = y + (themeTexte.GetHeight() - size.y) / 2;
	switch (position)
	{
	case RENDERFONT_LEFT:
		xPos = x;
		break;

	case RENDERFONT_CENTER:
		xPos = x + (themeTexte.GetWidth() - size.x) / 2;
		break;

	case RENDERFONT_RIGHT:
		xPos = x + themeTexte.GetWidth() - size.x;
		break;
	default: ;
	}


	DrawText(deviceContext, xPos, yPos);
}

