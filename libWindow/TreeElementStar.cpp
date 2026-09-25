#include "header.h"
#include "TreeElementStar.h"
#include "LibResource.h"
#include <SqlCriteria.h>
#include <SqlPhotoCriteria.h>
#include <WindowMain.h>
using namespace Regards::Sqlite;
using namespace Regards::Window;

CTreeElementStar::CTreeElementStar()
{
	value = 0;
}

void CTreeElementStar::CreateStar()
{
	int targetWidth = themeTriangle.GetWidth() * 2;
	int targetHeight = themeTriangle.GetHeight() * 2;

	// On ne régénère que si la taille a changé
	if (!starEmpty.IsOk() || starEmpty.GetWidth() != targetWidth)
	{
		starEmpty = wxBitmap(CLibResource::CreatePictureFromSVG(L"IDB_STAREMPTY", targetWidth, targetHeight));
		starYellow = wxBitmap(CLibResource::CreatePictureFromSVG(L"IDB_STARYELLOW", targetWidth, targetHeight));
	}
}
void CTreeElementStar::SetNumPhoto(const int& numPhotoId)
{
	this->numPhotoId = numPhotoId;
}

void CTreeElementStar::SetValue(const int& value)
{
	this->value = value;
}

int CTreeElementStar::GetValue()
{
	return value;
}

void CTreeElementStar::DrawElement(wxDC* deviceContext, const int& x, const int& y)
{
	localx = x;
	localy = y;
	DrawStar(deviceContext, x, y);
}


void CTreeElementStar::SetTheme(CThemeTreeTriangle* theme)
{
	themeTriangle = *theme;
	CreateStar(); // On génère les icônes ici, car les dimensions valides sont connues !
}


void CTreeElementStar::DrawStar(wxDC* dc, const int& x, const int& y)
{
	int xPos = x;
	int starW = starEmpty.GetWidth();

	for (int i = 0; i < 5; i++)
	{
		// Dessin instantané grâce aux structures native VRAM (wxBitmap)
		if (value > i)
			dc->DrawBitmap(starYellow, xPos, y);
		else
			dc->DrawBitmap(starEmpty, xPos, y);

		xPos += starW;
	}
}


void CTreeElementStar::ClickElement(wxWindow* window, const int& x, const int& y)
{
	CSqlPhotoCriteria sqlPhotoCriteria;
	CSqlCriteria sqlCriteria;
	bool isNew = false;
	if (value > 0 && value <= 5)
	{
		wxString libelle = to_string(value) + " Star";
		int oldCriteria = sqlCriteria.GetOrInsertCriteriaId(1, 6, libelle, isNew);
		sqlPhotoCriteria.DeletePhotoCriteria(numPhotoId, oldCriteria);
	}


	int maxX = 5 * starYellow.GetWidth() + localx;
	//int maxY = localy + starYellow.GetHeight();
	if (x > localx && x < maxX)
	{
		value = (x - localx) / starYellow.GetWidth();
		if ((x - localx) < starYellow.GetWidth() / 2)
			value = 0;
		else
			value++;
	}

	if (value >= 0 && value <= 5)
	{
		wxString libelle = to_string(value) + " Star";
		int newCriteria = sqlCriteria.GetOrInsertCriteriaId(1, 6, libelle, isNew);
		sqlPhotoCriteria.InsertPhotoCriteria(numPhotoId, newCriteria);
	}
}
