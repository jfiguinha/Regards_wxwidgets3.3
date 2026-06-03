#pragma once
#include <theme.h>
#include <WindowMain.h>

namespace Regards::Window
{
	class CScrollbarVerticalWnd : public CWindowMain
	{
	public:
		CScrollbarVerticalWnd(const wxString& windowName, wxWindow* parent, wxWindowID id,
		                      const CThemeScrollBar& theme);
		~CScrollbarVerticalWnd() override;

		int GetWidthSize();
		void ShowEmptyRectangle(const bool& show, const int& heightSize);

		bool DefineSize(const int& screenHeight, const int& pictureHeight);
		//bool SetPosition(const int &top);

		void SetPageSize(const int& pageSize);
		int GetPageSize();
		void SetLineSize(const int& lineSize);
		int GetLineSize();


		int GetPosition();

		int GetScreenHeight();
		int GetPictureHeight();

		bool UpdateScrollBar(const int& posHauteur, const int& screenHeight, const int& pictureHeight);

		bool IsMoving();

		void UpdateScreenRatio() override;

		void SetShowWindow(const bool& showValue);

		void ClickTopTriangle();
		void ClickBottomTriangle();
		void ClickTopPage();
		void ClickBottomPage();

		bool SetPosition(const int& top);

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

		void OnTimerTriangleTop(wxTimerEvent& event);
		void OnTimerTriangleBottom(wxTimerEvent& event);
		void OnTimerPageTop(wxTimerEvent& event);
		void OnTimerPageBottom(wxTimerEvent& event);
		void OnTimerStopMoving(wxTimerEvent& event);

		void OnEraseBackground(wxEraseEvent& /*event*/) override {}

		void Resize() override;

		void SendTopPosition(int value) const;

		bool FindTopTriangle(int y, int x) const;
		bool FindBottomTriangle(int y, int x) const;
		bool FindRectangleBar(int y, int x) const;

		void DrawTopTriangleElement(wxDC* dc, const wxRect& rc, const wxColour& color);
		void DrawBottomTriangleElement(wxDC* dc, const wxRect& rc, const wxColour& color);
		void DrawRectangleElement(wxDC* dc, const wxColour& color);

		// Shared implementation for all four Click*() methods
		void ScrollBy(int delta);

		void MoveBar(int currentPos, const wxColour& color);
		void SetIsMoving();
		void CalculBarSize();

		void FillRect(wxDC* dc, const wxRect& rc, const wxColour& color);

		void ClampPosition();

		// Helper methods for clamping (kept for compatibility but functionality merged into ClampPosition)
		bool TestMaxY();
		bool TestMinY();

	private:
			static constexpr int kBarSizeMin = 20;
			static constexpr int kDefaultLineSize = 5;
			static constexpr int kDefaultPageSize = 50;
			static constexpr int kTimerIntervalMs = 100;
			static constexpr int kStopMovingMs = 1000;

			int yPositionStart = 0;
			int yPositionStartMove = 0;

			wxRect rcPosTriangleTop{};
			wxRect rcPosTriangleBottom{};
			wxRect rcPosBar{};

			int  barSize = 0;
			int  barPosY = 0;
			bool captureBar = false;

			int  pictureHeight = 0;
			int  screenHeight = 0;
			int  pageSize = kDefaultPageSize;
			int  lineSize = kDefaultLineSize;
			int  pageSizeDefault = kDefaultPageSize;
			int  lineSizeDefault = kDefaultLineSize;

			int  barStartY = 0;
			int  barEndY = 0;
			int  currentYPos = 0;

			bool showEmptyRectangle = false;
			int  heightSize = 0;

			bool m_bTracking = false;
			bool scrollMoving = false;
			bool showTriangle = false;
			bool showWindow = true;

			// Owned timers – stored by value, no heap allocation
			wxTimer triangleTopTimer{ this };
			wxTimer triangleBottomTimer{ this };
			wxTimer pageTopTimer{ this };
			wxTimer pageBottomTimer{ this };
			wxTimer stopMovingTimer{ this };

			CThemeScrollBar themeScroll;
	};
}
