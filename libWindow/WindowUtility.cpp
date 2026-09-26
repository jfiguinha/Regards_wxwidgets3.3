#include <header.h>
#include "WindowUtility.h"
#include <wx/fontenum.h>

using namespace Regards::Window;

// Méthode d'aide privée/interne pour factoriser la création de la police
wxFont CWindowUtility::CreateFont(CThemeFont& font)
{
	int fontSize = font.GetFontSize();
	bool isBold = font.GetBold();
	bool isItalic = (font.GetItalic() == 1);
	wxString fontName = font.GetFontName();

	wxFontWeight weight = isBold ? wxFONTWEIGHT_BOLD : wxFONTWEIGHT_NORMAL;
	wxFontStyle style = isItalic ? wxFONTSTYLE_ITALIC : wxFONTSTYLE_NORMAL;

	// Vérification de l'existence de la police sur le système
	if (wxFontEnumerator::IsValidFacename(fontName))
	{
		return wxFont(fontSize, wxFONTFAMILY_DEFAULT, style, weight, false, fontName);
	}
	else
	{
		return wxFont(fontSize, wxFONTFAMILY_SWISS, style, weight, false, wxEmptyString);
	}
}

bool CWindowUtility::FontExists(const wxString& fontName)
{
	return wxFontEnumerator::IsValidFacename(fontName);
}

void CWindowUtility::FillRect(wxDC* dc, const wxRect& rc, const wxColour& color)
{
	wxBrush brush(color, wxBRUSHSTYLE_SOLID);
	dc->SetBrush(brush);
	dc->SetPen(wxPen(color, 1));
	dc->DrawRectangle(rc);
	dc->SetPen(wxNullPen);
	dc->SetBrush(wxNullBrush);
}

void CWindowUtility::DrawTexte(wxDC* dc, const wxString& libelle, const int& xPos, const int& yPos, CThemeFont font)
{
	wxColour color = font.GetColorFont();
	wxFont _font = CreateFont(font); // Utilisation de la méthode factorisée

	dc->SetFont(_font);
	dc->SetTextForeground(color);
	dc->DrawText(libelle, xPos, yPos);
	dc->SetFont(wxNullFont);
}

wxSize CWindowUtility::GetSizeTexte(wxDC* dc, const wxString& libelle, CThemeFont font)
{
	wxSize size;
	wxMemoryDC temp_dc(dc);
	wxFont _font = CreateFont(font); // Utilisation de la méthode factorisée

	temp_dc.SetFont(_font);
	size = temp_dc.GetTextExtent(libelle);
	temp_dc.SetFont(wxNullFont);
	return size;
}
