#pragma once
#include <memory>
#include <cstdint>
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
		void AddModification(const int& numEffect, CEffectParameter* effectParameter, const wxString& libelle, CLayerList* currentLayers, int64_t targetLayerId);

		void AddModificationState(const wxString& libelle, CLayerList* currentLayers, int64_t targetLayerId);

		void UpdateScreenRatio() override;

		void SlidePosChange(CTreeElement* treeElement, const int& position, CTreeElementValue* value,
			const wxString& key) override
		{};

	private:
		void InitTree(const wxString& libelle, const wxString& key);
		wxString GetNumModification();

		// Clé de l'élément d'arbre correspondant à l'étape modif (la racine garde la clé donnée à InitTree)
		wxString KeyForModification(unsigned int modif) const;

		// Se positionne sur l'étape modif : récupère les calques, met à jour le manager,
		// restaure dans le visualiseur et active l'élément activeKey. Retourne false si l'étape est invalide.
		bool GoToModification(unsigned int modif, const wxString& activeKey);

		// Si l'utilisateur est revenu en arrière (Undo), reconstruit l'arbre graphique jusqu'à l'étape courante
		void RebuildTreeIfHistoryTruncated();

		// Ajoute dans l'arbre l'étape qui vient d'être enregistrée par le manager et la rend active
		void AppendEvent(const wxString& libelle);

		// Transmet la liste des calques restaurée au visualiseur (prend la propriété de layers)
		void RestoreLayersToViewer(std::unique_ptr<CLayerList> layers, const int64_t& layerId);

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
		wxString rootKey; // clé de l'élément racine (étape 0), mémorisée par InitTree
	};
}
