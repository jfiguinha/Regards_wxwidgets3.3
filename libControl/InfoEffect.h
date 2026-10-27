#pragma once
#include <TreeControl.h>
#include "ModificationManager.h"
#include "LayerElement.h" // Nécessaire pour manipuler CLayerList
using namespace Regards::Window;

namespace Regards::Control
{
	class CInfoEffect : public CTreeControl
	{
	public:
		CInfoEffect(CTreeElementControlInterface* interfaceControl, CModificationManager* modificationManager,
			int bitmapWindowId, wxString filename);
		~CInfoEffect() = default;


		void Redo();
		void Undo();
		void CancelHistory();

		wxString GetFilename();
		void Init(CImageLoadingFormat* bitmap, CLayerList* currentLayers, const int64_t& layerId, const wxString& libelle, const wxString& key);
		void AddEvent(const wxString& libelle, const wxString& key);
		void SetActifElement(const wxString& key);
		void SetTreeInterface(CTreeElementControlInterface* interfaceControl);

		// Ajout d'une modification en passant l'état actuel des calques au manager
		void AddModification(const int& numEffect, CEffectParameter* effectParameter, const wxString& libelle, CLayerList* currentLayers, int targetLayerId);

		void AddModificationState(const wxString& libelle, CLayerList* currentLayers, int targetLayerId);

		void UpdateScreenRatio() override;

		void SlidePosChange(CTreeElement* treeElement, const int& position, CTreeElementValue* value,
			const wxString& key) override
		{};

	private:
		void InitTree(const wxString& libelle, const wxString& key);
		wxString GetNumModification();

		// Modifié pour transmettre la liste des calques restaurée au visualiseur
		void RestoreLayersToViewer(CLayerList * layers, const int64_t& layerId);

		void ClickOnElement(CPositionElement* element, wxWindow* window, const int& x, const int& y,
			const int& posLargeur, const int& posHauteur) override;
		void MouseOver(wxDC* deviceContext, CPositionElement* element, const int& x, const int& y,
			const int& posLargeur, const int& posHauteur, bool& update) override;

		void CreateElement(RenderMode mode);
		void UpdateChildTree(tree<CTreeData*>::sibling_iterator& parent);

		int numEvent;
		int yPos;
		int index;
		tree<CTreeData*>::iterator top;
		tree<CTreeData*>::iterator child;

		// Pointeur externe vers le gestionnaire unifié
		CModificationManager* modificationManager;

		int64_t initLayerId = 0;
		int widthPosition;
		wxString filename;
		int bitmapWindowId;
	};
}