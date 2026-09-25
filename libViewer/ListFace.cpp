#include <header.h>

#ifndef __NOFACE_DETECTION__
#include "ListFace.h"
#include <ParamInit.h>
#include "ViewerParam.h"
#include "ViewerParamInit.h"
#include "MainTheme.h"
#include <libPicture.h>
#include "MainThemeInit.h"
#include <MoveFaceDialog.h>
#include <SqlFacePhoto.h>
#include <SQLRemoveData.h>
#include <DeepLearning.h>
#include <ThumbnailMessage.h>
#include <ImageLoadingFormat.h>
#include <ScrollbarWnd.h>
#include "ThumbnailFace.h"
#include "ThumbnailFaceToolBar.h"
#include "ThumbnailFacePertinenceToolBar.h"
#include "LibResource.h"
#include <SqlFaceRecognition.h>
#include <SqlFindFacePhoto.h>
#include <RegardsConfigParam.h>
#include <wx/progdlg.h>
#include <FFmpegVideoThumb.h>
#include <appcontext.h>

#include <algorithm>
#include <chrono>
#include <memory>
#include <thread>
#include <vector>

extern AppContext application_context;

using namespace Regards::Picture;
using namespace Regards::Sqlite;
using namespace Regards::Window;
using namespace Regards::Video;
using namespace Regards::Viewer;
using namespace Regards::DeepLearning;

namespace
{
	// Pause appliquée dans le thread appelant après le lancement d'un traitement.
	constexpr std::chrono::milliseconds kThrottleDelay{ 50 };

	std::vector<int> ZoomValues()
	{
		return { 100, 150, 200, 250, 300, 350, 400, 450, 500, 550, 600 };
	}

	std::vector<int> PertinenceValues()
	{
		return { 60, 70, 80, 90, 100 };
	}

	// Données échangées entre le thread UI et un thread de travail.
	// Créée par CListFace::StartWorker, libérée par le handler d'événement final
	// (OnResourceLoad / OnFacePhotoAdd).
	struct CThreadFace
	{
		CThreadFace() = default;
		CThreadFace(const CThreadFace&) = delete;
		CThreadFace& operator=(const CThreadFace&) = delete;

		wxString filename;
		std::future<void> task;
		wxWindow* mainWindow = nullptr;
		int nbFace = 0;
		int type = 0; // 0 : détection sur photo/vidéo, 1 : reconnaissance
	};

	// Reprend la propriété des données transmises par un événement et attend la fin de la tâche.
	std::unique_ptr<CThreadFace> TakeThreadData(wxCommandEvent& event)
	{
		std::unique_ptr<CThreadFace> data(static_cast<CThreadFace*>(event.GetClientData()));
		if (data && data->task.valid())
			data->task.get();
		return data;
	}

	// Poste l'événement de fin de traitement vers la fenêtre propriétaire (thread de travail).
	void PostToOwner(CThreadFace* data, const wxEventType eventType)
	{
		if (data->mainWindow == nullptr)
			return;

		wxCommandEvent evt(eventType);
		evt.SetClientData(data);
		data->mainWindow->GetEventHandler()->AddPendingEvent(evt);
	}

	// Détection sur une photo. Retourne le nombre de visages trouvés.
	int DetectFacesInPicture(CLibPicture& libPicture, const wxString& filename, const bool fastDetection)
	{
		bool pictureOK = false;
		std::unique_ptr<CImageLoadingFormat> pictureData(libPicture.LoadPictureToBGRA(filename, pictureOK));
		if (!pictureOK || !pictureData)
			return 0;

		pictureData->SetFilename(filename);
		const std::vector<int> listFace = CDeepLearning::FindFace(pictureData->GetMatImage().clone(), filename,
			fastDetection);
		return static_cast<int>(listFace.size());
	}

	// Détection frame par frame sur une vidéo. Chaque frame contenant des visages
	// est signalée à l'UI (wxEVENT_FACEVIDEOADD, nombre de visages dans GetInt()).
	// Retourne le nombre de visages de la DERNIÈRE frame (comportement historique conservé).
	int DetectFacesInVideo(CThreadFace* data, const bool fastDetection)
	{
		CFFmpegVideoThumb video(data->filename);
		CImageLoadingFormat pictureData;
		int nbFace = 0;

		const int totalFrame = video.GetTotalFrame();
		for (int i = 0; i < totalFrame; i++)
		{
			// Remis à zéro à chaque frame : une frame vide ne doit pas réutiliser les visages précédents.
			std::vector<int> listFace;
			nbFace = 0;

			cv::Mat frame = video.GetVideoFrame();
			if (!frame.empty())
			{
				pictureData.SetPicture(frame);
				pictureData.ConvertToBGR();
				pictureData.Flip();
				pictureData.SetFilename(data->filename);
				pictureData.SetOrientation(0);
				listFace = CDeepLearning::FindFace(frame, data->filename, fastDetection);
				nbFace = static_cast<int>(listFace.size());
			}

			CSqlFacePhoto facePhoto;
			for (int numFace : listFace)
			{
				facePhoto.UpdateVideoFace(numFace, i);
			}

			if (nbFace > 0 && data->mainWindow != nullptr)
			{
				wxCommandEvent evt(wxEVENT_FACEVIDEOADD);
				evt.SetInt(nbFace);
				data->mainWindow->GetEventHandler()->AddPendingEvent(evt);
			}
		}

		return nbFace;
	}
}

CListFace::CListFace(wxWindow* parent, wxWindowID id)
	: CWindowMain("CListFace", parent, id)
{
	threadPool = std::make_unique<ThreadPool>();

	CMainParam* config = CMainParamInit::getInstance();
	CMainTheme* viewerTheme = CMainThemeInit::getInstance();

	if (viewerTheme != nullptr && config != nullptr)
	{
		int positionTab = 3;
		config->GetSlideFacePos(positionTab);
		const bool checkValidity = config->GetCheckThumbnailValidity();

		CThemeSplitter theme;
		viewerTheme->GetSplitterTheme(&theme);
		windowManager = new CWindowManager(this, wxID_ANY, theme);

		CreateThumbnailPane(checkValidity, positionTab);
		CreateZoomToolbar(positionTab);
		CreatePertinenceToolbar();
	}

	ConnectEvents();

	processIdle = true;
	listProcessWindow.push_back(this);
}

CListFace::~CListFace()
{
	if (thumbnailFace != nullptr)
	{
		CMainParam* config = CMainParamInit::getInstance();
		if (config != nullptr)
			config->SetSlideFacePos(thumbnailFace->GetTabValue());
	}

	threadPool.reset();
}

//---------------------------------------------------------------------------------------
// Construction de l'interface
//---------------------------------------------------------------------------------------
void CListFace::CreateThumbnailPane(const bool checkValidity, const int positionTab)
{
	CMainTheme* viewerTheme = CMainThemeInit::getInstance();
	if (viewerTheme == nullptr)
		return;

	CThemeThumbnail themeThumbnail;
	viewerTheme->GetThumbnailTheme(&themeThumbnail);

	std::vector<int> valueZoom = ZoomValues();
	thumbnailFace = new CThumbnailFace(windowManager, THUMBNAILFACE, themeThumbnail, checkValidity);
	thumbscrollbar = new CScrollbarWnd(windowManager, thumbnailFace, wxID_ANY);
	thumbscrollbar->ShowVerticalScroll();
	thumbnailFace->SetNoVScroll(false);
	thumbnailFace->SetCheck(true);
	thumbnailFace->ChangeTabValue(valueZoom, positionTab);
	thumbnailFace->init();

	wxRect rect;
	windowManager->AddWindow(thumbscrollbar, Pos::wxCENTRAL, false, 0, rect, wxID_ANY, false);
}

void CListFace::CreateZoomToolbar(const int positionTab)
{
	CMainTheme* viewerTheme = CMainThemeInit::getInstance();
	if (viewerTheme == nullptr)
		return;

	CThemeToolbar theme;
	viewerTheme->GetBitmapToolbarTheme(&theme);

	std::vector<int> valueZoom = ZoomValues();
	thumbFaceToolbar = new CThumbnailFaceToolBar(windowManager, wxID_ANY, theme, false);
	thumbFaceToolbar->SetTabValue(valueZoom);
	thumbFaceToolbar->SetTrackBarPosition(positionTab - 1);

	wxRect rect;
	windowManager->AddWindow(thumbFaceToolbar, Pos::wxBOTTOM, true, thumbFaceToolbar->GetHeight(), rect, wxID_ANY,
		false);
}

void CListFace::CreatePertinenceToolbar()
{
	CMainParam* config = CMainParamInit::getInstance();
	CMainTheme* viewerTheme = CMainThemeInit::getInstance();
	if (config == nullptr || viewerTheme == nullptr)
		return;

	std::vector<int> values = PertinenceValues();
	const int pertinenceValue = static_cast<int>(config->GetPertinenceValue());

	int position = 2;
	const auto it = std::find(values.begin(), values.end(), pertinenceValue);
	if (it != values.end())
		position = static_cast<int>(std::distance(values.begin(), it));

	CThemeToolbar theme;
	viewerTheme->GetThumbnailToolbarTheme(theme);

	thumbFacePertinenceToolbar = new CThumbnailFacePertinenceToolBar(windowManager, wxID_ANY, theme, false);
	thumbFacePertinenceToolbar->SetTabValue(values);
	thumbFacePertinenceToolbar->SetTrackBarPosition(position);

	wxRect rect;
	windowManager->AddWindow(thumbFacePertinenceToolbar, Pos::wxTOP, true, thumbFacePertinenceToolbar->GetHeight(),
		rect, wxID_ANY, false);
}

void CListFace::ConnectEvents()
{
	Connect(wxEVENT_RESOURCELOAD, wxCommandEventHandler(CListFace::OnResourceLoad));
	Connect(wxEVENT_FACEVIDEOADD, wxCommandEventHandler(CListFace::OnFaceVideoAdd));
	Connect(wxEVENT_FACEPHOTOADD, wxCommandEventHandler(CListFace::OnFacePhotoAdd));
	Connect(wxEVENT_REFRESHFOLDER, wxCommandEventHandler(CListFace::OnRefreshFolder));
	Connect(wxEVENT_THUMBNAILZOOMON, wxCommandEventHandler(CListFace::ThumbnailZoomOn));
	Connect(wxEVENT_THUMBNAILZOOMOFF, wxCommandEventHandler(CListFace::ThumbnailZoomOff));
	Connect(wxEVENT_THUMBNAILZOOMPOSITION, wxCommandEventHandler(CListFace::ThumbnailZoomPosition));
	Connect(wxEVENT_THUMBNAILREFRESH, wxCommandEventHandler(CListFace::ThumbnailRefresh));
	Connect(wxEVENT_THUMBNAILMOVE, wxCommandEventHandler(CListFace::ThumbnailMove));
	Connect(wxEVENT_THUMBNAILFOLDERADD, wxCommandEventHandler(CListFace::ThumbnailFolderAdd));
	Connect(wxEVENT_THUMBNAILREFRESHFACE, wxCommandEventHandler(CListFace::ThumbnailDatabaseRefresh));
}

//---------------------------------------------------------------------------------------
// Accesseurs
//---------------------------------------------------------------------------------------
CThumbnailFace* CListFace::GetThumbnailFace()
{
	return thumbnailFace;
}

std::vector<int> CListFace::GetFaceSelectID()
{
	if (thumbnailFace == nullptr)
		return {};
	return thumbnailFace->GetFaceSelectID();
}

wxString CListFace::GetActifItem()
{
	if (thumbnailFace == nullptr)
		return wxString();
	return thumbnailFace->GetActifItem();
}

wxString CListFace::GetFilename(const int& numItem)
{
	if (thumbnailFace == nullptr)
		return wxString();
	return thumbnailFace->GetFilename(numItem);
}

int CListFace::GetNumItem()
{
	if (thumbnailFace == nullptr)
		return -1;
	return thumbnailFace->GetNumItem();
}

int CListFace::ImageSuivante()
{
	if (thumbnailFace == nullptr)
		return -1;
	return thumbnailFace->ImageSuivante();
}

int CListFace::ImagePrecedente()
{
	// Valeur par défaut historique : 0 (et non -1 comme ImageSuivante).
	if (thumbnailFace == nullptr)
		return 0;
	return thumbnailFace->ImagePrecedente();
}

int CListFace::GetThumbnailHeight()
{
	if (thumbnailFace == nullptr || thumbscrollbar == nullptr)
		return 0;
	return thumbnailFace->GetIconeHeight() + thumbscrollbar->GetBarHeight();
}

void CListFace::SetActifItem(const int& numItem, const bool& move)
{
	if (thumbnailFace != nullptr)
		thumbnailFace->SetActifItem(numItem, move);
}

void CListFace::SetActifItem(const wxString& filename, const bool& move)
{
	if (thumbnailFace != nullptr)
		thumbnailFace->SetActifItem(filename, move);
}

//---------------------------------------------------------------------------------------
// Interface CWindowMain / CTitleBarInterface
//---------------------------------------------------------------------------------------
void CListFace::ClosePane()
{}

void CListFace::RefreshPane()
{
	// Suppression de toutes les faces
	if (isLoadingResource)
		return;

	const wxString title = CLibResource::LoadStringFromResource(L"LBLSTOPALLPROCESS", 1);
	const wxString message = CLibResource::LoadStringFromResource(L"LBLSTOPPROCESS", 1);
	StopAllProcess(title, message, this);

	// Suppression de toutes les données de faces
	CSQLRemoveData::DeleteFaceDatabase();

	SetStopProcess(false);

	processIdle = true;
}

void CListFace::UpdateScreenRatio()
{
	if (windowManager != nullptr)
		windowManager->UpdateScreenRatio();
}

void CListFace::Resize()
{
	if (windowManager != nullptr)
	{
		windowManager->SetSize(GetWindowWidth(), GetWindowHeight());
		needToRefresh = true;
	}
}

//---------------------------------------------------------------------------------------
// Threads et notifications
//---------------------------------------------------------------------------------------
void CListFace::StartWorker(void (*worker)(void*), const wxString& filename)
{
	auto data = std::make_unique<CThreadFace>();
	data->mainWindow = this;
	data->filename = filename;
	CThreadFace* workerData = data.get();
	data->task = threadPool->Enqueue([worker, workerData]()
	{
		worker(workerData);
	});

	// La propriété passe au handler d'événement qui recevra le résultat du thread.
	data.release();
}

void CListFace::NotifyCriteriaChange()
{
	wxWindow* mainWnd = FindWindowById(MAINVIEWERWINDOWID);
	if (mainWnd != nullptr)
		wxQueueEvent(mainWnd, new wxCommandEvent(wxEVT_CRITERIACHANGE));
}

void CListFace::RequestThumbnailRefresh()
{
	wxCommandEvent evt(wxEVENT_THUMBNAILREFRESH);
	GetEventHandler()->AddPendingEvent(evt);
}

void CListFace::SendStatusBarMessage(const int typeMessage, const int nbPhoto, const int position,
	const int nbElement)
{
	wxWindow* mainWnd = FindWindowById(MAINVIEWERWINDOWID);
	if (mainWnd == nullptr)
		return;

	// Le CThumbnailMessage est libéré par le destinataire de l'événement.
	auto thumbnailMessage = new CThumbnailMessage();
	thumbnailMessage->nbPhoto = nbPhoto;
	thumbnailMessage->thumbnailPos = position;
	thumbnailMessage->nbElement = nbElement;
	thumbnailMessage->typeMessage = typeMessage;

	wxCommandEvent eventChange(wxEVENT_UPDATESTATUSBARMESSAGE);
	eventChange.SetClientData(thumbnailMessage);
	mainWnd->GetEventHandler()->AddPendingEvent(eventChange);
}

void CListFace::UpdateModificationState(const bool enable)
{
	if (thumbnailFace != nullptr)
		thumbnailFace->EnableModification(enable);
	if (thumbFaceToolbar != nullptr)
		thumbFaceToolbar->EnableModification(enable);
	if (thumbFacePertinenceToolbar != nullptr)
		thumbFacePertinenceToolbar->EnableModification(enable);
}

void CListFace::InitializeListFace()
{
	// Mise à jour de la liste des photos à traiter
	CSqlFacePhoto facePhoto;
	const auto photos = facePhoto.GetPhotoListTreatment();
	listPhoto.assign(photos.begin(), photos.end());
	nbTotalImage = static_cast<int>(listPhoto.size());
	nbImageAnalyzed = 0;

	CSqlFindFacePhoto faceRecognition;
	nbNbFace = faceRecognition.GetNbListFaceToRecognize();
	nbTotalFace = nbNbFace;

	nbFaceRecognized = 0;
}

//---------------------------------------------------------------------------------------
// Handlers d'événements
//---------------------------------------------------------------------------------------
void CListFace::OnResourceLoad(wxCommandEvent& event)
{
	// Attend la fin du thread puis libère les données
	TakeThreadData(event).reset();

	processIdle = true;
	isLoadingResource = false;
	resourceLoaded = true;

	InitializeListFace();
}

void CListFace::OnFaceVideoAdd(wxCommandEvent& event)
{
	// Le nombre de visages est transmis par l'événement : les données du thread
	// (CThreadFace) sont encore utilisées par celui-ci et ne doivent pas être lues ici.
	if (event.GetInt() > 0)
	{
		NotifyCriteriaChange();
		RequestThumbnailRefresh();
	}
}

void CListFace::OnFacePhotoAdd(wxCommandEvent& event)
{
	int nbFace = 0;
	int type = 0;

	if (auto path = TakeThreadData(event))
	{
		type = path->type;
		nbFace = path->nbFace;

		if (nbFace > 0)
			NotifyCriteriaChange();
	}

	if (type == 0)
	{
		nbProcessFacePhoto--;
		nbImageAnalyzed++;
		nbNbFace += nbFace;
		nbTotalFace += nbFace;
		SendStatusBarMessage(4, nbTotalImage - nbImageAnalyzed, nbImageAnalyzed, nbTotalImage);
	}
	else
	{
		nbProcessFaceRecognition--;
		nbNbFace--;
		nbFaceRecognized++;
		SendStatusBarMessage(5, nbNbFace, nbFaceRecognized, nbTotalFace);
	}

	if (nbFace > 0)
		RequestThumbnailRefresh();

	processIdle = true;
}

void CListFace::ThumbnailFolderAdd(wxCommandEvent& /*event*/)
{
	processIdle = true;
}

void CListFace::OnRefreshFolder(wxCommandEvent& /*event*/)
{
	InitializeListFace();
}

void CListFace::ThumbnailRefresh(wxCommandEvent& /*event*/)
{
	cleanDatabase = true;
	if (thumbnailFace != nullptr)
	{
		// AU LIEU DE : thumbnailFace->init();
		// ON FAIT :
		thumbnailFace->SyncWithDatabase();
	}
	processIdle = true;
}


void CListFace::ThumbnailDatabaseRefresh(wxCommandEvent& /*event*/)
{
	cleanDatabase = true;
	RefreshPane();
	InitializeListFace();
	if (thumbnailFace != nullptr)
	{
		thumbnailFace->EraseData();
		thumbnailFace->init();
	}
	processIdle = true;
	needToRefresh = true;
}

void CListFace::ThumbnailMove(wxCommandEvent& /*event*/)
{
	if (thumbnailFace == nullptr)
		return;

	if (thumbnailFace->GetFaceSelectID().empty())
	{
		wxMessageBox("No picture selected", "Informations");
		return;
	}

	// Choix de la Face
	MoveFaceDialog moveFaceDialog(this->GetParent());
	moveFaceDialog.ShowModal();
	if (moveFaceDialog.IsOk())
	{
		thumbnailFace->MoveFace(moveFaceDialog.GetFaceNameSelected());
		processIdle = true;
		needToRefresh = true;
	}
}

void CListFace::ThumbnailZoomOn(wxCommandEvent& /*event*/)
{
	if (thumbnailFace != nullptr)
		thumbnailFace->ZoomOn();
}

void CListFace::ThumbnailZoomOff(wxCommandEvent& /*event*/)
{
	if (thumbnailFace != nullptr)
		thumbnailFace->ZoomOff();
}

void CListFace::ThumbnailZoomPosition(wxCommandEvent& event)
{
	if (thumbnailFace != nullptr)
		thumbnailFace->ZoomPosition(event.GetExtraLong());
}

//---------------------------------------------------------------------------------------
// Fonctions exécutées dans les threads de travail
//---------------------------------------------------------------------------------------
void CListFace::LoadResource(void* param)
{
	auto path = static_cast<CThreadFace*>(param);

	bool openCLCompatible = false;
	bool cudaCompatible = false;
	CRegardsConfigParam* config = CParamInit::getInstance();
	if (config != nullptr)
	{
		openCLCompatible = config->GetIsOpenCLSupport();
		cudaCompatible = config->GetIsCudaSupport();
	}

	CDeepLearning::LoadRessource(openCLCompatible, cudaCompatible);

	PostToOwner(path, wxEVENT_RESOURCELOAD);
}

// Reconnaissance d'UNE face par appel (la première de la liste à reconnaître).
void CListFace::FacialDetectionRecognition(void* param)
{
	auto path = static_cast<CThreadFace*>(param);

	bool fastDetection = true;
	CRegardsConfigParam* config = CParamInit::getInstance();
	if (config != nullptr)
		fastDetection = config->GetFastDetectionFace();

	CSqlFindFacePhoto facePhoto;
	const auto listFace = facePhoto.GetListFaceToRecognize();

	if (!listFace.empty())
		CDeepLearning::FindFaceCompatible(listFace.front(), fastDetection);

	path->nbFace = static_cast<int>(listFace.size());
	path->type = 1;
	PostToOwner(path, wxEVENT_FACEPHOTOADD);
}

// Détection des visages d'une photo ou d'une vidéo.
void CListFace::FacialRecognition(void* param)
{
	auto path = static_cast<CThreadFace*>(param);
	CLibPicture libPicture;

	bool fastDetection = true;
	int faceVideoDetection = 0;
	CRegardsConfigParam* config = CParamInit::getInstance();
	if (config != nullptr)
	{
		fastDetection = config->GetFastDetectionFace();
		faceVideoDetection = config->GetFaceVideoDetection();
	}

	if (faceVideoDetection && libPicture.TestIsVideo(path->filename))
		path->nbFace = DetectFacesInVideo(path, fastDetection);
	else
		path->nbFace = DetectFacesInPicture(libPicture, path->filename, fastDetection);

	path->type = 0;
	PostToOwner(path, wxEVENT_FACEPHOTOADD);
}

//---------------------------------------------------------------------------------------
// Reconnaissance faciale complète (thread UI, dialogue modal)
//---------------------------------------------------------------------------------------
void CListFace::FacialRecognitionReload()
{
	bool fastDetection = true;
	int nbProcesseur = 1;
	CRegardsConfigParam* config = CParamInit::getInstance();
	if (config != nullptr)
	{
		fastDetection = config->GetFastDetectionFace();
		nbProcesseur = config->GetFaceProcess();
	}

	if (nbProcessFaceRecognition >= nbProcesseur)
		return;

	CSqlFaceRecognition faceRecognition;
	faceRecognition.DeleteFaceRecognitionDatabase();

	CSqlFindFacePhoto facePhoto;
	const auto listFace = facePhoto.GetListFaceToRecognize();
	wxProgressDialog dialog("Face Recognition", "", static_cast<int>(listFace.size()), nullptr,
		wxPD_APP_MODAL | wxPD_CAN_ABORT | wxPD_AUTO_HIDE);

	int i = 0;
	for (int numFace : listFace)
	{
		const wxString text = wxString::Format("Face number : %d", i + 1);
		CDeepLearning::FindFaceCompatible(numFace, fastDetection);
		if (!dialog.Update(++i, text))
			break;
	}

	// Mise à jour des critères
	if (!listFace.empty())
	{
		NotifyCriteriaChange();
		RequestThumbnailRefresh();
	}

	std::this_thread::sleep_for(kThrottleDelay);
}

//---------------------------------------------------------------------------------------
// Traitement idle
//---------------------------------------------------------------------------------------
void CListFace::IdleFunction()
{
	if (endProgram)
	{
		processIdle = false;
	}
}

bool CListFace::GetProcessEnd()
{
	return nbProcessFacePhoto <= 0 && nbProcessFaceRecognition <= 0;
}

void CListFace::ProcessIdle()
{
	CRegardsConfigParam* config = CParamInit::getInstance();
	if (config == nullptr)
	{
		processIdle = false;
		return;
	}

	// Attente de l'initialisation d'OpenCL
	if (!application_context.isOpenCLInitialized && config->GetIsOpenCLSupport() && !config->GetIsUseCuda())
	{
		processIdle = true;
		return;
	}

	// Chargement des ressources (réseaux de neurones) dans un thread dédié
	if (!isLoadingResource && !resourceLoaded)
	{
		isLoadingResource = true;
		StartWorker(LoadResource);
		return;
	}
	if (isLoadingResource)
	{
		processIdle = true;
		return;
	}

	if (!config->GetFaceDetection())
	{
		processIdle = false;
		return;
	}

	const int nbProcesseur = config->GetFaceProcess();

	// Détection des visages : une photo par passage tant qu'il reste des processeurs disponibles
	const size_t nbPhotoAtStart = listPhoto.size();
	bool photoLaunched = false;

	if (nbProcessFacePhoto < nbProcesseur && nbPhotoAtStart > 0)
	{
		const wxString filename = listPhoto.front();

		CSqlFacePhoto sqlFacePhoto;
		sqlFacePhoto.InsertFaceTreatment(filename);

		StartWorker(FacialRecognition, filename);
		nbProcessFacePhoto++;
		photoLaunched = true;

		listPhoto.pop_front();

		std::this_thread::sleep_for(kThrottleDelay);
	}

	if (photoLaunched)
		SendStatusBarMessage(4, nbTotalImage - nbImageAnalyzed, nbImageAnalyzed, nbTotalImage);

	// Reconnaissance des visages : une face par passage
	const int nbFaceLocal = nbNbFace;

	if (nbProcessFaceRecognition == 0 && nbFaceLocal > 0)
	{
		StartWorker(FacialDetectionRecognition);
		nbProcessFaceRecognition++;

		if (cleanDatabase)
		{
			CDeepLearning::CleanRecognition();
			cleanDatabase = false;
		}

		SendStatusBarMessage(5, nbNbFace, nbFaceRecognized, nbTotalFace);
	}

	// Fin de traitement ?
	const bool allDone = (nbPhotoAtStart == 0 && nbFaceLocal == 0);
	if (allDone)
	{
		processIdle = false;
		SendStatusBarMessage(5, nbNbFace, nbFaceRecognized, nbTotalFace);

		// AU LIEU DE : thumbnailFace->init();
		// ON FAIT :
		if (thumbnailFace != nullptr)
			thumbnailFace->SyncWithDatabase();
	}

	isEnable = allDone;
	UpdateModificationState(isEnable);
}

#endif