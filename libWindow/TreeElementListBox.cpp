#include "header.h"
#include "TreeElementListBox.h"
#include <LibResource.h>
using namespace Regards::Window;

CTreeElementListBox::CTreeElementListBox(
	CTreeElementSlideInterface* eventInterface)
	: eventInterface(eventInterface),
	position(0)
{

}

wxBitmap CTreeElementListBox::CreateTriangle(const int& width, const int& height, const wxColor& color,
                                             const wxColor& colorBack)
{
	auto bitmapBuffer = wxBitmap(width, height);
	wxMemoryDC memDC(bitmapBuffer);
	wxBrush brushHatch(color);

	wxRect rc;
	rc.x = 0;
	rc.width = width;
	rc.y = 0;
	rc.height = height;
	CWindowMain::FillRect(&memDC, rc, colorBack);

	memDC.SetBrush(brushHatch);
	wxPoint star[3];
	star[0] = wxPoint(0, 0);
	star[1] = wxPoint(width, 0);
	star[2] = wxPoint(width / 2, height);
	memDC.DrawPolygon(WXSIZEOF(star), star, 0, 0);
	memDC.SetBrush(wxNullBrush);
	memDC.SelectObject(wxNullBitmap);
	return bitmapBuffer;
}

CTreeElementListBox& CTreeElementListBox::operator=(const CTreeElementListBox& other)
{
	visible = other.visible;
	xPos = other.xPos;
	yPos = other.yPos;
	numRow = other.numRow;
	numColumn = other.numColumn;
	themeTreeListBox = other.themeTreeListBox;
	position = other.position;
	tabValue = other.tabValue;
	exifKey = other.exifKey;
	eventInterface = other.eventInterface;
	return *this;
}


void CTreeElementListBox::SetElementPos(const int& x, const int& y)
{
	xPos = x;
	yPos = y;
}

wxString CTreeElementListBox::GetPositionValue()
{
	if (position >= 0 &&
		position < static_cast<int>(tabValue.size()))
	{
		return tabValue[position].value;
	}

	return {};
}

void CTreeElementListBox::SetTabValue(const vector<CMetadata>& value, const int& index)
{
	tabValue = value;
	position = index;
	changeValue = true;
}

void CTreeElementListBox::SetExifKey(const wxString& exifKey)
{
	this->exifKey = exifKey;
}

void CTreeElementListBox::SetTheme(CThemeTreeListBox* theme)
{
	themeTreeListBox = *theme;
}

void CTreeElementListBox::TestMaxMinValue()
{
	if (tabValue.empty())
	{
		position = 0;
		return;
	}

	if (position >= static_cast<int>(tabValue.size()))
		position = static_cast<int>(tabValue.size()) - 1;

	if (position < 0)
		position = 0;

	
}

void CTreeElementListBox::ClickElement(wxWindow* window, const int& x, const int& y)
{
	if (x >= moinsPos.x && x < (moinsPos.width + moinsPos.x))
	{
		changeValue = true;
		position--;
		TestMaxMinValue();
		CMetadata data = tabValue[position];
		CTreeElementValueInt elementValue(data.depth);
		eventInterface->SlidePosChange(this, position, &elementValue, exifKey);
	}
	else if (x >= plusPos.x && x < (plusPos.width + plusPos.x))
	{
		changeValue = true;
		position++;
		TestMaxMinValue();
		CMetadata data = tabValue[position];
		CTreeElementValueInt elementValue(data.depth);
		eventInterface->SlidePosChange(this, position, &elementValue, exifKey);
	}
}

void CTreeElementListBox::GenerateBitmap(wxDC* deviceContext)
{
	if (!bitmapBuffer.IsOk() || bitmapBuffer.GetWidth() != themeTreeListBox.GetWidth() || bitmapBuffer.GetHeight() != themeTreeListBox.GetHeight())
	{
		bitmapBuffer = wxBitmap(themeTreeListBox.GetWidth(), themeTreeListBox.GetHeight());
	}

	wxMemoryDC memDC(bitmapBuffer);

	wxRect rc(0, 0, themeTreeListBox.GetWidth(), themeTreeListBox.GetHeight());
	CWindowMain::FillRect(&memDC, rc, themeTreeListBox.color);

	wxSize renderElement = CWindowMain::GetSizeTexte(deviceContext, GetPositionValue(), themeTreeListBox.font);
	int yMedium = (themeTreeListBox.GetHeight() - renderElement.y) / 2;
	CWindowMain::DrawTexte(&memDC, GetPositionValue(), 0, yMedium, themeTreeListBox.font);

	int btnW = themeTreeListBox.GetButtonWidth();
	int btnH = themeTreeListBox.GetButtonHeight();

	// OPTIMISATION : Chargement unique du SVG ET conversion asynchrone unique en version désactivée
	if (!buttonMoins.IsOk() || buttonMoins.GetWidth() != btnW)
	{
		wxImage imgMoins = CLibResource::CreatePictureFromSVG("IDB_MINUS", btnW, btnH);
		wxImage imgPlus = CLibResource::CreatePictureFromSVG("IDB_PLUS", btnW, btnH);

		buttonMoins = wxBitmap(imgMoins);
		buttonPlus = wxBitmap(imgPlus);

		// Conversion et mise en cache immédiate
		buttonMoinsDisabled = wxBitmap(imgMoins.ConvertToDisabled());
		buttonPlusDisabled = wxBitmap(imgPlus.ConvertToDisabled());
	}

	moinsPos.x = themeTreeListBox.GetWidth() - (buttonMoins.GetWidth() + themeTreeListBox.GetMarge() + buttonPlus.GetWidth() + themeTreeListBox.GetMarge());
	moinsPos.y = (themeTreeListBox.GetHeight() - btnH) / 2;
	moinsPos.width = btnW;
	moinsPos.height = btnH;

	// ACCÈS DIRECT : Plus aucune allocation ni conversion pixel par pixel ici
	memDC.DrawBitmap(buttonMoinsDisabled, moinsPos.x, moinsPos.y);

	plusPos.x = moinsPos.x + buttonMoins.GetWidth() + 2 * themeTreeListBox.GetMarge();
	plusPos.y = moinsPos.y;
	plusPos.width = btnW;
	plusPos.height = btnH;
	memDC.DrawBitmap(buttonPlusDisabled, plusPos.x, plusPos.y);

	memDC.SelectObject(wxNullBitmap);
}




void CTreeElementListBox::DrawElement(wxDC* deviceContext, const int& x, const int& y)
{
	if (changeValue || !bitmapBuffer.IsOk() || bitmapBuffer.GetWidth() != themeTreeListBox.GetWidth() || bitmapBuffer.GetHeight() != themeTreeListBox.GetHeight())
	{
		GenerateBitmap(deviceContext);
		if(changeValue)
			changeValue = false;
	}
	deviceContext->DrawBitmap(bitmapBuffer, x, y);
}
