#include "header.h"
#include "InfosSeparationBar.h"
#include <ConvertUtility.h>
#include "WindowMain.h"
#include <wx/graphics.h>
#include <wx/math.h>
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


CInfosSeparationBar::CInfosSeparationBar(const CThemeInfosSeparationBar& theme, float dpiScale): _xPos(0), _yPos(0), width(0), dpiScale(dpiScale)
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
    if (dc == nullptr || width <= 0 || GetHeight() <= 0)
    {
        titleRectPos = wxRect(0, 0, 0, 0);
        return;
    }

    const int height = GetHeight();

    // Fond de l'en-tête.
    const wxRect clientRect(0, 0, width, height);
    CWindowMain::FillRect(dc, clientRect, theme.colorBack);

    titleRectPos = wxRect(0, 0, 0, 0);

    const vector<wxString> lines =
        CConvertUtility::split(title, '@');

    if (lines.empty())
        return;

    // Marges adaptées au DPI.
    const int horizontalPadding = static_cast<int>(8 * dpiScale);
    const int verticalPadding = static_cast<int>(5 * dpiScale);
    const int spacingBetweenLines = static_cast<int>(3 * dpiScale);



    vector<TextLine> validLines;
    validLines.reserve(lines.size());

    int totalTextHeight = 0;
    int maxTextWidth = 0;

    // Première passe : calcul des dimensions.
    for (const wxString& line : lines)
    {
        if (line.IsEmpty())
            continue;

        const wxSize textSize =
            CWindowMain::GetSizeTexte(dc, line, theme.themeFont);

        if (textSize.x <= 0 || textSize.y <= 0)
            continue;

        validLines.push_back({ line, textSize });

        totalTextHeight += textSize.y;
        maxTextWidth = std::max(maxTextWidth, textSize.x);
    }

    if (validLines.empty())
        return;

    totalTextHeight +=
        static_cast<int>(validLines.size() - 1) * spacingBetweenLines;

    // Centrage vertical de l'ensemble du titre.
    int posY = std::max(
        verticalPadding,
        (height - totalTextHeight) / 2);

    // Deuxième passe : dessin des lignes.
    int minX = width;
    int minY = height;
    int maxX = 0;
    int maxY = 0;

    for (const TextLine& line : validLines)
    {
        const int posX = (width - line.size.x) / 2;

        CWindowMain::DrawTexte(
            dc,
            line.text,
            posX,
            posY,
            theme.themeFont);

        minX = std::min(minX, posX);
        minY = std::min(minY, posY);

        maxX = std::max(maxX, posX + line.size.x);
        maxY = std::max(maxY, posY + line.size.y);

        posY += line.size.y + spacingBetweenLines;
    }

    // Rectangle englobant du titre.
    titleRectPos = wxRect(
        minX,
        minY,
        maxX - minX,
        maxY - minY);


    // Dessin de la barre de séparation
    int y = GetHeight() - 1;

    dc->SetPen(wxPen(theme.themeFont.colorBack, 1));
    dc->DrawLine(5, y, width - 5, y);

    //RenderBorder(dc);
}


void CInfosSeparationBar::RenderBorder(wxDC* dc)
{
    if (!dc)
        return;

    const int width = GetWidth();
    const int height = GetHeight();
    const int radius = 8;

    dc->SetBrush(*wxTRANSPARENT_BRUSH);
    dc->SetPen(wxPen(theme.themeFont.colorBack, 1));

    // Bord supérieur
    dc->DrawLine(radius, 0, width - radius, 0);

    // Bord droit
    dc->DrawLine(width - 1, radius, width - 1, height - 1);

    // Bord gauche
    dc->DrawLine(0, radius, 0, height - 1);

    // Bordure arrondie supérieure gauche
    dc->DrawArc(
        0, radius * 2,
        radius * 2, 0,
        radius, radius);

    // Bordure arrondie supérieure droite
    dc->DrawArc(
        width - radius * 2, 0,
        width, radius * 2,
        width - radius, radius);
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
    if (dc == nullptr)
        return;



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
