#pragma once
#include <TreeControl.h>
#include "EffectParameter.h"
#include <FiltreEffectInterface.h>
#include <FiltreUpdate.h>
#include "FilterWindowParam.h"
using namespace Regards::Window;


namespace Regards::Window
{
	class CTreeDataEffect;

	class CFiltreEffect : public CTreeControl, public IFiltreEffectInterface
	{
	public:
		CFiltreEffect(IFiltreUpdate* bitmapViewer, CTreeElementControlInterface* interfaceControl, bool isVideo,
			int bitmapWindowId);
		~CFiltreEffect(void) = default;
		virtual void Init(CEffectParameter* effectParameter, cv::Mat source, const wxString& filename,
			const int& filtre);
		void SlidePosChange(CTreeElement* treeElement, const int& position, CTreeElementValue* value,
			const wxString& key) override;
		virtual CImageLoadingFormat* ApplyEffect();
		CEffectParameter* GetEffectParameter();
		void UpdateScreenRatio() override;
		void UpdateMousePosition();
		void UpdateSourceBitmap(cv::Mat& source);

		// Largeur disponible du panneau (en pixels). Rappelee automatiquement par BeforeDrawBitmap.
		void SetPanelWidth(const int& width);
		void AddTreeInfos(const wxString& exifKey, CTreeElementValue* position, void* value,
			const int& typeValue = 1, const int& type = TYPE_SLIDE) override;

	protected:
		void MouseOver(wxDC* deviceContext, CPositionElement* element, const int& x, const int& y,
			const int& posLargeur, const int& posHauteur, bool& update) override;
		void ClickOnElement(CPositionElement* element, wxWindow* window, const int& x, const int& y,
			const int& posLargeur, const int& posHauteur) override;
		void UnclickOnElement(CPositionElement* element, wxWindow* window, const int& x, const int& y,
			const int& posLargeur, const int& posHauteur) override;
		void BeforeDrawBitmap(wxDC* deviceContext, const int& width, const int& height,
			const int& posLargeur, const int& posHauteur) override;
		wxColour GetBackgroundColour(const int& yPos) override;
		void AddTreeInfos(const wxString& exifKey, CTreeElementValue* position, void* value, int typeValue,
			const int& index, tree<CTreeData*>::iterator& top, tree<CTreeData*>::iterator& child,
			const int& type);
		//void UpdateElement();
		void UpdateBitmapToViewer(CImageLoadingFormat* bitmap);
		void RenderElement(RenderMode mode);
		void UpdateElement(tree<CTreeData*>::sibling_iterator& parent, RenderMode mode);

		// Mise en page "moderne" (style CInfosFile)
		void UpdateLayoutMetrics();
		int ToRawX(const int& pixels);
		int ToRawY(const int& pixels);
		CTreeElementTriangle* RenderHeaderRow(CTreeData* data, const int& profondeur, RenderMode mode);
		void RenderValueRow(CTreeDataEffect* data, const int& profondeur, RenderMode mode);
		void StyleTextCell(CPositionElement* posElement, const int& zoneWidth, const int& insetLeft,
			const int& insetRight, const wxColour& textColor, const bool& bold,
			const int& alignment);
		void StyleControlCell(CPositionElement* posElement, const int& xPos, const bool& fillColumn,
			const bool& scaledByRatio);

		CPositionElement* RenderSlide(
			CTreeData* data,
			int& xPos,
			int& yPos,
			bool visible,
			RenderMode mode);

		CPositionElement* RenderCheckbox(
			CTreeData* dataEffect,
			int& xPos,
			int& yPos,
			bool visible,
			RenderMode mode);

		CPositionElement* RenderList(
			CTreeData* dataEffect,
			int& xPos,
			int& yPos,
			bool visible,
			RenderMode mode);

		CPositionElement* RenderColor(
			CTreeData* dataEffect,
			int& xPos,
			int& yPos,
			bool visible,
			RenderMode mode);

		CPositionElement* RenderComboBox(
			CTreeData* data,
			int& xPos,
			int& yPos,
			bool visible,
			RenderMode mode);

		int filtre;

		int yPos;
		int cxMax;
		int cyMax;
		int xMaxPos;
		int xMaxPosValue;
		int xMinPos;
		int widthPosition;

		// Mise en page "moderne" (recalculee par UpdateLayoutMetrics)
		int panelWidth = 0;
		int layoutWidth = 0;
		int labelColumnWidth = 0;
		int valueColumnWidth = 0;
		int controlWidth = 0;
		int chevronWidth = 0;
		int margin = 0;
		int rowHeight = 0;
		wxColour colorRowBack;
		wxColour colorRowBorder;
		wxColour colorTitle;
		wxColour colorLabel;

		CEffectParameter* effectParameter = nullptr;
		std::unique_ptr<CFilterWindowParam> filterEffect;

		cv::Mat source;
		IFiltreUpdate* bitmapViewer;

		tree<CTreeData*>::iterator top;
		tree<CTreeData*>::iterator child;

		bool isVideo = false;
		int index;
		int bitmapWindowId;
	};
}