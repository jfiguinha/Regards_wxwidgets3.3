#pragma once
#include <theme.h>
#include <TreeControl.h>
//#include <TreeElementControlInterface.h>
//#include <PositionElement.h>


namespace Regards
{
	namespace Window
	{
		class CTreeElementTriangle;
		class CTreeElementTexte;
		class CPositionElement;
		class CTreeElementControlInterface;
	}

	using namespace Window;

	namespace Control
	{
		class CInfosFile : public CTreeControl
		{
		public:
			CInfosFile(CThemeTree* theme, CTreeElementControlInterface* interfaceControl);

			~CInfosFile(void) = default;

			void SetFile(const wxString& filename);
			const wxString GetFilename();
			void UpdateScreenRatio() override;


			void SlidePosChange(CTreeElement* treeElement, const int& position, CTreeElementValue* value,
				const wxString& key) override
			{};

			void CreateElement();

			// Largeur disponible du panneau (en pixels). A rappeler a chaque redimensionnement :
			// les valeurs sont alignees a droite sur ce bord.
			void SetPanelWidth(const int& width);

		private:
			void MouseOver(wxDC* dc, CPositionElement* element, const int& x, const int& y, const int& posLargeur,
				const int& posHauteur, bool& update) override;
			void ClickOnElement(CPositionElement* element, wxWindow* window, const int& x, const int& y,
				const int& posLargeur, const int& posHauteur) override;

			void BeforeDrawBitmap(wxDC* deviceContext, const int& width, const int& height,
				const int& posLargeur, const int& posHauteur) override;
			wxColour GetBackgroundColour(const int& yPos) override;

			void RenderElement(RenderMode mode);
			CTreeElementTriangle* RenderHeaderRow(CTreeData* data, const int& profondeur, RenderMode mode);
			void UpdateLayoutMetrics();
			void StyleTextCell(CPositionElement* posElement, const int& zoneWidth, const int& insetLeft,
				const int& insetRight, const wxColour& textColor, const bool& bold,
				const int& alignment);
			int ToRawX(const int& pixels);
			int ToRawY(const int& pixels);
			void RenderChildTree(tree<CTreeData*>::sibling_iterator& parent, RenderMode mode);


			void AddTreeInfos(const wxString& exifKey, const wxString& exifValue, const int& index,
				tree<CTreeData*>::iterator& top, tree<CTreeData*>::iterator& child);


			int yPos;
			//int xMargePos;
			int cxMax;
			//int cyMax;
			int xMaxPos;
			int xMaxPosValue;
			//int xMinPos;
			int rotation;
			wxString filename;
			int widthPosition;

			// Mise en page "moderne" (recalculee par UpdateLayoutMetrics)
			int panelWidth = 0;
			int layoutWidth = 0;
			int labelColumnWidth = 0;
			int valueColumnWidth = 0;
			int chevronWidth = 0;
			int margin = 0;
			int rowHeight = 0;
			wxColour colorRowBack;
			wxColour colorRowBorder;
			wxColour colorTitle;
			wxColour colorLabel;
			wxColour colorValue;
			wxColour colorEmptyValue;
		};
	}
}