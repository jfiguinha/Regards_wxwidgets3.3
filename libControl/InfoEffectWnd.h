//
//  InfoEffectWnd.h
//  Regards.libViewer
//

#pragma once
#include "InfoEffect.h"
#include "LayerElement.h" // Nécessaire pour manipulator CLayerList
#include <TreeWithScrollbar.h>
using namespace Regards::Window;

class CImageLoadingFormat;

namespace Regards::Control
{
	class CInfoEffectWnd : public CTreeWithScrollbar
	{
	public:
		CInfoEffectWnd(wxWindow* parent, wxWindowID id, const CThemeScrollBar& themeScroll,
			const CThemeTree& themeTree, int bitmap_window_id, bool eraseMemory = true);
		~CInfoEffectWnd(void);

		CTreeElementControlInterface* GetTreeInterface();

		void HistoryRefresh();

		void SetHistoryEffect(CInfoEffect* infoEffect);

		void Redo();
		void Undo();
		void CancelHistory();

		// Prise en compte du paramètre currentLayers pour alimenter le gestionnaire d'historique
		void AddModification(const int& numEffect, CEffectParameter* effectParameter, const wxString& libelle, CLayerList* currentLayers);

		void AddModificationState(const wxString& libelle, CLayerList* currentLayers);

		// Prise en compte de initialLayers lors de l'initialisation de l'historique
		void HistoryUpdate(CImageLoadingFormat* bitmap, CLayerList* currentLayers, const int64_t& layerId, const wxString& filename,
			const wxString& historyLibelle, CModificationManager* modificationManager);

	private:
		CInfoEffect* historyEffectOld;
		int bitmapWindowId;
		bool eraseMemory = true;
	};
}