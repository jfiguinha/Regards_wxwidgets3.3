#pragma once
#ifndef __NOFACE_DETECTION__

#include <deque>
#include <vector>
#include <TitleBarInterface.h>
#include <WindowMain.h>
#include <ThreadPool.h>
#include <memory>
using namespace Regards::Window;
using namespace std;

class CFaceLoadData;

namespace Regards
{
	namespace Window
	{
		class CWindowManager;
		class CScrollbarWnd;
		class CTitleBar;
	}

	namespace Viewer
	{
		class CThumbnailFacePertinenceToolBar;
		class CThumbnailFace;
		class CThumbnailFaceToolBar;

		class CListFace : public CWindowMain, public CTitleBarInterface
		{
		public:
			CListFace(wxWindow* parent, wxWindowID idCTreeWithScrollbar);
			~CListFace() override;

			// --- Interface CWindowMain / CTitleBarInterface
			void UpdateScreenRatio() override;
			void Resize() override;
			void ClosePane() override;
			void RefreshPane() override;

			// --- Navigation / sélection
			void SetActifItem(const int& numItem, const bool& move);
			void SetActifItem(const wxString& filename, const bool& move);
			int ImageSuivante();
			int ImagePrecedente();
			int GetNumItem();
			wxString GetActifItem();
			wxString GetFilename(const int& numItem);
			std::vector<int> GetFaceSelectID();
			CThumbnailFace* GetThumbnailFace();
			int GetThumbnailHeight();

			// --- Reconnaissance faciale
			void FacialRecognitionReload();

		private:
			// --- Traitements exécutés dans des threads de travail
			static void FacialDetectionRecognition(void* param);
			static void FacialRecognition(void* param);
			static void LoadResource(void* param);

			// --- Traitement idle
			void IdleFunction() override;
			void ProcessIdle() override;
			bool GetProcessEnd() override;

			// --- Construction
			void CreateThumbnailPane(bool checkValidity, int positionTab);
			void CreateZoomToolbar(int positionTab);
			void CreatePertinenceToolbar();
			void ConnectEvents();

			// --- Threads et notifications
			void StartWorker(void (*worker)(void*), const wxString& filename = wxString());
			void NotifyCriteriaChange();
			void RequestThumbnailRefresh();
			void SendStatusBarMessage(int typeMessage, int nbPhoto, int position, int nbElement);
			void UpdateModificationState(bool enable);
			void InitializeListFace();

			// --- Handlers d'événements
			void ThumbnailDatabaseRefresh(wxCommandEvent& event);
			void ThumbnailFolderAdd(wxCommandEvent& event);
			void OnRefreshFolder(wxCommandEvent& event);
			void ThumbnailZoomOn(wxCommandEvent& event);
			void ThumbnailZoomOff(wxCommandEvent& event);
			void ThumbnailZoomPosition(wxCommandEvent& event);
			void ThumbnailRefresh(wxCommandEvent& event);
			void ThumbnailMove(wxCommandEvent& event);
			void OnFacePhotoAdd(wxCommandEvent& event);
			void OnFaceVideoAdd(wxCommandEvent& event);
			void OnResourceLoad(wxCommandEvent& event);

			// --- Fenêtres (propriété : wxWidgets, via le parent)
			CWindowManager* windowManager = nullptr;
			CScrollbarWnd* thumbscrollbar = nullptr;
			CThumbnailFaceToolBar* thumbFaceToolbar = nullptr;
			CThumbnailFacePertinenceToolBar* thumbFacePertinenceToolbar = nullptr;
			CThumbnailFace* thumbnailFace = nullptr;

			// --- État des traitements
			int nbProcessFacePhoto = 0;
			int nbProcessFaceRecognition = 0;
			bool isLoadingResource = false;
			bool resourceLoaded = false;
			bool cleanDatabase = false;
			bool isEnable = true;
			std::unique_ptr<ThreadPool> threadPool;

			// --- Files de travail et compteurs
			std::deque<wxString> listPhoto;
			int nbNbFace = 0;
			int nbTotalImage = 0;
			int nbImageAnalyzed = 0;
			int nbTotalFace = 0;
			int nbFaceRecognized = 0;
		};
	}
}

#endif