#pragma once
#include <WindowMain.h>
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
			CListLayer(wxWindow* parent, wxWindowID id);
			~CListLayer() override;
			void UpdateScreenRatio() override;
			int GetNumItem();
			void SetActifItem(const int& numItem, const bool& move);
			void SetFilename(const wxString& filename);

			void Resize() override;

		private:

			void DeleteLayer(wxCommandEvent& event);
			void CopyLayer(wxCommandEvent& event);
			void CreateLayer(wxCommandEvent& event);


			CWindowManager* windowManager;
			CLayerToolBar* layerToolbar;
			CThumbnailCalqueWnd * thumbnailCalqueWnd;
		};
	}
}