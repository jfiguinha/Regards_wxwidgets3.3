#pragma once
#include <WindowMain.h>
#include "LayerElement.h"
using namespace Regards::Window;


namespace Regards
{
	namespace Window
	{
		class CWindowManager;
	}

	namespace Control
	{
		
		class CLayerToolBar;
		class CThumbnailCalqueWnd;

		class CListLayer : public CWindowMain
		{
		public:
			CListLayer(wxWindow* parent, wxWindowID id, wxWindowID frameId);
			~CListLayer() override;
			void UpdateScreenRatio() override;
			void SetLayer(CLayerList * listOfLayer);
			void Resize() override;
			void RefreshList();
			void SetActifLayer(const int& numElement);
			int GetActifLayer();
			int64_t GetActifLayerId();
			vector<int> GetSelectLayer();
			void UpdateLayerList();
		private:

			CLayerList* listOfLayer = nullptr;
			CWindowManager* windowManager;
			CLayerToolBar* layerToolbar;
			CThumbnailCalqueWnd * thumbnailCalqueWnd;
		};
	}
}