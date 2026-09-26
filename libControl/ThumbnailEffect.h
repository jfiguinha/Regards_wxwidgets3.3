#pragma once
#include "ThumbnailVerticalSeparator.h"
#include <memory>
#include <thread>

class CImageLoadingFormat;
class CRegardsConfigParam;

namespace Regards::Control
{
	class CInfosSeparationBarEffect;

	class CThumbnailEffect : public CThumbnailVerticalSeparator
	{
	public:
		CThumbnailEffect(wxWindow* parent, wxWindowID idCTreeWithScrollbarInterface,
			const CThemeThumbnail& themeThumbnail, const bool& testValidity);
		~CThumbnailEffect(void) override;

		void SetFile(const wxString& filename, CImageLoadingFormat* imageLoading);
		wxString GetFilename();

		void UpdateScroll() override;

	private:
		void GetBitmapDimension(const int& width, const int& height, int& tailleAffichageBitmapWidth,
			int& tailleAffichageBitmapHeight, float& newRatio);
		float CalculRatio(const int& width, const int& height, const int& tailleBitmapWidth,
			const int& tailleBitmapHeight);
		CInfosSeparationBarEffect* CreateNewSeparatorBar(const wxString& libelle);
		static bool ItemCompFonct(int x, int y, CIcone* icone, CWindowMain* parent);
		CIcone* FindElement(const int& xPos, const int& yPos) override;

		void ProcessIdle() override;
		static void LoadPicture(void* param);
		void UpdateRenderIcone(wxCommandEvent& event);

		bool isAllProcess;
		wxString filename;
		int barseparationHeight;

		CRegardsConfigParam* config;
		wxString colorEffect;
		wxString convolutionEffect;
		wxString specialEffect;
		wxString histogramEffect;
		wxString blackRoomEffect;
		wxString rotateEffect;
		wxString hdrEffect;
		wxString videoLabelEffect;

		// CORRECTIONS CRITIQUES POUR LE MULTI-THREAD :
		// 1. Remplacement de l'unique_ptr par shared_ptr pour que les threads en arrière-plan
		//    maintiennent la ressource en vie si l'utilisateur change d'image rapidement.
		std::shared_ptr<CImageLoadingFormat> sharedImageLoading;

		// 2. Identifiant unique de session pour rejeter les calculs des threads obsolètes.
		int currentProcessId = 0;

		int nbProcess = 0;
	};
}
