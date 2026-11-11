#include "header.h"
#include <ClosedHandCursor.h>
#include <wx/graphics.h>
#include <ConvertUtility.h>
#include "SliderInterface.h"
#include "ToolbarSliderVideo.h"
#include "WindowMain.h"

using namespace Regards::Window;

namespace
{
	// Géométrie et palette identiques à CSliderVideo
	constexpr int SLIDER_TRACK_HEIGHT = 4;
	constexpr int SLIDER_THUMB_RADIUS = 7;
	constexpr int SLIDER_TRACK_MARGIN = 9;
	constexpr int SLIDER_TEXT_MARGIN = 6;

	wxColour GetRailInactiveColour() { return wxColour(74, 77, 94); }
	wxColour GetRailActiveColour() { return wxColour(91, 157, 245); }

	wxColour GetThumbFillColour(const wxColour& back)
	{
		const int luminance = (back.Red() * 299 + back.Green() * 587 + back.Blue() * 114) / 1000;
		return luminance < 128 ? wxColour(235, 238, 242) : wxColour(255, 255, 255);
	}
}

// ============================================================
// Constructeur
// ============================================================

CToolbarSliderVideo::CToolbarSliderVideo(const CThemeSlider& themeSlider, CSliderInterface* eventInterface)
{
	x = 0;
	y = 0;
	colorBackground = false;
	position = 0;

	this->eventInterface = eventInterface;
	this->themeSlider = themeSlider;

	positionButton = wxRect(0, 0, 0, 0);
	hCursorHand = CResourceCursor::GetClosedHand();

	mouseBlock = false;
	captureBall = false;
}

// ============================================================
// Dimensions & Libellés
// ============================================================

void CToolbarSliderVideo::Resize(const int& tailleX, const int& tailleY)
{
	themeSlider.SetWidth(tailleX);
	themeSlider.SetHeight(tailleY);
}

int CToolbarSliderVideo::GetWidth() { return themeSlider.GetWidth(); }
int CToolbarSliderVideo::GetHeight() { return themeSlider.GetHeight(); }

void CToolbarSliderVideo::SetLeftLabel(const wxString& label)
{
	leftLabel = label;
	useCustomLeftLabel = true;
}

void CToolbarSliderVideo::SetRightLabel(const wxString& label)
{
	rightLabel = label;
	useCustomRightLabel = true;
}

void CToolbarSliderVideo::SetLabels(const wxString& left, const wxString& right)
{
	SetLeftLabel(left);
	SetRightLabel(right);
}

// ============================================================
// Gestion de la position
// ============================================================

void CToolbarSliderVideo::ZoomPos(const int& position)
{
	if (eventInterface != nullptr)
		eventInterface->ZoomPos(position);
}

void CToolbarSliderVideo::SetPosition(const int& iPos)
{
	if (tabValue.empty())
	{
		position = 0;
		return;
	}
	position = std::clamp(iPos, 0, static_cast<int>(tabValue.size()) - 1);
}

int CToolbarSliderVideo::GetNbValue() { return static_cast<int>(tabValue.size()); }
int CToolbarSliderVideo::GetPosition() { return position; }

void CToolbarSliderVideo::SetTabValue(std::vector<int> value)
{
	tabValue = std::move(value);
	if (tabValue.empty())
		position = 0;
	else
		position = std::clamp(position, 0, static_cast<int>(tabValue.size()) - 1);
}

int CToolbarSliderVideo::GetPositionValue()
{
	if (position >= 0 && position < static_cast<int>(tabValue.size()))
		return tabValue[position];
	return -1;
}

int CToolbarSliderVideo::GetLastValue()
{
	return !tabValue.empty() ? tabValue.back() : -1;
}

int CToolbarSliderVideo::GetFirstValue()
{
	return !tabValue.empty() ? tabValue.front() : -1;
}

void CToolbarSliderVideo::SetBackgroundBitmap(const wxBitmap& background)
{
	colorBackground = false;
	this->background = background;
}

double CToolbarSliderVideo::GetPositionRatio() const
{
	if (videoTimeMode)
	{
		if (totalDurationMs <= 0) return 0.0;
		return std::clamp(static_cast<double>(currentTimeMs) / static_cast<double>(totalDurationMs), 0.0, 1.0);
	}


	if (tabValue.size() < 2 || position <= 0)
		return 0.0;

	return std::clamp(static_cast<double>(position) / static_cast<double>(tabValue.size() - 1), 0.0, 1.0);
}

// ============================================================
// Calculs géométriques et souris
// ============================================================

void CToolbarSliderVideo::CalculZoomPosition(const int& localRailX)
{
	if (videoTimeMode)
	{
		if (totalDurationMs <= 0 || positionSlider.width <= 0) return;
		const double posX = static_cast<double>(localRailX - positionSlider.x);
		const double ratio = std::clamp(posX / static_cast<double>(positionSlider.width), 0.0, 1.0);
		currentTimeMs = static_cast<int64_t>(ratio * static_cast<double>(totalDurationMs));
		SetLeftLabel(CConvertUtility::GetTimeLibelle(currentTimeMs / 1000));
		return;
	}


	if (tabValue.size() < 2 || positionSlider.width <= 0)
	{
		position = 0;
		return;
	}

	const double posX = static_cast<double>(localRailX - positionSlider.x);
	const double ratio = std::clamp(posX / static_cast<double>(positionSlider.width), 0.0, 1.0);

	position = static_cast<int>(std::round(ratio * static_cast<double>(tabValue.size() - 1)));
}

void CToolbarSliderVideo::ClickLeftPage(const int& localX)
{
	CalculZoomPosition(localX);

	if (eventInterface != nullptr)
	{
		if (videoTimeMode)
			eventInterface->MoveSlider(currentTimeMs);
		else
			eventInterface->SlidePosChange(position, L"");
	}
}

void CToolbarSliderVideo::ClickRightPage(const int& localX)
{
	CalculZoomPosition(localX);

	if (eventInterface != nullptr)
	{
		if (videoTimeMode)
			eventInterface->MoveSlider(currentTimeMs);
		else
			eventInterface->SlidePosChange(position, L"");
	}
}

bool CToolbarSliderVideo::FindCirclePos(const int& yPos, const int& xPos)
{
	const int posXButtonBegin = this->x + positionButton.x;
	const int posXButtonEnd = posXButtonBegin + positionButton.width;
	const int posYButtonBegin = this->y + positionButton.y;
	const int posYButtonEnd = posYButtonBegin + positionButton.height;

	return xPos >= posXButtonBegin && xPos <= posXButtonEnd &&
		yPos >= posYButtonBegin && yPos <= posYButtonEnd;
}

void CToolbarSliderVideo::ClickElement(wxWindow* window, const int& xPos, const int& yPos)
{
	if (window == nullptr)
		return;

	const int railLeft = this->x + positionSlider.x - SLIDER_THUMB_RADIUS;
	const int railRight = this->x + positionSlider.x + positionSlider.width + SLIDER_THUMB_RADIUS;

	if (xPos < railLeft || xPos > railRight)
		return;

	if (FindCirclePos(yPos, xPos))
	{
		mouseBlock = true;
		captureBall = true;

		if (!window->HasCapture())
			window->CaptureMouse();

		wxSetCursor(hCursorHand);
		return;
	}

	const int localX = xPos - this->x;
	const int thumbCenter = positionButton.x + positionButton.width / 2;

	if (localX > thumbCenter)
		ClickRightPage(localX);
	else
		ClickLeftPage(localX);
}

void CToolbarSliderVideo::UnclickElement(wxWindow* window, const int& xPos, const int& yPos)
{
	if (captureBall)
	{
		mouseBlock = false;
		captureBall = false;

		if (window != nullptr && window->HasCapture())
			window->ReleaseMouse();
	}
}

bool CToolbarSliderVideo::MouseOver(wxDC* deviceContext, const int& xPos, const int& yPos)
{


	if (!mouseBlock || tabValue.empty())
		return false;

	wxSetCursor(hCursorHand);

	const int localX = xPos - this->x;
	CalculZoomPosition(localX);

	if (eventInterface != nullptr)
	{
		if (videoTimeMode)
			eventInterface->MoveSlider(currentTimeMs);
		else
			eventInterface->SlidePosChange(position, L"");
	}

	return true;
}

void CToolbarSliderVideo::CalculPositionButton()
{
	if (positionSlider.width <= 0)
	{
		positionButton = wxRect(0, 0, 0, 0);
		return;
	}

	const double ratio = GetPositionRatio();
	const int thumbX = positionSlider.x + static_cast<int>(ratio * positionSlider.width);
	CalculPositionButton(thumbX);
}

void CToolbarSliderVideo::CalculPositionButton(const int& centerX)
{
	const int thumbSize = SLIDER_THUMB_RADIUS * 2 + 2;
	const int buttonWidth = std::max(themeSlider.GetButtonWidth(), thumbSize);
	const int buttonHeight = std::max(themeSlider.GetButtonHeight(), thumbSize);

	positionButton.x = centerX - buttonWidth / 2;
	positionButton.y = positionSlider.y + (positionSlider.height - buttonHeight) / 2;
	positionButton.width = buttonWidth;
	positionButton.height = buttonHeight;
}

// ============================================================
// Rendu vectoriel (style CSliderVideo)
// ============================================================

void CToolbarSliderVideo::DrawShapeElement(wxGraphicsContext* gc, const wxRect& rc)
{
	if (gc == nullptr || rc.width <= 0)
		return;

	const double railHeight = SLIDER_TRACK_HEIGHT;
	const double railRadius = railHeight / 2.0;
	const double ratio = GetPositionRatio();
	const double progressWidth = static_cast<double>(rc.width) * ratio;

	// Rail inactif
	gc->SetPen(*wxTRANSPARENT_PEN);
	gc->SetBrush(wxBrush(GetRailInactiveColour()));
	gc->DrawRoundedRectangle(rc.x, rc.y, rc.width, railHeight, railRadius);

	// Rail actif
	if (progressWidth > 0.0)
	{
		gc->SetBrush(wxBrush(GetRailActiveColour()));
		gc->DrawRoundedRectangle(rc.x, rc.y, progressWidth, railHeight,
			std::min(railRadius, progressWidth / 2.0));
	}
}

void CToolbarSliderVideo::DrawThumb(wxGraphicsContext* gc, const wxRect& rcButton)
{
	if (gc == nullptr)
		return;

	const double centerX = rcButton.x + rcButton.width / 2.0;
	const double centerY = rcButton.y + rcButton.height / 2.0;
	const double radius = mouseBlock ? SLIDER_THUMB_RADIUS + 1.0 : SLIDER_THUMB_RADIUS;

	gc->SetBrush(wxBrush(GetThumbFillColour(themeSlider.colorBack)));
	gc->SetPen(wxPen(GetRailActiveColour(), 2));
	gc->DrawEllipse(centerX - radius, centerY - radius, radius * 2.0, radius * 2.0);
}

void CToolbarSliderVideo::SetPosition(const int& x, const int& y)
{
	CToolbarElement::SetPosition(x, y);
}

// ============================================================
// Rendu complet : [Libellé Gauche] --- Rail --- [Libellé Droite]
// ============================================================

void CToolbarSliderVideo::DrawButton(wxDC* dc, const int& x, const int& y)
{
	if (dc == nullptr)
		return;

	const int sWidth = themeSlider.GetWidth();
	const int sHeight = themeSlider.GetHeight();

	if (sWidth <= 0 || sHeight <= 0)
		return;

	// 1. Fond
	if (colorBackground)
	{
		wxRect rc(x, y, sWidth, sHeight);
		CWindowMain::FillRect(dc, rc, themeSlider.colorBack);
	}
	else if (background.IsOk())
	{
		dc->DrawBitmap(background, x, y);
	}

	// 2. Préparation des libellés (personnalisés ou valeurs par défaut)
	const wxString textLeft = useCustomLeftLabel
		? leftLabel
		: wxString::Format("%d", GetPositionValue());

	const wxString textRight = useCustomRightLabel
		? rightLabel
		: wxString::Format("%d", GetLastValue());

	wxSize sizeLeft = CWindowMain::GetSizeTexte(dc, textLeft, themeSlider.font);
	wxSize sizeRight = CWindowMain::GetSizeTexte(dc, textRight, themeSlider.font);

	const int margin = SLIDER_TEXT_MARGIN;
	const int centerY = sHeight / 2;

	// 3. Dessin du libellé AVANT (gauche)
	const int xLeftText = margin;
	const int yLeftText = (sHeight - sizeLeft.y) / 2;
	if (!textLeft.IsEmpty())
	{
		CWindowMain::DrawTexte(dc, textLeft, x + xLeftText, y + yLeftText, themeSlider.font);
	}
	// 4. Dessin du libellé APRÈS (droite)
	const int xRightText = sWidth - sizeRight.x - margin;
	const int yRightText = (sHeight - sizeRight.y) / 2;
	if (!textRight.IsEmpty())
	{
		CWindowMain::DrawTexte(dc, textRight, x + xRightText, y + yRightText, themeSlider.font);
	}
	// 5. Calcul de la zone du rail entre les deux libellés
	const int railAreaLeft = (textLeft.IsEmpty() ? margin : xLeftText + sizeLeft.x + margin);
	const int railAreaRight = (textRight.IsEmpty() ? sWidth - margin : xRightText - margin);
	const int railAreaWidth = std::max(0, railAreaRight - railAreaLeft);
	posRectangle = wxRect(railAreaLeft, 0, railAreaWidth, sHeight);
	positionSlider.x = posRectangle.x + SLIDER_TRACK_MARGIN;
	positionSlider.y = centerY - SLIDER_TRACK_HEIGHT / 2;
	positionSlider.width = std::max(0, posRectangle.width - SLIDER_TRACK_MARGIN * 2);
	positionSlider.height = SLIDER_TRACK_HEIGHT;
	CalculPositionButton();
	// 6. Rendu vectoriel du rail et du curseur
	std::unique_ptr< wxGraphicsContext> gc(wxGraphicsContext::CreateFromUnknownDC(*dc));
	if (!gc)
		return;
	wxRect rcRail = positionSlider;
	rcRail.Offset(x, y);
	DrawShapeElement(gc.get(), rcRail);
	wxRect rcThumb = positionButton;
	rcThumb.Offset(x, y);
	DrawThumb(gc.get(), rcThumb);
}


void CToolbarSliderVideo::SetTotalDuration(const int64_t& millisecondTime)
{
	videoTimeMode = true;
	totalDurationMs = millisecondTime;
	SetRightLabel(CConvertUtility::GetTimeLibelle(totalDurationMs / 1000));
}

void CToolbarSliderVideo::SetCurrentTime(const int64_t& millisecondTime)
{
	if (!mouseBlock)
	{
		currentTimeMs = millisecondTime;
		SetLeftLabel(CConvertUtility::GetTimeLibelle(currentTimeMs / 1000));
	}
}

