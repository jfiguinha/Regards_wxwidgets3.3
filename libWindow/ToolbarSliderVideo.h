#pragma once
#include <theme.h>
#include "ToolbarElement.h"

class WXDLLIMPEXP_FWD_CORE wxGraphicsContext;

namespace Regards::Window
{
	class CSliderInterface;

	class CToolbarSliderVideo : public CToolbarElement
	{
	public:
		CToolbarSliderVideo(const CThemeSlider& themeSlider, CSliderInterface* eventInterface);
		~CToolbarSliderVideo() = default;

		void Resize(const int& tailleX, const int& tailleY) override;
		void SetPosition(const int& iPos);
		void SetTabValue(std::vector<int> value);
		void DrawButton(wxDC* dc, const int& x, const int& y) override;

		void SetPosition(const int& x, const int& y) override;

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

		// Gestion des libellés avant et après le slider
		void SetLeftLabel(const wxString& label);
		void SetRightLabel(const wxString& label);
		void SetLabels(const wxString& left, const wxString& right);

		void SetTotalDuration(const int64_t& millisecondTime);
		void SetCurrentTime(const int64_t& millisecondTime);
		int64_t GetCurrentTime() const { return currentTimeMs; }
		bool IsDragging() const { return mouseBlock; }

	private:
		void CalculZoomPosition(const int& x);
		void CalculPositionButton(const int& x);
		void CalculPositionButton();

		bool FindCirclePos(const int& y, const int& x);
		void ClickLeftPage(const int& x);
		void ClickRightPage(const int& x);

		void DrawShapeElement(wxGraphicsContext* gc, const wxRect& rc);
		void DrawThumb(wxGraphicsContext* gc, const wxRect& rcButton);
		double GetPositionRatio() const;

		int GetFirstValue();
		int GetLastValue();

		// Section private
		int64_t totalDurationMs = 0;
		int64_t currentTimeMs = 0;
		bool videoTimeMode = false;

		wxBitmap background;
		CSliderInterface* eventInterface;
		CThemeSlider themeSlider;
		wxCursor hCursorHand;

		wxRect positionButton;
		wxRect positionSlider;
		wxRect posRectangle;

		bool mouseBlock;
		bool captureBall;
		bool colorBackground;
		int position;
		std::vector<int> tabValue;

		wxString leftLabel;
		wxString rightLabel;
		bool useCustomLeftLabel = false;
		bool useCustomRightLabel = false;
	};
}
