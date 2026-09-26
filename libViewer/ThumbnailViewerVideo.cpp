#include <header.h>
#include "ThumbnailViewerVideo.h"
#include "MainWindow.h"
#include <window_id.h>
using namespace Regards::Viewer;

CThumbnailViewerVideo::CThumbnailViewerVideo(wxWindow* parent, wxWindowID id, const CThemeThumbnail& themeThumbnail,
	const bool& testValidity)
	: CThumbnailVideo(parent, id, themeThumbnail, testValidity)
{
	idWindowToRefresh = THUMBNAILVIDEOWINDOW;
	moveOnPaint = false;
}

void CThumbnailViewerVideo::OnScrollBarH(wxCommandEvent& event)
{
	const int isScrollBarH = event.GetInt();
	const long scrollBarHSize = event.GetExtraLong();

	if (isScrollBarH)
		themeThumbnail.themeIcone.SetHeight(themeIconeHeight);
	else
		themeThumbnail.themeIcone.SetHeight(themeIconeHeight + scrollBarHSize);

	ResizeThumbnail();
}

void CThumbnailViewerVideo::OnPictureClick(const int& numPhotoId)
{
	auto mainWindow = static_cast<CMainWindow*>(this->FindWindowById(MAINVIEWERWINDOWID));
	if (mainWindow == nullptr)
		return; // Sécurité : Fenêtre principale introuvable

	CIcone* icone = GetIconeById(numPhotoId);
	if (icone != nullptr && icone->GetPtData() != nullptr) // Sécurité anti-crash
	{
#ifdef FFMPEG
		int timePosition = icone->GetPtData()->GetTimePosition();
#else
		int timePosition = icone->GetPtData()->GetTimePosition() * 1000;
#endif
		wxCommandEvent evt(wxEVENT_PICTUREVIDEOCLICK);
		evt.SetExtraLong(timePosition);
		mainWindow->GetEventHandler()->AddPendingEvent(evt);
	}
	else
	{
		wxLogError("CThumbnailViewerVideo::OnPictureClick - Vignette ou donnees introuvables pour l'ID %d", numPhotoId);
	}
}
