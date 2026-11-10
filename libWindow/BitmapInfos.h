#pragma once
#include "WindowMain.h"
#include <functional>
//#include <FileGeolocation.h>
//using namespace Regards::Internet;
namespace Regards
{
	namespace Internet
	{
		class CFileGeolocation;
	}

	namespace Window
	{
		class CBitmapInfos : public CWindowMain
		{
		public:
			CBitmapInfos(wxWindow* parent, wxWindowID id, const CThemeBitmapInfos& theme);
			~CBitmapInfos() override;
			void UpdateData();
			void SetFilename(const wxString& filename);
			int GetHeight() override;
			void Redraw();
			void UpdateScreenRatio() override;
			bool GetProcessEnd() override;
			// Action du bouton plein ecran (si absente : bascule la fenetre de premier niveau)
			void SetFullscreenCallback(std::function<void()> callback);

		private:

			void OnStartGps(wxTimerEvent& event);
			void UpdateGpsInfosLocal(wxCommandEvent& event);
			static void GetGpsInfos(void* data);

			wxString GenerateDefaultTimeStamp();
			void OnUpdateGpsInfos(wxCommandEvent& event);
			void SetDateInfos(const wxString& dataInfos, char seperator);
			void on_paint(wxPaintEvent& event);
			void OnHeaderMouseMove(wxMouseEvent& event);
			void OnHeaderMouseLeave(wxMouseEvent& event);
			void OnHeaderLButtonDown(wxMouseEvent& event);
			void UpdateFullscreenRect();
			void DrawFullscreenButton(wxDC* dc);
			int Dayofweek(int d, int m, int y);
			void DrawInformations(wxDC* dc);
			wxString filename;
			wxString gpsInfos;
			wxString dateInfos;
			bool gpsInfosUpdate;
			wxRect fullscreenRect;
			bool fullscreenHover = false;
			std::function<void()> fullscreenCallback;

			std::unique_ptr<std::thread> threadGps = nullptr;
			CThemeBitmapInfos bitmapInfosTheme;
			std::unique_ptr<wxTimer> gpsTimer;
		};
	}
}