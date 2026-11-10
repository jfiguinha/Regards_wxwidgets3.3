#pragma once
#include "GeneralToolbar.h"
#include <SliderInterface.h>
#include <ToolbarSeparator.h>

namespace Regards::Viewer
{
	// Maquette vidéo :
	// [|< < (play) > >|] | [repeat volume zoom shrink] | [rot270 rot90 flipH flipV save export]
	class CPreviewVideoToolbar final : public CGeneralToolbar
	{
	public:
		CPreviewVideoToolbar(wxWindow* parent, wxWindowID id, const CThemeToolbar& theme,
		                     CToolbarInterface* toolbarInterface, CSliderInterface* sliderEvent,
		                     const bool& vertical);
		~CPreviewVideoToolbar() override = default;

		void DisableNavigationButton() override;
		void EnableNavigationButton() override;

		// Appelés par le lecteur : SetPlay() = la vidéo est en lecture (affiche "pause"),
		// SetPause() = la vidéo est en pause (affiche "play"). Même sémantique que CSliderVideo.
		void SetPlay();
		void SetPause();

	protected:
		void EventManager(const int& id) override;

	private:
		void ShowPopup(CToolbarButton* button, int value, int count, const wxString& key);

		std::unique_ptr<CToolbarButton> repeat;
		std::unique_ptr<CToolbarButton> volume;
		std::unique_ptr<CToolbarButton> zoom;
		std::unique_ptr<CToolbarButton> shrink;
		std::unique_ptr<CToolbarSeparator> separatorNav;
		std::unique_ptr<CToolbarSeparator> separatorMedia;

		CSliderInterface* sliderEvent;
	};
}
