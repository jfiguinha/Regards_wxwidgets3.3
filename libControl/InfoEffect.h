#pragma once
#include <TreeControl.h>
#include "ModificationManager.h"
using namespace Regards::Window;

namespace Regards::Control
{
	class CInfoEffect : public CTreeControl
	{
	public:
		CInfoEffect(CTreeElementControlInterface* interfaceControl, CModificationManager* modificationManager,
			int bitmapWindowId);
		~CInfoEffect() = default;

		wxString GetFilename();
		void Init(CImageLoadingFormat* bitmap, const wxString& libelle, const wxString& key);
		void AddEvent(const wxString& libelle, const wxString& key);
		void SetActifElement(const wxString& key);

		// Conserve la signature, mais en interne le bitmap n'est plus enregistré sur le disque
		// Remplacer l'ancienne signature par la nouvelle
		void AddModification(const int& numEffect, CEffectParameter* effectParameter, const wxString& libelle);

		void UpdateScreenRatio() override;

		void SlidePosChange(CTreeElement* treeElement, const int& position, CTreeElementValue* value,
			const wxString& key) override
		{};

	private:
		void InitTree(const wxString& libelle, const wxString& key);
		wxString GetNumModification();
		void SetBitmapToViewer(CImageLoadingFormat* bitmap);
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

		// Pointeur externe
		CModificationManager* modificationManager;

		// Sauvegarde locale de l'image d'origine pour rejouer l'historique non destructif
		CImageLoadingFormat* baseBitmap = nullptr;

		int widthPosition;
		wxString filename;
		int bitmapWindowId;
	};
}
