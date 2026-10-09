#pragma once
#include "ThumbnailVertical.h"
#include <memory>
#include <thread>
#include "LayerElement.h"

class CImageLoadingFormat;
class CRegardsConfigParam;

namespace Regards::Control
{
	class CInfosSeparationBarEffect;

	class CThumbnailCalque : public CThumbnailVertical
	{
	public:
		CThumbnailCalque(wxWindow* parent, wxWindowID idCTreeWithScrollbarInterface,
			const CThemeThumbnail& themeThumbnail);
		~CThumbnailCalque(void) override;

		void SetLayer(CLayerList * listOfLayer);
		void RefreshList();

		void Resize();
		int GetSelectLayer();
		void OnPictureClick(const int& numPhotoId) override;

	private:

		
		static bool ItemCompFonct(int x, int y, CIcone* icone, CWindowMain* parent);
		CIcone* FindElement(const int& xPos, const int& yPos) override;

		void ProcessIdle() override;

		CLayerList * listOfLayer = nullptr;
		CRegardsConfigParam* config;
		wxString calqueLibelle;
		wxString filename;
		CImageLoadingFormat* imageLoading;
		bool isAllProcess = false;
	};
}
