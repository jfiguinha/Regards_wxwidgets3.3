#pragma once
#include <theme.h>
#include "ToolbarElement.h"
//#include "SliderInterface.h"

class WXDLLIMPEXP_FWD_CORE wxGraphicsContext;

namespace Regards::Window
{
	class CSliderInterface;

	class CToolbarSlide : public CToolbarElement
	{
	public:
		CToolbarSlide(const CThemeSlider& themeSlider, CSliderInterface* eventInterface);
		~CToolbarSlide() = default;
		void Resize(const int& tailleX, const int& tailleY) override;
		void SetPosition(const int& iPos);
		void SetTabValue(vector<int> value);
		void DrawButton(wxDC* dc, const int& x, const int& y) override;
		int GetPosition();
		int GetNbValue();
		void ZoomPos(const int& position);
		int GetWidth() override;
		int GetHeight() override;
		void SetBackgroundBitmap(const wxBitmap& background);
		bool MouseOver(wxDC* context, const int& x, const int& y) override;
		void UnclickElement(wxWindow* window, const int& x, const int& y) override;
		void ClickElement(wxWindow* window, const int& x, const int& y) override;
		int GetPositionValue();
		bool IsDragging() const override { return mouseBlock; }

	private:
		void CalculZoomPosition(const int& x);
		void CalculPositionButton(const int& x);
		void CalculPositionButton();

		bool FindCirclePos(wxWindow* window, const int& y, const int& x);
		void ClickLeftPage(const int& x);
		void ClickRightPage(const int& x);
		void DrawShapeElement(wxGraphicsContext* gc, const wxRect& rc);
		void DrawThumb(wxGraphicsContext* gc, const wxRect& rcButton);
		void DrawModernButton(wxGraphicsContext* gc, const wxRect& rc, bool plus, bool pressed);
		wxColour GetThumbColour() const;
		void ChangePosition(const int& delta);
		double GetPositionRatio() const;
		void RenderSlide(wxDC* deviceContext, const int& width, const int& height, const int& x, const int& y);

		int GetFirstValue();
		int GetLastValue();


		wxBitmap background;
		CSliderInterface* eventInterface;
		CThemeSlider themeSlider;
		wxCursor hCursorHand;
		wxRect positionButton;
		wxRect positionSlider;
		bool mouseBlock;
		bool captureBall;
		bool colorBackground;
		int position;
		vector<int> tabValue;
		wxRect posRectangle;
		wxRect minusRect;
		wxRect plusRect;
		bool minusPressed;
		bool plusPressed;

		wxBitmap slideCacheBuffer;

		void DrawModernSlider(wxGraphicsContext* gc);
		void DrawModernIcons(wxGraphicsContext* gc);

		wxColour disabled{ 160, 166, 174 };

	};
}