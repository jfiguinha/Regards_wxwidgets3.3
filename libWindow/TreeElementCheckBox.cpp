#include "header.h"
#include "TreeElementCheckBox.h"
#include <LibResource.h>
using namespace Regards::Window;

CTreeElementCheckBox::CTreeElementCheckBox()
{
	checked = false;
}

CTreeElementCheckBox& CTreeElementCheckBox::operator=(const CTreeElementCheckBox& other)
{
	visible = other.visible;
	xPos = other.xPos;
	yPos = other.yPos;
	numRow = other.numRow;
	numColumn = other.numColumn;
	checked = other.checked;
	themeTreeCheckBox = other.themeTreeCheckBox;
	return *this;
}


void CTreeElementCheckBox::SetTheme(CThemeTreeCheckBox* theme)
{
	themeTreeCheckBox = *theme;
}

void CTreeElementCheckBox::ClickElement(wxWindow* window, const int& x, const int& y)
{
	checked = !checked;
}

bool CTreeElementCheckBox::GetCheckState()
{
	return checked;
}

void CTreeElementCheckBox::DrawElement(wxDC* deviceContext, const int& x, const int& y)
{
	DrawBitmap(deviceContext, x, y);
}

void CTreeElementCheckBox::SetCheckState(const bool& check)
{
	checked = check;
}

void CTreeElementCheckBox::InitBitmaps()
{
	int checkWidth = themeTreeCheckBox.GetCheckBoxWidth();
	int checkHeight = themeTreeCheckBox.GetCheckBoxHeight();

	// On ne charge les ressources SVG QUE si elles ne sont pas encore prêtes
	if (!checkOn.IsOk() || checkOn.GetWidth() != checkWidth)
	{
		checkOn = wxBitmap(CLibResource::CreatePictureFromSVG("IDB_CHECKBOX_ON", checkWidth, checkHeight));
		checkOff = wxBitmap(CLibResource::CreatePictureFromSVG("IDB_CHECKBOX_OFF", checkWidth, checkHeight));
	}
}

void CTreeElementCheckBox::DrawBitmap(wxDC* deviceContext, const int& xPos, const int& yPos)
{
	InitBitmaps(); // Chargement unique au premier affichage

	const wxBitmap& bitmapToDraw = checked ? checkOn : checkOff;

	int y = yPos + (themeTreeCheckBox.GetHeight() - bitmapToDraw.GetHeight()) / 2;
	int x = xPos + (themeTreeCheckBox.GetWidth() - bitmapToDraw.GetWidth()) / 2;

	// Utilisation directe d'une wxBitmap (Ultra rapide)
	deviceContext->DrawBitmap(bitmapToDraw, x, y);
}