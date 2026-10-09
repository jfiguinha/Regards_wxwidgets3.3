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
			int GetActifLayer();
			vector<int> GetSelectLayer();

		private:

			CLayerList* listOfLayer = nullptr;
			CWindowManager* windowManager;
			CLayerToolBar* layerToolbar;
			CThumbnailCalqueWnd * thumbnailCalqueWnd;
		};
	}
}