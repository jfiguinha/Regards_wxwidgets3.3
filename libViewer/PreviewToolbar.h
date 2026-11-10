#pragma once
#include <theme.h>
#include "ToolbarWindow.h"
//#include "SliderInterface.h"
#include <ToolbarInterface.h>
#include <SliderInterface.h>
#include <array>
#include <vector>
using namespace Regards::Window;


enum class MediaMode
{
	General,
	Bitmap,
	Animation,
	Video
};

namespace Regards
{

	namespace Viewer
	{
		class CGeneralToolbar;
		class CPreviewBitmapToolbar;
		class CPreviewVideoToolbar;

		class CPreviewToolbar final : public CWindowMain
		{
		public:
			// viewerId       : fenêtre bitmap pilotée par la barre Bitmap (zoom, print, email...)
			// videoInterface : récepteur des clics de la barre Video (même rôle que CSliderVideo::sliderEvent)
			CPreviewToolbar(wxWindow* parent, wxWindowID id, const CThemeToolbar& theme,
			                CToolbarInterface* toolbarInterface, const bool& vertical,
			                wxWindowID viewerId = wxID_ANY, CSliderInterface* videoInterface = nullptr);
			~CPreviewToolbar() = default;

			void SetMediaToolbar(MediaMode mediaMode);

			int GetHeight() override;
			void SetFullscreen();
			void SetScreen();
			void DisableNavigationButton();
			void EnableNavigationButton();
			void EnableSaveButton();
			void DisableSaveButton();
			void EnableExportButton();
			void DisableExportButton();
			void EnableScreenButton();
			void DisableScreenButton();
			void UpdateScreenRatio();

			// Barre Bitmap
			void SetTabValue(const std::vector<int>& value);
			void SetTrackBarPosition(const int& position);

			// Barre Video
			void SetVideoPlay();
			void SetVideoPause();

		private:
			void Resize() override;
			std::array<CGeneralToolbar*, 3> AllToolbars() const;

			MediaMode mediaMode = MediaMode::General;
			CGeneralToolbar* generalToolbar = nullptr;
			CPreviewBitmapToolbar* bitmapToolbar = nullptr;
			CPreviewVideoToolbar* videoToolbar = nullptr;
			CGeneralToolbar* activeToolbar = nullptr;
		};
	}
}
