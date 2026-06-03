#include "header.h"
#include "ScrollbarHorizontalWnd.h"
#include <window_id.h>
#include <wx/dcbuffer.h>
using namespace Regards::Window;

// ---------------------------------------------------------------------------
// Constructor / Destructor
// ---------------------------------------------------------------------------

CScrollbarHorizontalWnd::CScrollbarHorizontalWnd(const wxString& windowName,
    wxWindow* parent,
    wxWindowID      id,
    const CThemeScrollBar& theme)
    : CWindowMain(windowName, parent, id)
    , themeScroll(theme)
{
    showTriangle = true;

    // --- Bind events (replaces deprecated Connect()) ---
    Bind(wxEVT_PAINT, &CScrollbarHorizontalWnd::on_paint, this);
    Bind(wxEVT_MOTION, &CScrollbarHorizontalWnd::OnMouseMove, this);
    Bind(wxEVT_LEFT_DOWN, &CScrollbarHorizontalWnd::OnLButtonDown, this);
    Bind(wxEVT_LEFT_UP, &CScrollbarHorizontalWnd::OnLButtonUp, this);
    Bind(wxEVT_ENTER_WINDOW, &CScrollbarHorizontalWnd::OnMouseHover, this);
    Bind(wxEVT_ERASE_BACKGROUND, &CScrollbarHorizontalWnd::OnEraseBackground, this);
    Bind(wxEVT_MOUSE_CAPTURE_LOST,
        &CScrollbarHorizontalWnd::OnMouseCaptureLost, this);

    // Timer events bound directly to their wxTimer objects
    triangleLeftTimer.Bind(wxEVT_TIMER, &CScrollbarHorizontalWnd::OnTimerTriangleLeft, this);
    triangleRightTimer.Bind(wxEVT_TIMER, &CScrollbarHorizontalWnd::OnTimerTriangleRight, this);
    pageLeftTimer.Bind(wxEVT_TIMER, &CScrollbarHorizontalWnd::OnTimerPageLeft, this);
    pageRightTimer.Bind(wxEVT_TIMER, &CScrollbarHorizontalWnd::OnTimerPageRight, this);
    stopMovingTimer.Bind(wxEVT_TIMER, &CScrollbarHorizontalWnd::OnTimerStopMoving, this);
}

CScrollbarHorizontalWnd::~CScrollbarHorizontalWnd()
{
    // wxTimer is a member (not a pointer), so no delete needed.
    // Stop any running timers before they fire after partial destruction.
    triangleLeftTimer.Stop();
    triangleRightTimer.Stop();
    pageLeftTimer.Stop();
    pageRightTimer.Stop();
    stopMovingTimer.Stop();
}

// ---------------------------------------------------------------------------
// Accessors
// ---------------------------------------------------------------------------

int CScrollbarHorizontalWnd::GetHeightSize()
{
    return themeScroll.GetRectangleSize() + themeScroll.GetMarge() * 2;
}

int  CScrollbarHorizontalWnd::GetPosition()     const { return currentXPos; }
int  CScrollbarHorizontalWnd::GetScreenWidth()  const { return screenWidth; }
int  CScrollbarHorizontalWnd::GetPictureWidth() const { return pictureWidth; }
bool CScrollbarHorizontalWnd::IsMoving()        const { return scrollMoving; }

void CScrollbarHorizontalWnd::SetPageSize(int ps)
{
    pageSizeDefault = ps;
    pageSize = ps;
}

int CScrollbarHorizontalWnd::GetPageSize() const { return pageSize; }

void CScrollbarHorizontalWnd::SetLineSize(int ls)
{
    lineSizeDefault = ls;
    lineSize = ls;
}

int CScrollbarHorizontalWnd::GetLineSize() const { return lineSize; }

void CScrollbarHorizontalWnd::SetShowWindow(bool showValue)
{
    showWindow = showValue;
}

void CScrollbarHorizontalWnd::UpdateScreenRatio()
{
    Resize();
}

// ---------------------------------------------------------------------------
// Position / size management
// ---------------------------------------------------------------------------

bool CScrollbarHorizontalWnd::SetPosition(int left)
{
    if (left == currentXPos)
        return false;

    currentXPos = left;
    MoveBar(currentXPos, themeScroll.colorBarActif);
    return true;
}

bool CScrollbarHorizontalWnd::UpdateScrollBar(int posLargeur, int sw, int pw)
{
    bool needToShow = false;

    if (pw > sw && !IsShown())
    {
        Show(true);
        needToShow = true;
    }
    else if (pw <= sw && IsShown())
    {
        Show(false);
        needToShow = true;
    }

    if (IsShown())
    {
        bool needToRedraw = DefineSize(sw, pw);
        needToRedraw |= SetPosition(posLargeur);

        if (needToRedraw)
            PaintNow();
    }

    return needToShow;
}

void CScrollbarHorizontalWnd::CalculBarSize()
{
    barStartX = showTriangle
        ? (themeScroll.GetMarge() * 2 + themeScroll.GetRectangleSize())
        : 0;

    barEndX = GetWindowWidth() - barStartX;

    const int trackWidth = barEndX - barStartX;

    // Guard: nothing to compute if there is no track space or no scroll range
    const int diff = pictureWidth - screenWidth;
    if (trackWidth <= 0 || diff <= 0)
    {
        barSize = kBarSizeMin;
        barPosX = barStartX;
        return;
    }

    // Ideal bar size proportional to the visible fraction
    barSize = static_cast<int>(
        static_cast<float>(trackWidth) * static_cast<float>(screenWidth)
        / static_cast<float>(pictureWidth));

    if (barSize < kBarSizeMin)
    {
        barSize = kBarSizeMin;
        // Recompute lineSize so the bar still covers the full range
        const float usableTrack = static_cast<float>(trackWidth - barSize);
        if (usableTrack > 0.f)
        {
            lineSize = static_cast<int>(
                std::ceil(static_cast<float>(diff) / usableTrack));
        }
        pageSize = lineSize * 10;
    }

    barPosX = barStartX + (lineSize > 0 ? currentXPos / lineSize : 0);
}

bool CScrollbarHorizontalWnd::DefineSize(int sw, int pw)
{
    if (pictureWidth == pw && screenWidth == sw)
        return false;

    pictureWidth = pw;
    screenWidth = sw;

    if (barEndX > 0)
    {
        pageSize = pageSizeDefault;
        lineSize = lineSizeDefault;
        CalculBarSize();
        rcPosBar.x = barPosX;
        rcPosBar.width = barPosX + barSize;
        rcPosBar.y = themeScroll.GetMarge();
        rcPosBar.height = themeScroll.GetRectangleSize() + themeScroll.GetMarge();
    }
    return true;
}

// ---------------------------------------------------------------------------
// Resize / layout
// ---------------------------------------------------------------------------

void CScrollbarHorizontalWnd::Resize()
{
    barStartX = showTriangle
        ? (themeScroll.GetMarge() * 2 + themeScroll.GetRectangleSize())
        : 0;

    barEndX = GetWindowWidth() - barStartX;

    if (barPosX == 0)
        barPosX = barStartX;

    const int marge = themeScroll.GetMarge();
    const int size = themeScroll.GetRectangleSize();

    rcPosTriangleLeft.x = marge;
    rcPosTriangleLeft.width = marge + size;
    rcPosTriangleLeft.y = marge;
    rcPosTriangleLeft.height = marge + size;

    rcPosTriangleRight.x = (GetWindowWidth() - barStartX) + marge;
    rcPosTriangleRight.width = GetWindowWidth() - marge;
    rcPosTriangleRight.y = marge;
    rcPosTriangleRight.height = marge + size;

    if (barEndX > 0)
    {
        pageSize = pageSizeDefault;
        lineSize = lineSizeDefault;
        CalculBarSize();
        rcPosBar.x = barPosX;
        rcPosBar.width = barPosX + barSize;
        rcPosBar.y = marge;
        rcPosBar.height = size + marge;
    }

    needToRefresh = true;
}

// ---------------------------------------------------------------------------
// Scroll actions — shared implementation
// ---------------------------------------------------------------------------

void CScrollbarHorizontalWnd::ClampPosition()
{
    const int diff = pictureWidth - screenWidth;
    if (currentXPos < 0)    currentXPos = 0;
    if (currentXPos > diff) currentXPos = diff;
}

// Every ClickXxx() variant reduces to: adjust currentXPos by delta, clamp,
// redraw, notify parent.
void CScrollbarHorizontalWnd::ScrollBy(int delta)
{
    currentXPos += delta;
    ClampPosition();
    MoveBar(currentXPos, themeScroll.colorBar);
    SendLeftPosition(currentXPos);
}

void CScrollbarHorizontalWnd::ClickLeftTriangle() { ScrollBy(-lineSize); }
void CScrollbarHorizontalWnd::ClickRightTriangle() { ScrollBy(+lineSize); }
void CScrollbarHorizontalWnd::ClickLeftPage() { ScrollBy(-pageSize); }
void CScrollbarHorizontalWnd::ClickRightPage() { ScrollBy(+pageSize); }

// ---------------------------------------------------------------------------
// MoveBar
// ---------------------------------------------------------------------------

void CScrollbarHorizontalWnd::MoveBar(int currentPos, const wxColour& /*color*/)
{
    const int diff = pictureWidth - screenWidth;
    const int trackWidth = barEndX - barStartX;

    int posX = 0;
    if (diff > 0 && trackWidth > barSize)
    {
        const float pct = static_cast<float>(currentPos) / static_cast<float>(diff);
        const float freeSize = static_cast<float>(trackWidth - barSize);
        posX = static_cast<int>(freeSize * pct);
    }

    rcPosBar.x = barStartX + posX;
    rcPosBar.width = rcPosBar.x + barSize;

    // Clamp bar within track
    if (rcPosBar.width > barEndX)
    {
        rcPosBar.x = barEndX - barSize;
        rcPosBar.width = barEndX;
    }
    if (rcPosBar.x < barStartX)
    {
        rcPosBar.x = barStartX;
        rcPosBar.width = barStartX + barSize;
    }

    PaintNow();
}

// ---------------------------------------------------------------------------
// SetIsMoving
// ---------------------------------------------------------------------------

void CScrollbarHorizontalWnd::SetIsMoving()
{
    stopMovingTimer.Stop();
    scrollMoving = true;
    stopMovingTimer.Start(kStopMovingMs, wxTIMER_ONE_SHOT);

    wxCommandEvent evt(wxEVENT_SCROLLMOVE);
    evt.SetInt(1);
    GetParent()->GetEventHandler()->AddPendingEvent(evt);
}

// ---------------------------------------------------------------------------
// Hit-testing helpers
// ---------------------------------------------------------------------------

// wxRect uses (x, y, width, height) in wxWidgets but here "width" and "height"
// fields are used as right/bottom pixel coords (legacy convention in this file).
// Keep the same semantics as the original code.
static bool HitTestLocal(const wxRect& rc, int x, int y)
{
    return y > rc.y && y < rc.height && x > rc.x && x < rc.width;
}

bool CScrollbarHorizontalWnd::FindLeftTriangle(int x, int y)  const { return HitTestLocal(rcPosTriangleLeft, x, y); }
bool CScrollbarHorizontalWnd::FindRightTriangle(int x, int y) const { return HitTestLocal(rcPosTriangleRight, x, y); }
bool CScrollbarHorizontalWnd::FindRectangleBar(int x, int y)  const { return HitTestLocal(rcPosBar, x, y); }

// ---------------------------------------------------------------------------
// Mouse events
// ---------------------------------------------------------------------------

void CScrollbarHorizontalWnd::OnMouseCaptureLost(wxMouseCaptureLostEvent& /*event*/)
{
    captureBar = false;
}

void CScrollbarHorizontalWnd::OnMouseHover(wxMouseEvent& /*event*/)
{
    wxSetCursor(wxCursor(wxCURSOR_ARROW));
}

void CScrollbarHorizontalWnd::OnMouseLeave(wxMouseEvent& /*event*/)
{
    m_bTracking = false;
    triangleLeftTimer.Stop();
    triangleRightTimer.Stop();
    pageLeftTimer.Stop();
    pageRightTimer.Stop();

    wxCommandEvent evt(wxEVENT_SCROLLMOVE);
    evt.SetInt(0);
    GetParent()->GetEventHandler()->AddPendingEvent(evt);

    if (HasCapture())
        ReleaseMouse();
    captureBar = false;
}

void CScrollbarHorizontalWnd::OnMouseMove(wxMouseEvent& event)
{
    if (!captureBar)
        return;

    const int xPos = event.GetX();
    const int diffX = xPos - xPositionStart;
    xPositionStartMove = xPositionStart = xPos;

    currentXPos += diffX * lineSize;
    ClampPosition();
    MoveBar(currentXPos, themeScroll.colorBarActif);
    SendLeftPosition(currentXPos);
    SetIsMoving();
}

void CScrollbarHorizontalWnd::OnLButtonDown(wxMouseEvent& event)
{
    const int xPos = event.GetX();
    const int yPos = event.GetY();

    if (showTriangle && FindLeftTriangle(xPos, yPos))
    {
        ClickLeftTriangle();
        triangleLeftTimer.Start(kTimerIntervalMs);
    }
    else if (showTriangle && FindRightTriangle(xPos, yPos))
    {
        ClickRightTriangle();
        triangleRightTimer.Start(kTimerIntervalMs);
    }
    else if (FindRectangleBar(xPos, yPos))
    {
        xPositionStart = xPos;
        xPositionStartMove = xPos;
        CaptureMouse();
        captureBar = true;
    }
    else if (xPos > rcPosBar.width)
    {
        ClickRightPage();
        pageRightTimer.Start(kTimerIntervalMs);
    }
    else if (xPos < rcPosBar.x)
    {
        ClickLeftPage();
        pageLeftTimer.Start(kTimerIntervalMs);
    }
}

void CScrollbarHorizontalWnd::OnLButtonUp(wxMouseEvent& event)
{
    if (captureBar)
    {
        if (HasCapture())
            ReleaseMouse();
        captureBar = false;

        const int diff = event.GetX() - xPositionStartMove;
        currentXPos += diff * lineSize;
        ClampPosition();
        SendLeftPosition(currentXPos);
    }

    triangleLeftTimer.Stop();
    triangleRightTimer.Stop();
    pageLeftTimer.Stop();
    pageRightTimer.Stop();
}

// ---------------------------------------------------------------------------
// Timer events
// ---------------------------------------------------------------------------

void CScrollbarHorizontalWnd::OnTimerTriangleLeft(wxTimerEvent&) { ClickLeftTriangle(); }
void CScrollbarHorizontalWnd::OnTimerTriangleRight(wxTimerEvent&) { ClickRightTriangle(); }
void CScrollbarHorizontalWnd::OnTimerPageLeft(wxTimerEvent&) { ClickLeftPage(); }
void CScrollbarHorizontalWnd::OnTimerPageRight(wxTimerEvent&) { ClickRightPage(); }

void CScrollbarHorizontalWnd::OnTimerStopMoving(wxTimerEvent&)
{
    scrollMoving = false;
    stopMovingTimer.Stop();
}

// ---------------------------------------------------------------------------
// Painting
// ---------------------------------------------------------------------------

void CScrollbarHorizontalWnd::PaintNow()
{
    Refresh();
    Update();
}

void CScrollbarHorizontalWnd::on_paint(wxPaintEvent& /*event*/)
{
    wxBufferedPaintDC dc(this);
    DrawElement(&dc);
}

void CScrollbarHorizontalWnd::DrawElement(wxDC* dc)
{
    wxRect rc(0, 0, GetWindowWidth(), GetWindowHeight());
    FillRect(dc, rc, themeScroll.colorBack);

    if (showTriangle)
    {
        DrawLeftTriangleElement(dc, rcPosTriangleLeft, themeScroll.colorTriangle);
        DrawRightTriangleElement(dc, rcPosTriangleRight, themeScroll.colorTriangle);
    }

    DrawRectangleElement(dc, captureBar ? themeScroll.colorBarActif : themeScroll.colorBar);
}

void CScrollbarHorizontalWnd::FillRect(wxDC* dc, const wxRect& rc, const wxColour& color)
{
    dc->SetBrush(wxBrush(color));
    dc->DrawRectangle(rc);
    dc->SetBrush(wxNullBrush);
}

void CScrollbarHorizontalWnd::DrawLeftTriangleElement(wxDC* dc, const wxRect& rc, const wxColour& color)
{
    dc->SetBrush(wxBrush(color));
    const wxPoint pts[3] = {
        { rc.x,     (rc.height - rc.y) / 2 + themeScroll.GetMarge() },
        { rc.width,  rc.y },
        { rc.width,  rc.height }
    };
    dc->DrawPolygon(3, pts);
    dc->SetBrush(wxNullBrush);
}

void CScrollbarHorizontalWnd::DrawRightTriangleElement(wxDC* dc, const wxRect& rc, const wxColour& color)
{
    dc->SetBrush(wxBrush(color));
    const wxPoint pts[3] = {
        { rc.x,     rc.y },
        { rc.x,     rc.height },
        { rc.width, (rc.height - rc.y) / 2 + themeScroll.GetMarge() }
    };
    dc->DrawPolygon(3, pts);
    dc->SetBrush(wxNullBrush);
}

void CScrollbarHorizontalWnd::DrawRectangleElement(wxDC* dc, const wxColour& color)
{
    // Clamp bar rect before drawing
    if (rcPosBar.width > barEndX)
    {
        rcPosBar.x = barEndX - barSize;
        rcPosBar.width = barEndX;
    }
    if (rcPosBar.x < barStartX)
    {
        rcPosBar.x = barStartX;
        rcPosBar.width = barStartX + barSize;
    }

    wxRect rc = rcPosBar;
    rc.width = rcPosBar.width - rcPosBar.x;
    rc.height = GetWindowHeight() - (themeScroll.GetMarge() * 2);

    const int radius = (GetWindowHeight() / 2) - themeScroll.GetMarge();
    dc->SetBrush(wxBrush(color));
    dc->DrawRoundedRectangle(rc, radius);
    dc->SetBrush(wxNullBrush);
}

void CScrollbarHorizontalWnd::SendLeftPosition(int value) const
{
    if (!showWindow)
        return;

    wxWindow* window = GetParent();
    if (window)
    {
        wxCommandEvent evt(wxEVENT_LEFTPOSITION);
        evt.SetInt(value);
        window->GetEventHandler()->AddPendingEvent(evt);
    }
}