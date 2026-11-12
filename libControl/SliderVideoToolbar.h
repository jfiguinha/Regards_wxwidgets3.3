#pragma once
#pragma once
#include <ToolbarWindow.h>
#include <ToolbarButton.h>
#include <SliderInterface.h>
#include "ToolbarSliderVideo.h"
#include "VolumePopup.h"
#include <ToolbarSeparator.h>

using namespace Regards::Window;

namespace Regards::Video
{
	class CSliderVideoToolbar : public CToolbarWindow, public CSliderInterface
	{
	public:
		CSliderVideoToolbar(wxWindow* parent, wxWindowID id, CSliderInterface* sliderEvent,
			const CThemeToolbar& theme, const bool& vertical = false);
		~CSliderVideoToolbar() override = default;

		void SetPlay();
		void SetPause();
		void SetZoomIndex(const int& index);
		void SetTotalSecondTime(const int64_t& millisecondTime);
		void SetPastSecondTime(const int64_t& millisecondTime);
		void UpdatePositionEvent();

		

		// Implémentation de CSliderInterface
		void SlidePosChange(const int& position, const wxString& key) override;
		void MoveSlider(const int64_t& position) override;
		void ZoomPos(const int& position) override;
		void ClickButton(const int& id) override;
		void SetTrackBarPosition(const int& iPos) override;

	private:
		void EventManager(const int& id) override;
		void ShowVolumePopup();
		void ShowZoomPopup();
		wxRect GetButtonRect(CToolbarButton* targetButton) const;

		CSliderInterface* sliderEvent = nullptr;
		Regards::UI::CVolumePopupModern* popup = nullptr;

		std::unique_ptr<CToolbarSeparator> separatorPlay;
		std::unique_ptr<CToolbarButton> btnPlay;
		std::unique_ptr<CToolbarButton> btnPause;
		std::unique_ptr<CToolbarSliderVideo> videoSlide;
		std::unique_ptr<CToolbarSeparator> separatorSlide;
		std::unique_ptr<CToolbarButton> btnRepeat;
		std::unique_ptr<CToolbarButton> btnSpeaker;
		std::unique_ptr<CToolbarButton> btnZoom;
		std::unique_ptr<CToolbarButton> btnScreenFormat;

		bool isPlay = false;
		bool repeatActif = false;
		int zoomIndex = 0;
	};
}
