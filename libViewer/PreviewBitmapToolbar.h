#pragma once
#include "GeneralToolbar.h"
#include <SliderInterface.h>
#include <ToolbarSlide.h>
#include <ToolbarSeparator.h>
#include <vector>

namespace Regards::Viewer
{
	// Maquette image :
	// [|< < (play) > >|] | [shrink] 25 % ---o--- 1600 % | [rot270 rot90 flipH flipV] | [email print save export]
	class CPreviewBitmapToolbar final : public CGeneralToolbar, public CSliderInterface
	{
	public:
		CPreviewBitmapToolbar(wxWindow* parent, wxWindowID id, wxWindowID viewerId, const CThemeToolbar& theme,
		                      CToolbarInterface* toolbarInterface, const bool& vertical);
		~CPreviewBitmapToolbar() override = default;

		void DisableNavigationButton() override;
		void EnableNavigationButton() override;

		void SetTabValue(const std::vector<int>& value);

		// CSliderInterface
		void SetTrackBarPosition(const int& iPos) override;
		void SlidePosChange(const int& position, const wxString& key) override;
		void MoveSlider(const int64_t& position) override {}
		void ZoomPos(const int& position) override;
		void ClickButton(const int& id) override {}

	protected:
		void EventManager(const int& id) override;

	private:
		void UpdateZoomText(const int& position);
		wxString FormatPercent(const int& value) const;

		std::unique_ptr<CToolbarButton> email;
		std::unique_ptr<CToolbarButton> printer;
		std::unique_ptr<CToolbarButton> shrink;
		std::unique_ptr<CToolbarSlide> slide;
		std::unique_ptr<CToolbarTexte> textCurrent;
		std::unique_ptr<CToolbarTexte> textMax;
		std::unique_ptr<CToolbarSeparator> separatorNav;
		std::unique_ptr<CToolbarSeparator> separatorZoom;
		std::unique_ptr<CToolbarSeparator> separatorShare;

		std::vector<int> zoomValues;
		wxWindowID parentId;
	};
}
