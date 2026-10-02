#pragma once
#include "ThumbnailVertical.h"
#include <memory>
#include <thread>

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

		void SetFile(const wxString& filename, CImageLoadingFormat* imageLoading);
		wxString GetFilename();

		void Resize();

		void OnPictureClick(const int& numPhotoId) override;

	private:

		
		static bool ItemCompFonct(int x, int y, CIcone* icone, CWindowMain* parent);
		CIcone* FindElement(const int& xPos, const int& yPos) override;

		void ProcessIdle() override;


		CRegardsConfigParam* config;
		wxString calqueLibelle;
		wxString filename;
		CImageLoadingFormat* imageLoading;
		bool isAllProcess = false;
	};
}
