#include "header.h"
#include "ScrollbarVerticalWnd.h"

#include <window_id.h>
#include <wx/dcbuffer.h>
using namespace Regards::Window;

#define BARSIZEMIN 20
#define LINESIZE 5
#define PAGESIZE 50

enum
{
	TIMER_TRIANGLETOP = 10,
	TIMER_TRIANGLEBOTTOM = 11,
	TIMER_PAGETOP = 12,
	TIMER_PAGEBOTTOM = 13,
	TIMER_STOPMOVING = 14
};

CScrollbarVerticalWnd::CScrollbarVerticalWnd(const wxString& windowName, wxWindow* parent, wxWindowID id,
											 const CThemeScrollBar& theme)
	: CWindowMain(windowName, parent, id)
	, themeScroll(theme)
{
	showTriangle = true;

	// --- Bind events (replaces deprecated Connect()) ---
	Bind(wxEVT_PAINT, &CScrollbarVerticalWnd::on_paint, this);
	Bind(wxEVT_MOTION, &CScrollbarVerticalWnd::OnMouseMove, this);
	Bind(wxEVT_LEFT_DOWN, &CScrollbarVerticalWnd::OnLButtonDown, this);
	Bind(wxEVT_LEFT_UP, &CScrollbarVerticalWnd::OnLButtonUp, this);
	Bind(wxEVT_ENTER_WINDOW, &CScrollbarVerticalWnd::OnMouseHover, this);
	Bind(wxEVT_LEAVE_WINDOW, &CScrollbarVerticalWnd::OnMouseLeave, this);
	Bind(wxEVT_ERASE_BACKGROUND, &CScrollbarVerticalWnd::OnEraseBackground, this);
	Bind(wxEVT_MOUSE_CAPTURE_LOST, &CScrollbarVerticalWnd::OnMouseCaptureLost, this);

	// Timer events bound directly to their wxTimer objects
	triangleTopTimer.Bind(wxEVT_TIMER, &CScrollbarVerticalWnd::OnTimerTriangleTop, this);
	triangleBottomTimer.Bind(wxEVT_TIMER, &CScrollbarVerticalWnd::OnTimerTriangleBottom, this);
	pageTopTimer.Bind(wxEVT_TIMER, &CScrollbarVerticalWnd::OnTimerPageTop, this);
	pageBottomTimer.Bind(wxEVT_TIMER, &CScrollbarVerticalWnd::OnTimerPageBottom, this);
	stopMovingTimer.Bind(wxEVT_TIMER, &CScrollbarVerticalWnd::OnTimerStopMoving, this);
}


void CScrollbarVerticalWnd::OnMouseCaptureLost(wxMouseCaptureLostEvent& /*event*/)
{
	captureBar = false;
}

void CScrollbarVerticalWnd::UpdateScreenRatio()
{
	Resize();
}

int CScrollbarVerticalWnd::GetWidthSize()
{
	return themeScroll.GetRectangleSize() + (themeScroll.GetMarge() * 2);
}

void CScrollbarVerticalWnd::CalculBarSize()
{
	barStartY = showTriangle
		? (themeScroll.GetMarge() * 2 + themeScroll.GetRectangleSize())
		: 0;

	barEndY = GetWindowHeight() - barStartY;

	if (showEmptyRectangle)
		barEndY -= heightSize;

	const int diff = pictureHeight - screenHeight;
	const int trackHeight = barEndY - barStartY;

	if (trackHeight <= 0 || diff <= 0)
	{
		barSize = kBarSizeMin;
		barPosY = barStartY;
		return;
	}

	// Ideal bar size proportional to the visible fraction
	barSize = static_cast<int>(
		static_cast<float>(trackHeight) * static_cast<float>(screenHeight)
		/ static_cast<float>(pictureHeight));

	if (barSize < kBarSizeMin)
	{
		barSize = kBarSizeMin;
		const float usableTrack = static_cast<float>(trackHeight - barSize);
		if (usableTrack > 0.f)
		{
			lineSize = static_cast<int>(
				std::ceil(static_cast<float>(diff) / usableTrack));
		}
		pageSize = lineSize * 10;
	}

	barPosY = barStartY + (lineSize > 0 ? currentYPos / lineSize : 0);
}


bool CScrollbarVerticalWnd::DefineSize(const int& screenHeight, const int& pictureHeight)
{
	if (this->pictureHeight == pictureHeight && this->screenHeight == screenHeight)
	{
		return false;
	}

	this->pictureHeight = pictureHeight;
	this->screenHeight = screenHeight;


	if (barEndY > 0)
	{
		pageSize = pageSizeDefault;
		lineSize = lineSizeDefault;
		CalculBarSize();
		rcPosBar.x = themeScroll.GetMarge();
		rcPosBar.width = themeScroll.GetMarge() + themeScroll.GetRectangleSize();
		rcPosBar.y = barPosY;
		rcPosBar.height = barPosY + barSize;
	}
	return true;
}

bool CScrollbarVerticalWnd::UpdateScrollBar(const int& posHauteur, const int& screenHeight, const int& pictureHeight)
{
	bool needToShow = false;


	if (pictureHeight > screenHeight && !this->IsShown())
	{
		Show(true);
		needToShow = true;
	}
	else if (pictureHeight <= screenHeight && IsShown())
	{
		Show(false);
		needToShow = true;
	}

	if (IsShown())
	{
		bool needToRedraw = false;
		bool return_value = DefineSize(screenHeight, pictureHeight);
		if (return_value)
			needToRedraw = true;
		return_value = SetPosition(posHauteur);
		if (return_value)
			needToRedraw = true;

		if (needToRedraw)
		{
			PaintNow();
		}
	}

	return needToShow;
}

int CScrollbarVerticalWnd::GetPosition()
{
	return currentYPos;
}

int CScrollbarVerticalWnd::GetScreenHeight()
{
	return screenHeight;
}

int CScrollbarVerticalWnd::GetPictureHeight()
{
	return pictureHeight;
}

bool CScrollbarVerticalWnd::SetPosition(const int& top)
{
	bool value = true;
	if (top != currentYPos)
	{
		currentYPos = top;
		MoveBar(currentYPos, themeScroll.colorBarActif);
	}
	else
		value = false;
	return value;
}

void CScrollbarVerticalWnd::DrawElement(wxDC* dc)
{
	wxRect rc;
	rc.x = 0;
	rc.y = 0;
	rc.width = GetWindowWidth();
	rc.height = GetWindowHeight();

	FillRect(dc, rc, themeScroll.colorBack);

	if (showTriangle)
	{
		DrawTopTriangleElement(dc, rcPosTriangleTop, themeScroll.colorTriangle);
		DrawBottomTriangleElement(dc, rcPosTriangleBottom, themeScroll.colorTriangle);
	}
	if (captureBar)
		DrawRectangleElement(dc, themeScroll.colorBarActif);
	else
		DrawRectangleElement(dc, themeScroll.colorBar);
}

void CScrollbarVerticalWnd::ShowEmptyRectangle(const bool& show, const int& heightSize)
{
	if (showEmptyRectangle != show)
	{
		showEmptyRectangle = show;
		this->heightSize = heightSize;
		Resize();
	}
	//this->Redraw();
}

void CScrollbarVerticalWnd::SetPageSize(const int& pageSize)
{
	pageSizeDefault = pageSize;
	this->pageSize = pageSize;
}

int CScrollbarVerticalWnd::GetPageSize()
{
	return pageSize;
}

void CScrollbarVerticalWnd::SetLineSize(const int& lineSize)
{
	lineSizeDefault = lineSize;
	this->lineSize = lineSize;
}

int CScrollbarVerticalWnd::GetLineSize()
{
	return lineSize;
}


CScrollbarVerticalWnd::~CScrollbarVerticalWnd()
{
	triangleTopTimer.Stop();
	triangleBottomTimer.Stop();
	pageTopTimer.Stop();
	pageBottomTimer.Stop();
	stopMovingTimer.Stop();
}

void CScrollbarVerticalWnd::DrawTopTriangleElement(wxDC* dc, const wxRect& rc, const wxColour& color)
{
	wxBrush brushHatch(color);
	dc->SetBrush(brushHatch);
	dc->SetPen(wxNullPen);
	wxPoint star[3];
	star[0] = wxPoint((rc.width - rc.x) / 2 + themeScroll.GetMarge(), rc.y);
	star[1] = wxPoint(rc.x, rc.height);
	star[2] = wxPoint(rc.width, rc.height);
	dc->DrawPolygon(WXSIZEOF(star), star, 0, 0);
	dc->SetBrush(wxNullBrush);
	dc->SetPen(wxNullPen);
}


void CScrollbarVerticalWnd::DrawBottomTriangleElement(wxDC* dc, const wxRect& rc, const wxColour& color)
{
	wxBrush brushHatch(color);
	dc->SetBrush(brushHatch);
	wxPoint star[3];
	star[0] = wxPoint((rc.width - rc.x) / 2 + themeScroll.GetMarge(), rc.height);
	star[1] = wxPoint(rc.x, rc.y);
	star[2] = wxPoint(rc.width, rc.y);
	dc->DrawPolygon(WXSIZEOF(star), star, 0, 0);
}

void CScrollbarVerticalWnd::DrawRectangleElement(wxDC* dc, const wxColour& colorBar)
{
	dc->SetBrush(wxBrush(colorBar));
	wxRect rc = rcPosBar;
	if (rcPosBar.height > barEndY)
	{
		rcPosBar.y = barEndY - barSize;
		rcPosBar.height = barEndY;
	}

	if (rcPosBar.y < barStartY)
	{
		rcPosBar.y = barStartY;
		rcPosBar.height = barStartY + barSize;
	}

	rc.height = rcPosBar.height - rcPosBar.y;
	rc.width = GetWindowWidth() - (themeScroll.GetMarge() * 2);
	const int radius = (GetWindowWidth() / 2) - themeScroll.GetMarge();
	dc->DrawRoundedRectangle(rc, radius);
	dc->SetBrush(wxNullBrush);
}

void CScrollbarVerticalWnd::SetIsMoving()
{
	stopMovingTimer.Stop();
	scrollMoving = true;
	stopMovingTimer.Start(kStopMovingMs, wxTIMER_ONE_SHOT);

	wxCommandEvent evt(wxEVENT_SCROLLMOVE);
	evt.SetInt(1);
	GetParent()->GetEventHandler()->AddPendingEvent(evt);
}

bool CScrollbarVerticalWnd::IsMoving()
{
	return scrollMoving;
}


void CScrollbarVerticalWnd::OnMouseLeave(wxMouseEvent& event)
{

	m_bTracking = FALSE;

	triangleTopTimer.Stop();
	triangleBottomTimer.Stop();
	pageTopTimer.Stop();
	pageBottomTimer.Stop();
	stopMovingTimer.Stop();

	scrollMoving = false;

	wxCommandEvent evt(wxEVENT_SCROLLMOVE);
	evt.SetInt(0);
	GetParent()->GetEventHandler()->AddPendingEvent(evt);

	if (HasCapture())
		ReleaseMouse();
	captureBar = false;
}

void CScrollbarVerticalWnd::OnMouseHover(wxMouseEvent& events)
{
	//::wxSetCursor(wxCursor( wxSTANDARD_CURSOR ) );
	wxSetCursor(wxCursor(wxCURSOR_ARROW));
}

void CScrollbarVerticalWnd::Resize()
{
	if (showTriangle)
		barStartY = themeScroll.GetMarge() + themeScroll.GetMarge() + themeScroll.GetRectangleSize();
	else
		barStartY = themeScroll.GetRectangleSize();
	barEndY = GetWindowHeight() - barStartY;
	int tailleY = GetWindowHeight();

	if (showEmptyRectangle)
	{
		barEndY -= heightSize;
		tailleY -= heightSize;
	}

	if (barPosY == 0)
		barPosY = barStartY;

	if (showTriangle)
	{
		const int marge = themeScroll.GetMarge();
		const int size = themeScroll.GetRectangleSize();

		rcPosTriangleTop.x = marge;
		rcPosTriangleTop.width = marge + size;
		rcPosTriangleTop.y = marge;
		rcPosTriangleTop.height = marge + size;

		rcPosTriangleBottom.x = marge;
		rcPosTriangleBottom.width = marge + size;
		rcPosTriangleBottom.y = tailleY - size - marge;
		rcPosTriangleBottom.height = tailleY - marge;
	}


	if (barEndY > 0)
	{
		pageSize = pageSizeDefault;
		lineSize = lineSizeDefault;
		CalculBarSize();
		rcPosBar.x = themeScroll.GetMarge();
		rcPosBar.width = themeScroll.GetMarge() + themeScroll.GetRectangleSize();
		rcPosBar.y = barPosY;
		rcPosBar.height = barPosY + barSize;
	}

	needToRefresh = true;
}


bool CScrollbarVerticalWnd::FindTopTriangle(int y, int x) const
{
	return y > rcPosTriangleTop.y && y < rcPosTriangleTop.height
		&& x > rcPosTriangleTop.x && x < rcPosTriangleTop.width;
}

bool CScrollbarVerticalWnd::FindBottomTriangle(int y, int x) const
{
	return y > rcPosTriangleBottom.y && y < rcPosTriangleBottom.height
		&& x > rcPosTriangleBottom.x && x < rcPosTriangleBottom.width;
}

bool CScrollbarVerticalWnd::FindRectangleBar(int y, int x) const
{
	return y > rcPosBar.y && y < rcPosBar.height
		&& x > rcPosBar.x && x < rcPosBar.width;
}

void CScrollbarVerticalWnd::MoveBar(int currentPos, const wxColour& /*color*/)
{
	const int diff = pictureHeight - screenHeight;
	const int trackHeight = barEndY - barStartY;

	int posY = 0;
	if (diff > 0 && trackHeight > barSize)
	{
		const float pct = static_cast<float>(currentPos) / static_cast<float>(diff);
		const float freeSize = static_cast<float>(trackHeight - barSize);
		posY = static_cast<int>(freeSize * pct);
	}

	rcPosBar.y = barStartY + posY;
	rcPosBar.height = rcPosBar.y + barSize;

	// Clamp bar within track
	if (rcPosBar.height > barEndY)
	{
		rcPosBar.y = barEndY - barSize;
		rcPosBar.height = barEndY;
	}
	if (rcPosBar.y < barStartY)
	{
		rcPosBar.y = barStartY;
		rcPosBar.height = barStartY + barSize;
	}

	PaintNow();
}

void CScrollbarVerticalWnd::OnMouseMove(wxMouseEvent& event)
{
	if (!captureBar)
		return;

	scrollMoving = true;
	const int diffY = event.GetY() - yPositionStart;
	yPositionStartMove = yPositionStart = event.GetY();
	currentYPos += diffY * lineSize;
	ClampPosition();
	MoveBar(currentYPos, themeScroll.colorBarActif);
	SendTopPosition(currentYPos);
	PaintNow();
	SetIsMoving();
}

void CScrollbarVerticalWnd::SetShowWindow(const bool& showValue)
{
	if (this->showWindow != showValue)
		this->showWindow = showValue;
}

void CScrollbarVerticalWnd::SendTopPosition(int value) const
{
	if (!showWindow)
		return;

	wxWindow* window = GetParent();
	if (window)
	{
		wxCommandEvent evt(wxEVENT_TOPPOSITION);
		evt.SetInt(value);
		window->GetEventHandler()->AddPendingEvent(evt);
	}
}

bool CScrollbarVerticalWnd::TestMaxY()
{
	const int diff = pictureHeight - screenHeight;
	if (currentYPos > diff)
	{
		currentYPos = diff;
		return true;
	}
	return false;
}

bool CScrollbarVerticalWnd::TestMinY()
{
	if (currentYPos < 0)
	{
		currentYPos = 0;
		return true;
	}
	return false;
}

void CScrollbarVerticalWnd::ClampPosition()
{
	const int diff = pictureHeight - screenHeight;
	if (currentYPos < 0)    currentYPos = 0;
	if (currentYPos > diff) currentYPos = diff;
}

void CScrollbarVerticalWnd::ScrollBy(int delta)
{
	currentYPos += delta;
	ClampPosition();
	MoveBar(currentYPos, themeScroll.colorBar);
	SendTopPosition(currentYPos);
}

void CScrollbarVerticalWnd::ClickTopTriangle()
{
	ScrollBy(-lineSize);
}

void CScrollbarVerticalWnd::ClickBottomTriangle()
{
	ScrollBy(+lineSize);
}

void CScrollbarVerticalWnd::ClickTopPage()
{
	ScrollBy(-pageSize);
}

void CScrollbarVerticalWnd::ClickBottomPage()
{
	ScrollBy(+pageSize);
}


void CScrollbarVerticalWnd::OnLButtonDown(wxMouseEvent& event)
{
	int xPos = event.GetX();
	int yPos = event.GetY();
	//bool initTimer = false;

	//if(showTriangle)

	if (showTriangle && FindTopTriangle(yPos, xPos))
	{
		scrollMoving = true;
		ClickTopTriangle();
		triangleTopTimer.Start(kTimerIntervalMs);
	}
	else if (showTriangle && FindBottomTriangle(yPos, xPos))
	{
		ClickBottomTriangle();
		triangleBottomTimer.Start(kTimerIntervalMs);
	}
	else if (FindRectangleBar(yPos, xPos))
	{
		//SetIsMoving();
		yPositionStartMove = yPositionStart = yPos;
		CaptureMouse();
		captureBar = true;
	}
	else if (yPos > rcPosBar.height)
	{
		//SetIsMoving();
		//initTimer = true;
		ClickBottomPage();
		pageBottomTimer.Start(kTimerIntervalMs);
	}
	else if (yPos < rcPosBar.y)
	{
		//initTimer = true;
		ClickTopPage();
		pageTopTimer.Start(kTimerIntervalMs);
		//SetIsMoving();
	}
}

void CScrollbarVerticalWnd::OnTimerTriangleTop(wxTimerEvent& event)
{
	ClickTopTriangle();
}

void CScrollbarVerticalWnd::OnTimerTriangleBottom(wxTimerEvent& event)
{
	ClickBottomTriangle();
}

void CScrollbarVerticalWnd::OnTimerPageTop(wxTimerEvent& event)
{
	ClickTopPage();
}

void CScrollbarVerticalWnd::OnTimerPageBottom(wxTimerEvent& event)
{
	ClickBottomPage();
}


void CScrollbarVerticalWnd::OnTimerStopMoving(wxTimerEvent& event)
{
	scrollMoving = false;
	stopMovingTimer.Stop();
}

void CScrollbarVerticalWnd::OnLButtonUp(wxMouseEvent& event)
{
	if (captureBar)
	{
		if (HasCapture())
			ReleaseMouse();
		captureBar = false;

		const int diff = event.GetY() - yPositionStartMove;
		currentYPos += diff * lineSize;
		ClampPosition();
		SendTopPosition(currentYPos);
	}

	triangleTopTimer.Stop();
	triangleBottomTimer.Stop();
	pageTopTimer.Stop();
	pageBottomTimer.Stop();

	captureBar = false;
}

void CScrollbarVerticalWnd::PaintNow()
{
	Refresh();
	Update();
}

void CScrollbarVerticalWnd::on_paint(wxPaintEvent& event)
{
	wxBufferedPaintDC dc(this);
	DrawElement(&dc);
}

void CScrollbarVerticalWnd::FillRect(wxDC* dc, const wxRect& rc, const wxColour& color)
{
	dc->SetBrush(wxBrush(color));
	dc->DrawRectangle(rc);
	dc->SetBrush(wxNullBrush);
}
