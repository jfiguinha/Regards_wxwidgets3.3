#include "header.h"
#include "TreeElementDelete.h"
#include "WindowMain.h"
using namespace Regards::Window;

CTreeElementDelete::CTreeElementDelete()
{
	visible = true;
}

CTreeElementDelete& CTreeElementDelete::operator=(const CTreeElementDelete& other)
{
	visible = other.visible;
	xPos = other.xPos;
	yPos = other.yPos;
	numRow = other.numRow;
	numColumn = other.numColumn;
	themeTreeDelete = other.themeTreeDelete;
	return *this;
}

void CTreeElementDelete::GenerateCrossBitmap()
{
	int w = themeTreeDelete.GetCroixWidth();
	int h = themeTreeDelete.GetCroixHeight();

	if (w <= 0 || h <= 0)
		return;

	wxRect rcBitmap(0, 0, w, h);

	m_croixOff = wxBitmap(w, h, 32);
	wxMemoryDC memorydc(m_croixOff);

	// Utilisation directe de la brosse transparente et d'un pen optimisé
	CWindowMain::FillRect(&memorydc, rcBitmap, themeTreeDelete.color);

	memorydc.SetPen(wxPen(themeTreeDelete.crossColor, 2));
	memorydc.DrawLine(3, 3, w - 4, h - 4);
	memorydc.DrawLine(w - 4, 3, 3, h - 4);

	memorydc.SetPen(wxNullPen);
	memorydc.SelectObject(wxNullBitmap);
}

void CTreeElementDelete::SetTheme(CThemeTreeDelete* theme)
{
	themeTreeDelete = *theme;
	GenerateCrossBitmap();

}


void CTreeElementDelete::DrawElement(wxDC* deviceContext, const int& x, const int& y)
{
	DrawBitmap(deviceContext, x, y);
}

void CTreeElementDelete::DrawBitmap(wxDC* deviceContext, const int& xPos, const int& yPos)
{
	if(!m_croixOff.IsOk())
		GenerateCrossBitmap();

	int y = yPos + (themeTreeDelete.GetHeight() - m_croixOff.GetHeight()) / 2;
	//int x = xPos + (themeTreeDelete.width - m_croixOff.GetWidth()) / 2;
	deviceContext->DrawBitmap(m_croixOff, xPos, y);
}
