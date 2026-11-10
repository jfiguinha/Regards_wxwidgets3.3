#pragma once
#include <theme.h>
#include "ToolbarWindow.h"
//#include "SliderInterface.h"
//#include <ToolbarInterface.h>
using namespace Regards::Window;

namespace Regards
{
	namespace Window
	{
		class CToolbarInterface;
	}

	namespace Viewer
	{
		// Barre "générale" : navigation + transformations + save/export.
		// Sert aussi de base à CPreviewBitmapToolbar et CPreviewVideoToolbar, qui
		// réutilisent ses boutons et réordonnent navElement selon leur maquette.
		class CGeneralToolbar : public CToolbarWindow
		{
		public:
			CGeneralToolbar(wxWindow* parent, wxWindowID id, const CThemeToolbar& theme,
				CToolbarInterface* toolbarInterface, const bool& vertical);
			~CGeneralToolbar() override = default;

			void SetFullscreen();
			void SetScreen();
			virtual void DisableNavigationButton();
			virtual void EnableNavigationButton();
			void EnableSaveButton();
			void DisableSaveButton();
			void EnableExportButton();
			void DisableExportButton();
			void EnableScreenButton();
			void DisableScreenButton();

		protected:
			void EventManager(const int& id) override;

			std::unique_ptr<CToolbarButton> imagePlayDiaporama;
			std::unique_ptr<CToolbarButton> imageStopDiaporama;
			std::unique_ptr<CToolbarButton> fullscreen;
			std::unique_ptr<CToolbarButton> imageEnd;
			std::unique_ptr<CToolbarButton> imageSuiv;
			std::unique_ptr<CToolbarButton> imageFirst;
			std::unique_ptr<CToolbarButton> imagePrec;
			std::unique_ptr<CToolbarButton> save;
			std::unique_ptr<CToolbarButton> exportFile;
			std::unique_ptr<CToolbarButton> editFile;
			std::unique_ptr<CToolbarButton> editor;
			std::unique_ptr<CToolbarButton> rotate90;
			std::unique_ptr<CToolbarButton> rotate270;
			std::unique_ptr<CToolbarButton> flipVertical;
			std::unique_ptr<CToolbarButton> flipHorizontal;

		private:
			void DiaporamaStart();
			void DiaporamaStop();
			void Rotate90();
			void Rotate270();
			void FlipVertical();
			void FlipHorizontal();
			void Fullscreen();
			void NextPicture();
			void PreviousPicture();
			void FirstPicture();
			void LastPicture();
			void Save();

			CToolbarInterface* toolbarInterface;
			bool navigationButtonEnable = true;
		};
	}
}
