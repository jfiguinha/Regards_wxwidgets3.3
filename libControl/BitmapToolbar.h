#pragma once
#include <ToolbarWindow.h>
#include <SliderInterface.h>
#include <ToolbarSlide.h>
#include <ToolbarSeparator.h>
#include <MediaMode.h>
using namespace Regards::Window;

namespace Regards::Window
{
	class CToolbarInterface;
}

namespace Regards::Control
{

	class CBitmapToolbar : public CToolbarWindow, public CSliderInterface
	{
	public:
		CBitmapToolbar(wxWindow* parent, wxWindowID id, wxWindowID viewerId, const CThemeToolbar& theme,
			const bool& vertical, const bool& exportPicture,
			CToolbarInterface* toolbarInterface = nullptr);
		~CBitmapToolbar() = default;

		// Choisit les boutons affichés selon le type de média
		void SetMedia(const MediaMode& mode);
		MediaMode GetMedia() const { return mediaMode; }

		//void SetBitmapDisplayPt(CBitmapWndViewer * bitmapWindow);
		void SetTrackBarPosition(const int& iPos) override;
		void SetTabValue(vector<int> value);
		void ZoomOn();
		void ZoomOut();
		//void ShowExportButton();
		//void HideExportButton();
		void SlidePosChange(const int& position, const wxString& key) override;

		void MoveSlider(const int64_t& position) override
		{};
		void ZoomPos(const int& position) override;
		void ClickButton(const int& id) override;
		void ChangeZoomInPos();
		void ChangeZoomOutPos();

		// Mode Animation (ex CAnimationToolbar)
		void AnimationStart();
		void AnimationStop();

	private:
		void EventManager(const int& id) override;
		void EventManagerBitmap(const int& id);
		bool EventManagerAnimation(const int& id); // true si l'id est une commande d'animation

		void ApplyMedia();
		void NextPicture();
		void PreviousPicture();

		// Mode Bitmap
		std::unique_ptr<CToolbarButton> email;
		std::unique_ptr<CToolbarButton> printer;
		std::unique_ptr<CToolbarButton> shrink;
		//std::unique_ptr<CToolbarButton> moins;
		//std::unique_ptr<CToolbarButton> plus;
		std::unique_ptr<CToolbarSlide> slide;

		// Mode Animation
		std::unique_ptr<CToolbarButton> imagePlayDiaporama;
		std::unique_ptr<CToolbarButton> imageStopDiaporama;
		std::unique_ptr<CToolbarButton> imagePrec;
		std::unique_ptr<CToolbarButton> imageSuiv;

		// Séparation : navigation animation (gauche) | autres boutons (droite)
		std::unique_ptr<CToolbarSeparator> separator;

		CToolbarInterface* toolbarInterface = nullptr;
		MediaMode mediaMode = MediaMode::Bitmap;
		bool animationRunning = false;

		wxWindowID parentId;
		bool exportPicture;
	};
}