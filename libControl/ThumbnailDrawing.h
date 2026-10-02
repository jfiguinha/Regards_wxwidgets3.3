#pragma once
#include "ThumbnailVerticalSeparator.h"
#include <memory>
#include <thread>

class CImageLoadingFormat;
class CRegardsConfigParam;

namespace Regards::Control
{
	class CInfosSeparationBarEffect;

	class CThumbnailDrawing : public CThumbnailVerticalSeparator
	{
	public:
		CThumbnailDrawing(wxWindow* parent, wxWindowID idCTreeWithScrollbarInterface,
			const CThemeThumbnail& themeThumbnail, const bool& testValidity, const int& panelInfosId);
		~CThumbnailDrawing(void) override;

		void Init();

		void UpdateScroll() override;

		void OnPictureClick(const int& numPhotoId) override;

	private:


		CInfosSeparationBarEffect* CreateNewSeparatorBar(const wxString& libelle);
		static bool ItemCompFonct(int x, int y, CIcone* icone, CWindowMain* parent);
		CIcone* FindElement(const int& xPos, const int& yPos) override;

		int barseparationHeight;

		CRegardsConfigParam* config;
		int panelInfosId = 0;
		wxString drawingEffect;

	};
}
