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
			int GetNumItem();
			void SetActifItem(const int& numItem, const bool& move);
			//void SetFilename(const wxString& filename);
			void SetLayer(CLayerList * listOfLayer);
			void Resize() override;
			void RefreshList();

		private:

			CLayerList* listOfLayer = nullptr;
			CWindowManager* windowManager;
			CLayerToolBar* layerToolbar;
			CThumbnailCalqueWnd * thumbnailCalqueWnd;
		};
	}
}