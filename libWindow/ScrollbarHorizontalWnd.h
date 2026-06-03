#pragma once
#include <theme.h>
#include <WindowMain.h>

namespace Regards::Window
{
    class CScrollInterface;

    class CScrollbarHorizontalWnd : public CWindowMain
    {
    public:
        CScrollbarHorizontalWnd(const wxString& windowName, wxWindow* parent, wxWindowID id,
            const CThemeScrollBar& theme);
        ~CScrollbarHorizontalWnd() override;

        int  GetHeightSize();

        void SetPageSize(int pageSize);
        int  GetPageSize() const;
        void SetLineSize(int lineSize);
        int  GetLineSize() const;

        bool DefineSize(int screenWidth, int pictureWidth);
        bool SetPosition(int left);

        int  GetPosition() const;
        int  GetScreenWidth() const;
        int  GetPictureWidth() const;
        bool IsMoving() const;

        bool UpdateScrollBar(int posLargeur, int screenWidth, int pictureWidth);

        void UpdateScreenRatio() override;
        void SetShowWindow(bool showValue);

        void ClickLeftTriangle();
        void ClickRightTriangle();
        void ClickLeftPage();
        void ClickRightPage();

    protected:
        void PaintNow();
        void DrawElement(wxDC* dc);

        void on_paint(wxPaintEvent& event);
        void OnMouseMove(wxMouseEvent& event);
        void OnLButtonDown(wxMouseEvent& event);
        void OnLButtonUp(wxMouseEvent& event);
        void OnMouseLeave(wxMouseEvent& event);
        void OnMouseHover(wxMouseEvent& event);
        void OnMouseCaptureLost(wxMouseCaptureLostEvent& event);

        void OnTimerTriangleLeft(wxTimerEvent& event);
        void OnTimerTriangleRight(wxTimerEvent& event);
        void OnTimerPageLeft(wxTimerEvent& event);
        void OnTimerPageRight(wxTimerEvent& event);
        void OnTimerStopMoving(wxTimerEvent& event);

        void OnEraseBackground(wxEraseEvent& /*event*/) override {}

        void Resize() override;

        void SendLeftPosition(int value) const;

        bool FindLeftTriangle(int x, int y) const;
        bool FindRightTriangle(int x, int y) const;
        bool FindRectangleBar(int x, int y) const;

        void DrawLeftTriangleElement(wxDC* dc, const wxRect& rc, const wxColour& color);
        void DrawRightTriangleElement(wxDC* dc, const wxRect& rc, const wxColour& color);
        void DrawRectangleElement(wxDC* dc, const wxColour& color);

        // Shared implementation for all four Click*() methods
        void ScrollBy(int delta);

        void MoveBar(int currentPos, const wxColour& color);
        void SetIsMoving();
        void CalculBarSize();

        void FillRect(wxDC* dc, const wxRect& rc, const wxColour& color);

        void ClampPosition();

        // --- data members ---

        static constexpr int kBarSizeMin = 20;
        static constexpr int kDefaultLineSize = 5;
        static constexpr int kDefaultPageSize = 50;
        static constexpr int kTimerIntervalMs = 100;
        static constexpr int kStopMovingMs = 1000;

        int xPositionStart = 0;
        int xPositionStartMove = 0;

        wxRect rcPosTriangleLeft{};
        wxRect rcPosTriangleRight{};
        wxRect rcPosBar{};

        int  barSize = 0;
        int  barPosX = 0;
        bool captureBar = false;

        int  pictureWidth = 0;
        int  screenWidth = 0;
        int  pageSize = kDefaultPageSize;
        int  lineSize = kDefaultLineSize;
        int  pageSizeDefault = kDefaultPageSize;
        int  lineSizeDefault = kDefaultLineSize;

        int  barStartX = 0;
        int  barEndX = 0;
        int  currentXPos = 0;

        bool m_bTracking = false;
        bool scrollMoving = false;
        bool showTriangle = false;
        bool showWindow = true;

        // Owned timers – stored by value, no heap allocation
        wxTimer triangleLeftTimer{ this };
        wxTimer triangleRightTimer{ this };
        wxTimer pageLeftTimer{ this };
        wxTimer pageRightTimer{ this };
        wxTimer stopMovingTimer{ this };

        CThemeScrollBar themeScroll;
    };

} // namespace Regards::Window