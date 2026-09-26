#include "header.h"
#include "InfosSeparationBar.h"
#include <ConvertUtility.h>
#include "WindowMain.h"
using namespace Regards::Window;
//const unsigned int cuStackSize = 128 * 1024;

#define WM_PICTURELOAD 1


void CInfosSeparationBar::OnClick(const int& x, const int& y)
{
}

int CInfosSeparationBar::GetXPos()
{
	return _xPos;
}

int CInfosSeparationBar::GetYPos()
{
	return _yPos;
}

const int& CInfosSeparationBar::GetWidth()
{
	return width;
}

const int& CInfosSeparationBar::GetHeight()
{
	return theme.GetHeight();
}

void CInfosSeparationBar::Clear()
{
	listElement.clear();
	listElement.reserve(0);
}

void CInfosSeparationBar::SetTitle(const wxString& title)
{
	this->title = title;
}

bool CInfosSeparationBar::operator ==(const CInfosSeparationBar& n2)
{
	int left = _xPos;
	int right = _xPos + width;
	int top = _yPos;
	int bottom = _yPos + GetHeight();

	if ((left < n2._xPos && n2._xPos < right) && (top < n2._yPos && n2._yPos < bottom))
	{
		return true;
	}
	return false;
}

void CInfosSeparationBar::SetWindowPos(const int& x, const int& y)
{
	this->_xPos = x;
	this->_yPos = y;
}

wxRect CInfosSeparationBar::GetPos()
{
	wxRect rc;
	rc.x = _xPos;
	rc.width = width;
	rc.y = _yPos;
	rc.height = theme.GetHeight();
	return rc;
}

void CInfosSeparationBar::SetWidth(const int& width)
{
	if (width != this->width)
	{
		this->width = width;
	}
}


CInfosSeparationBar::CInfosSeparationBar(const CThemeInfosSeparationBar& theme): _xPos(0), _yPos(0), width(0)
{
	this->theme = theme;
}


CInfosSeparationBar::~CInfosSeparationBar(void)
{
	listElement.clear();
	listElement.reserve(0);
}
void CInfosSeparationBar::RenderTitle(wxDC* dc)
{
	wxRect rc;
	int x = 0;
	int y = 0;
	rc.x = 0;
	rc.y = 0;
	rc.width = width;
	rc.height = GetHeight();
	CWindowMain::FillRect(dc, rc, theme.colorBack);

	int posY = 0; // Va cumuler la hauteur au fur et à mesure
	int spacingBetweenLines = 2; // Petit espace en pixels entre les lignes (optionnel)

	vector<wxString> listOfTexte = CConvertUtility::split(this->title, '@');

	if (!listOfTexte.empty())
	{
		int minX = width;
		int minY = GetHeight();
		int maxX = 0;
		int maxY = 0;
		bool hasDrawnAnything = false;

		for (const wxString& textLine : listOfTexte)
		{
			if (!textLine.IsEmpty())
			{
				wxSize size = CWindowMain::GetSizeTexte(dc, textLine, theme.themeFont);

				// Aligné tout en haut : posY commence à 0 et augmente de la hauteur du texte écrit
				int localy = y + posY;
				int localx = x + (width - size.x) / 2; // Reste centré horizontalement

				CWindowMain::DrawTexte(dc, textLine, localx, localy, theme.themeFont);

				// Mise à jour des bornes pour titleRectPos
				if (localx < minX) minX = localx;
				if (localy < minY) minY = localy;
				if (localx + size.x > maxX) maxX = localx + size.x;
				if (localy + size.y > maxY) maxY = localy + size.y;
				hasDrawnAnything = true;

				// On avance posY de la hauteur exacte de la ligne de texte + l'espacement
				posY += size.y + spacingBetweenLines;
			}
		}

		if (hasDrawnAnything)
		{
			titleRectPos.x = minX;
			titleRectPos.y = minY;
			titleRectPos.width = maxX - minX;
			titleRectPos.height = maxY - minY;
		}
		else
		{
			titleRectPos = wxRect(0, 0, 0, 0);
		}
	}
}

void  CInfosSeparationBar::SetNbElementX(const int& nbElementX)
{
	this->nbElementX = nbElementX;
}

void CInfosSeparationBar::SetNbElementY(const int& nbElementY)
{
	this->nbElementY = nbElementY;
}

int  CInfosSeparationBar::GetNbElementX()
{
	return nbElementX;
}

int  CInfosSeparationBar::GetNbElementY()
{
	return nbElementY;
}

void CInfosSeparationBar::Render(wxDC* dc, const int& posLargeur, const int& posHauteur)
{
	wxBitmap memBitmap(GetWidth(), GetHeight());
	wxMemoryDC memDC(memBitmap);
	RenderIcone(&memDC, posLargeur, posHauteur);
	memDC.SelectObject(wxNullBitmap);
	dc->DrawBitmap(memBitmap, _xPos + posLargeur, _yPos + posHauteur);
}

//----------------------------------------------------------------------------------
//
//----------------------------------------------------------------------------------
void CInfosSeparationBar::RenderIcone(wxDC* dc, const int& posLargeur, const int& posHauteur)
{
	RenderTitle(dc);
}
