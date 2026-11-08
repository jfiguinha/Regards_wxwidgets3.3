#pragma once
//#include "TreeElement.h"
#include "TreeElementSlideInterface.h"
#include "Tree.h"
#include <Metadata.h>
#include "PositionElement.h"
#include "TreeElementDelete.h"
#include <wx/tokenzr.h>
#include <treetypeid.h>


namespace Regards::Window
{
	class CTreeData;
	class CTreeElementControlInterface;
	class CTreeElementListBox;
	class CTreeElementTexteClick;
	class CTreeElementDelete;
	class CTreeElementTriangle;
	class CTreeElementTexte;
	class CTreeElementSlide;
	class CTreeElementCheckBox;
	class CTreeElementStar;
	class CTreeElementColor;
	class CTreeElementComboBox;

	class CTreeControl : public CTreeElementSlideInterface
	{
	public:
		CTreeControl() : eventControl(nullptr) { nbRow = 0; };
		CTreeControl(CThemeTree* theme, CTreeElementControlInterface* interfaceControl);
		virtual ~CTreeControl();

		CTreeControl& operator=(const CTreeControl& other);

		void SlidePosChange(CTreeElement* treeElement, const int& position, CTreeElementValue* value,
		                    const wxString& key) override = 0;
		void GenerateWindowBitmap(wxDC* deviceContext, const int& width, const int& height, const int& posLargeur,
		                          const int& posHauteur);
		CPositionElement* FindElement(const int& x, const int& y);

		int GetWidth();
		int GetNbRow();
		int GetWidthRow(const int& numRow);

		virtual void MouseOver(wxDC* deviceContext, CPositionElement* element, const int& x, const int& y,
		                       const int& posLargeur, const int& posHauteur, bool& update)
		{
			if (element != nullptr)
			{
				CTreeElement* treeElement = element->GetTreeElement();
				if (treeElement != nullptr)
					treeElement->MouseOver(deviceContext, x, y, update);
			}
		}

		template<typename TreeDataT, typename LeafInitializer>
		void AddTreeInfosImpl(const wxString& exifKey,
			int index,
			tree<CTreeData*>::iterator& top,
			tree<CTreeData*>::iterator& child,
			LeafInitializer initLeaf)
		{
			wxStringTokenizer tokenizer(exifKey, ".");

			int level = 0;

			while (tokenizer.HasMoreTokens())
			{
				const wxString key = tokenizer.GetNextToken();
				const bool hasChildren = tokenizer.HasMoreTokens();

				auto* treeData = new TreeDataT();
				treeData->SetKey(key);

				if (hasChildren)
				{
					treeData->SetIsParent(true);

					if (index > 0)
					{
						tree<CTreeData*>::iterator it;

						if (level == 0)
							it = FindKey(key);
						else
							it = FindKey(key, child);

						if (it != nullptr) // replace with tr.end() if appropriate
						{
							child = it;
							++level;
							delete treeData;
							continue;
						}
					}

					if (level > 0)
						child = tr.append_child(child, treeData);
					else
						child = tr.insert(top, treeData);
				}
				else
				{
					treeData->SetIsParent(false);

					// Let caller initialize leaf-specific data
					initLeaf(treeData);

					tr.append_child(child, treeData);
				}

				++level;
			}
		}

		virtual void MouseOut(wxDC* deviceContext, CPositionElement* element, const int& x, const int& y,
		                      const int& posLargeur, const int& posHauteur, bool& update)
		{
			update = false;
		};

		virtual void ClickOnElement(CPositionElement* element, wxWindow* window, const int& x, const int& y,
		                            const int& posLargeur, const int& posHauteur)
		{
		};

		virtual void UnclickOnElement(CPositionElement* element, wxWindow* window, const int& x, const int& y,
		                              const int& posLargeur, const int& posHauteur)
		{
		};

		virtual void DoubleClickOnElement(CPositionElement* element)
		{
		};

		virtual void AfterDrawBitmap()
		{
		};

		// Appele au debut de GenerateWindowBitmap (largeur/hauteur reelles de la zone dessinee)
		virtual void BeforeDrawBitmap(wxDC* deviceContext, const int& width, const int& height,
		                              const int& posLargeur, const int& posHauteur);

		// Appele a la fin de GenerateWindowBitmap (apres le dessin des elements)
		virtual void AfterDrawElements(wxDC* deviceContext, const int& width, const int& height,
		                               const int& posLargeur, const int& posHauteur);
		virtual void UpdateScreenRatio() = 0;

	protected:

		void HideAll();

		virtual bool GetCheckState(CTreeData* data)
		{
			return true;
		}

		virtual bool GetTriangleState(CTreeData* data)
		{
			return true;
		}

		enum class RenderMode
		{
			Create,
			Update
		};

		CPositionElement* RenderStar(
			CTreeData* data,
			int& xPos,
			int& yPos,
			bool visible,
			RenderMode mode);

		CPositionElement* RenderText(
			CTreeData* data,
			int& xPos,
			int& yPos,
			bool visible,
			RenderMode mode,
			bool addDynamic = false);

		CPositionElement* RenderCheckBox(
			CTreeData* data,
			int& xPos,
			int& yPos,
			bool check,
			bool visible,
			RenderMode mode);

		CPositionElement* RenderTriangle(
			CTreeData* data,
			int& xPos,
			int& yPos,
			bool visible,
			RenderMode mode);

		CPositionElement* RenderTextValue(
			CTreeData* data,
			int& xPos,
			int& yPos,
			bool visible,
			RenderMode mode);

		CPositionElement* RenderTextLink(
			CTreeData* data,
			int& xPos,
			int& yPos,
			bool visible,
			RenderMode mode);

		CPositionElement* RenderColor(
			CTreeData* data,
			int& xPos,
			int& yPos,
			const wxColour& initialColor,
			bool visible,
			RenderMode mode);

		// ===== Style moderne (optionnel) ===========================================
		// Fond uni, filets de 1 px entre les lignes, palette derivee du theme et mise en page
		// calculee sur la largeur reelle de la zone dessinee. Un arbre qui n'appelle pas
		// EnableModernStyle() garde exactement son rendu actuel.
		void EnableModernStyle();
		// labelValueLayout = true : colonne libelles + colonne valeurs ; false : une seule colonne
		void ModernUpdateMetrics(const bool& labelValueLayout, const int& labelPercent = 45);
		int ModernToRawX(const int& pixels);
		int ModernToRawY(const int& pixels);
		void ModernStyleText(CPositionElement* posElement, const int& zoneWidth, const int& insetLeft,
		                     const int& insetRight, const wxColour& textColor, const bool& bold,
		                     const int& alignment);
		void ModernColorText(CPositionElement* posElement, const wxColour& textColor);
		// Place un element de la colonne valeurs, aligne a droite (tronque si trop large)
		void ModernPlaceValue(CPositionElement* posElement, const int& y);
		// Ligne de titre : TITRE en gras + chevron a droite. clickable = clic sur toute la ligne
		CTreeElementTriangle* ModernRenderHeaderRow(CTreeData* data, RenderMode mode, int& yPos,
		                                            const bool& clickable);
		// Appele quand la largeur de la zone change : l'arbre doit recalculer ses positions
		virtual void OnModernRelayout()
		{
		};





		virtual wxColour GetBackgroundColour(const int& yPos);

		//Tree Window
		tree<CTreeData*>::iterator FindKey(const wxString& key, tree<CTreeData*>::iterator& parent);
		tree<CTreeData*>::iterator FindKey(const wxString& key);

		void EraseChildTree(tree<CTreeData*>::sibling_iterator& parent);
		void ClearData();

		CPositionElement* CreatePositionElement(const int& x, const int& y, const int& numColumn, const int& numRow,
		                                        const int& width, const int& height, const int& type,
		                                        CTreeElement* treeElement, CTreeData* treeData,
		                                        const bool& dynamic = true);
		CPositionElement* GetElement(CTreeData* data, const int& typeElement);

		CTreeElementListBox* CreateListBoxElement(const int& width, const int& height, const vector<CMetadata>& value,
		                                          const int& index, const wxString& exifKey);
		CTreeElementTexteClick* CreateTexteLinkElement(const int& width, const int& height, const wxString& libelle,
		                                               const wxString& link, const int& linkType);
		CTreeElementDelete* CreateDeleteElement(const int& width, const int& height);
		CTreeElementTriangle* CreateTriangleElement(const int& width, const int& height, const bool& open);
		CTreeElementTexte* CreateTexteElement(const int& width, const int& height, const wxString& libelle);
		CTreeElementSlide* CreateSlideElement(const int& width, const int& height, CTreeElementValue* position,
		                                      vector<CTreeElementValue*>* value, const wxString& exifKey);
		CTreeElementCheckBox* CreateCheckBoxElement(const int& width, const int& height, const bool& check);
		CTreeElementStar* CreateStarElement(const int& width, const int& height, const wxString& libelle,
		                                    const wxString& value, const int& numPhotoId);

		// --- À ajouter dans la section public: de la classe CTreeControl ---
		CTreeElementColor* CreateColorElement(const int& width, const int& height, const wxColour& initialColor);
		// --- À ajouter dans la section protected: de la classe CTreeControl ---
		CTreeElementComboBox* CreateComboBoxElement(const int& width, const int& height,
			const std::vector<wxString>& items, wxString exifKey,
			const int& defaultSelection = 0);


		//Element de rendu pour l'arbre
		tree<CTreeData*> tr;
		PositionElementVector vectorPosElement;
		PositionElementVector vectorPosElementDynamic;

		CThemeTree themeTree;

		// Style moderne
		bool modernStyle = false;
		int modernPanelWidth = 0;
		int modernWidth = 0;
		int modernLabelWidth = 0;
		int modernValueWidth = 0;
		int modernChevronWidth = 0;
		int modernMargin = 0;
		int modernRowHeight = 0;
		wxColour modernBack;
		wxColour modernBorder;
		wxColour modernActive;
		wxColour modernTitle;
		wxColour modernLabel;
		wxColour modernValue;
		wxColour modernEmpty;

		vector<int> rowWidth;
		int nbRow;
		CTreeElementControlInterface* eventControl;
	};
}
