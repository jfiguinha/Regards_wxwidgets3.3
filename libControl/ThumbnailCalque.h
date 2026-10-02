#pragma once
#include "ThumbnailVerticalSeparator.h"
#include <memory>
#include <thread>

class CImageLoadingFormat;
class CRegardsConfigParam;

namespace Regards::Control
{
	class CInfosSeparationBarEffect;

	class CThumbnailCalque : public CThumbnailVerticalSeparator
	{
	public:
		CThumbnailCalque(wxWindow* parent, wxWindowID idCTreeWithScrollbarInterface,
			const CThemeThumbnail& themeThumbnail);
		~CThumbnailCalque(void) override;

		void SetFile(const wxString& filename, CImageLoadingFormat* imageLoading);
		wxString GetFilename();

		void UpdateScroll() override;

		void OnPictureClick(const int& numPhotoId) override;

	private:

		CInfosSeparationBarEffect* CreateNewSeparatorBar(const wxString& libelle);
		static bool ItemCompFonct(int x, int y, CIcone* icone, CWindowMain* parent);
		CIcone* FindElement(const int& xPos, const int& yPos) override;

		void ProcessIdle() override;



		int barseparationHeight;

		CRegardsConfigParam* config;
		wxString calqueLibelle;
		wxString filename;
		CImageLoadingFormat* imageLoading;
		bool isAllProcess = false;
	};
}
