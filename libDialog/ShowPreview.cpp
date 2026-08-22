#include <header.h>
#include "ShowPreview.h"
#if defined(__WXMSW__)
#include "../include/window_id.h"
#else
#include <window_id.h>
#endif
#include <wx/filename.h>
#include <libPicture.h>
#include <ParamInit.h>
#include <BitmapWnd3d.h>
#include <BitmapWndRender.h>
#include <ImageLoadingFormat.h>
#include <videothumb.h>
#include <FileUtility.h>
#include "CompressionAudioVideoOption.h"
#include "ffmpeg_transcoding.h"
#include <appcontext.h>
extern AppContext application_context;

using namespace Regards::Picture;
using namespace Regards::Window;
using namespace Regards::Control;
using namespace Regards::Video;



void CShowPreview::UpdateScreenRatio()
{
	scrollbar->UpdateScreenRatio();
	previewToolbar->UpdateScreenRatio();
	bitmapWindow->UpdateScreenRatio();
	this->Resize();
}

CShowPreview::CShowPreview(wxWindow* parent, wxWindowID id, CThemeParam* config, CVideoOptionCompress* videoOption)
	: CWindowMain("ShowBitmap", parent, id)
{
	transitionEnd = false;
	scrollbar = nullptr;
	previewToolbar = nullptr;
	bitmapWindow = nullptr;
	configRegards = nullptr;
	defaultToolbar = true;
	defaultViewer = true;
	this->videoOption = videoOption;

	CThemeBitmapWindow themeBitmap;
	configRegards = CParamInit::getInstance();
	CThemeScrollBar themeScroll;
	CThemeToolbar themeToolbar;
	CThemeSlider themeSlider;
	std::vector<int> value = {
		1, 2, 3, 4, 5, 6, 8, 12, 16, 25, 33, 50, 66, 75, 100, 133, 150, 166, 200, 300, 400, 500, 600, 700, 800, 1200,
		1600
	};


	if (config != nullptr)
	{
		config->GetBitmapToolbarTheme(&themeToolbar);
	}

	previewToolbar = nullptr;

	previewToolbar = std::make_unique<CPreviewToolbar>(this, wxID_ANY, BITMAPWINDOWVIEWERIDDLG, themeToolbar, false);
	previewToolbar->SetTabValue(value);

	if (config != nullptr)
		config->GetBitmapWindowTheme(&themeBitmap);

	themeBitmap.colorScreen = wxColour("black");

	bitmapWindow = std::make_unique<CBitmapWndRender>(previewToolbar.get(), 0, themeBitmap);
	bitmapWindowRender = std::make_unique<CBitmapWnd3D>(this, BITMAPWINDOWVIEWERIDDLG);
	bitmapWindowRender->SetBitmapRenderInterface(bitmapWindow.get());
	bitmapWindow->SetTabValue(value);
	bitmapWindow->SetPreview(1);
	if (config != nullptr)
		config->GetScrollTheme(&themeScroll);

	scrollbar = std::make_unique<CScrollbarWnd>(this, bitmapWindowRender.get(), wxID_ANY, "BitmapScroll");

	if (config != nullptr)
	{
		config->GetVideoSliderTheme(&themeSlider);
	}

	sliderVideo = std::make_unique<CSliderVideoPreview>(this, wxID_ANY, this, themeSlider);

	Connect(wxEVT_BITMAPZOOMIN, wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler(CShowPreview::OnViewerZoomIn));
	Connect(wxEVT_BITMAPZOOMOUT, wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler(CShowPreview::OnViewerZoomOut));
	Connect(wxEVENT_MOVELEFT, wxCommandEventHandler(CShowPreview::OnMoveLeft));
	Connect(wxEVENT_MOVERIGHT, wxCommandEventHandler(CShowPreview::OnMoveRight));
	Connect(wxEVENT_MOVETOP, wxCommandEventHandler(CShowPreview::OnMoveTop));
	Connect(wxEVENT_MOVEBOTTOM, wxCommandEventHandler(CShowPreview::OnMoveBottom));
	Connect(wxEVENT_SETCONTROLSIZE, wxCommandEventHandler(CShowPreview::OnControlSize));
	Connect(wxEVENT_SETPOSITION, wxCommandEventHandler(CShowPreview::OnSetPosition));

	Connect(wxEVENT_SHOWORIGINAL, wxCommandEventHandler(CShowPreview::OnShowOriginal));
	Connect(wxEVENT_SHOWNEW, wxCommandEventHandler(CShowPreview::OnShowNew));
	Connect(wxEVENT_CONTEXTCREATE, wxCommandEventHandler(CShowPreview::OnContextCreate));
	Connect(wxEVENT_UPDATEPICTURE, wxCommandEventHandler(CShowPreview::OnUpdatePicture));

	wxString decoder = "";
	progressValue = 0;

	wxString resourcePath = CFileUtility::GetResourcesFolderPath();
	
}


void CShowPreview::SetParameter(const wxString& videoFilename)
{
	isFirstPicture = true;
	wxString decoder = "";

	progressValue = 0;
	filename = videoFilename;

	CVideoThumb video(filename);
	timeTotal = video.GetMovieDuration();
	orientation = video.GetOrientation();
	sliderVideo->SetTotalSecondTime(timeTotal * 1000);

	MoveSlider(0);
}

void CShowPreview::SetBitmapToViewer(CImageLoadingFormat* bitmap, bool isUpdate)
{
	wxCommandEvent* event = nullptr;
	if(isUpdate)
		event = new wxCommandEvent(wxEVENT_UPDATEBITMAP);
	else
		event = new wxCommandEvent(wxEVENT_SETBITMAP);
	event->SetClientData(bitmap);
	wxQueueEvent(bitmapWindowRender.get(), event);
}

void CShowPreview::ShowPicture(cv::Mat& bitmap, const wxString& label)
{
	if (!bitmap.empty())
	{
		auto imageLoadingFormat = new CImageLoadingFormat();
		imageLoadingFormat->SetPicture(bitmap);
		if (isFirstPicture)
			SetBitmapToViewer(imageLoadingFormat, false);
		else
			SetBitmapToViewer(imageLoadingFormat, true);

		if (isFirstPicture)
			bitmapWindow->ShrinkImage();

		auto dlg = static_cast<CompressionAudioVideoOption*>(this->FindWindowByName("CompressionAudioVideoOption"));
		dlg->ChangeLabelPicture(label);

		isFirstPicture = false;
	}
}

void CShowPreview::ShowOriginal()
{
	ShowPicture(decodeFrameOriginal, "Original Video");
}

void CShowPreview::ShowNew()
{
	ShowPicture(decodeFrame, "New Video");
}

void CShowPreview::OnContextCreate(wxCommandEvent& event)
{
	//wxCommandEvent* evtevent = nullptr;
	//evtevent = new wxCommandEvent(wxEVENT_CONTEXTCREATE);
	//wxQueueEvent(this->GetParent(), evtevent);
}

void CShowPreview::OnShowOriginal(wxCommandEvent& event)
{
	ShowOriginal();
	showOriginal = true;
	oldShowOriginal = showOriginal;
}

void CShowPreview::OnShowNew(wxCommandEvent& event)
{
	ShowNew();
	showOriginal = false;
	oldShowOriginal = false;
}

void CShowPreview::OnUpdatePicture(wxCommandEvent& event)
{
	CRenderPreview* renderPreview = (CRenderPreview*)event.GetClientData();

	if (!renderPreview->compressIsOK)
	{
		wxCommandEvent evt(wxEVENT_ERRORCOMPRESSION);
		evt.SetInt(renderPreview->ret);
		renderPreview->parent->GetParent()->GetParent()->GetEventHandler()->AddPendingEvent(evt);

		ShowOriginal();
	}
	else
	{
		decodeFrame = renderPreview->decodeFrame;
		decodeFrameOriginal = renderPreview->decodeFrameOriginal;

		if (showOriginal)
			ShowOriginal();
		else
			ShowNew();
	}


	if (firstTime)
	{
		if (previewToolbar != nullptr)
			previewToolbar->SetTrackBarPosition(bitmapWindow->GetPosRatio());

		firstTime = false;
	}


	sliderVideo->Stop();

	StopThread();

	delete renderPreview;
	renderPreview = nullptr;
}

void CShowPreview::SlidePosChange(const int& position, const wxString& key)
{
	if (key == "Move")
	{
		this->key = key;
		moveSlider = true;
		showOriginal = true;
		this->position = position;
		UpdateBitmap("");
	}
	else
	{
		this->key = key;
		showOriginal = oldShowOriginal;
		moveSlider = false;
		this->position = position;
		UpdateBitmap("");
	}
}

void CShowPreview::MoveSlider(const int64_t& position)
{
	showOriginal = oldShowOriginal;
	moveSlider = false;
	this->position = position;
	UpdateBitmap("");
}

void CShowPreview::UpdateBitmap(const wxString& extension,
	const bool& updatePicture)
{
	wxString decoder = "";
	this->extension = extension;

	sliderVideo->Start();

	StopThread();

	CRenderPreview* renderPreview = new CRenderPreview();
	renderPreview->extension = extension;
	renderPreview->filename = filename;
	renderPreview->position = position;
	renderPreview->parent = this;
	renderPreview->videoOption = *videoOption;

	threadStart = std::make_unique<std::thread>(ThreadLoading, renderPreview);
}

void CShowPreview::ThreadLoading(void* data)
{
	int ret = 0;
	CRenderPreview * renderPreview = static_cast<CRenderPreview*>(data);
	COpenCLContext openCLContext;
	openCLContext.CreateDefaultOpenCLContext();
	CFFmpegTranscoding transcodeFFmpeg(&openCLContext, &renderPreview->videoOption);

	wxString fileTemp = "";

	if (renderPreview->extension == "")
	{
		wxFileName filename(renderPreview->filename);
		renderPreview->extension = filename.GetExt();
	}
	fileTemp = CFileUtility::GetTempFile("video_temp." + renderPreview->extension);
	ret = transcodeFFmpeg.EncodeFrame(renderPreview->filename, fileTemp, renderPreview->position);
	renderPreview->decodeFrameOriginal = transcodeFFmpeg.GetFrameOutputWithOutEffect();

	if (ret == 0)
	{
		CVideoThumb video(fileTemp);
		renderPreview->decodeFrame = video.GetVideoFramePos(0, 0, 0);
		if (renderPreview->decodeFrame.empty())
			renderPreview->decodeFrame = application_context.GetDefaultPicture();

		renderPreview->compressIsOK = true;

	}
	else
		renderPreview->compressIsOK = false;

	wxCommandEvent evt(wxEVENT_UPDATEPICTURE);
	evt.SetClientData(renderPreview);
	renderPreview->parent->GetEventHandler()->AddPendingEvent(evt);



}



void CShowPreview::OnControlSize(wxCommandEvent& event)
{
	if (scrollbar != nullptr)
	{
		wxCommandEvent evt(wxEVENT_SETCONTROLSIZE);
		evt.SetClientData(event.GetClientData());
		scrollbar->GetEventHandler()->AddPendingEvent(evt);
	}
}

void CShowPreview::OnSetPosition(wxCommandEvent& event)
{
	if (scrollbar != nullptr)
	{
		wxCommandEvent evt(wxEVENT_SETPOSITION);
		evt.SetClientData(event.GetClientData());
		scrollbar->GetEventHandler()->AddPendingEvent(evt);
	}
}

void CShowPreview::OnMoveLeft(wxCommandEvent& event)
{
	if (scrollbar != nullptr)
	{
		wxCommandEvent evt(wxEVENT_MOVELEFT);
		evt.SetInt(event.GetInt());
		scrollbar->GetEventHandler()->AddPendingEvent(evt);
	}
}

void CShowPreview::OnMoveRight(wxCommandEvent& event)
{
	if (scrollbar != nullptr)
	{
		wxCommandEvent evt(wxEVENT_MOVERIGHT);
		evt.SetInt(event.GetInt());
		scrollbar->GetEventHandler()->AddPendingEvent(evt);
	}
}

void CShowPreview::OnMoveTop(wxCommandEvent& event)
{
	if (scrollbar != nullptr)
	{
		wxCommandEvent evt(wxEVENT_MOVETOP);
		evt.SetInt(event.GetInt());
		scrollbar->GetEventHandler()->AddPendingEvent(evt);
	}
}

void CShowPreview::OnMoveBottom(wxCommandEvent& event)
{
	if (scrollbar != nullptr)
	{
		wxCommandEvent evt(wxEVENT_MOVEBOTTOM);
		evt.SetInt(event.GetInt());
		scrollbar->GetEventHandler()->AddPendingEvent(evt);
	}
}

void CShowPreview::StopThread()
{
	if (threadStart != nullptr)
	{
		threadStart->join();
		threadStart.reset();
		threadStart = nullptr;
	}

}

CShowPreview::~CShowPreview()
{
	StopThread();
}

void CShowPreview::Resize()
{
	int width = GetWindowWidth();
	int height = GetWindowHeight();
	if (width <= 0 || height <= 0)
		return;

	scrollbar->ShowVerticalScroll();
	scrollbar->ShowHorizontalScroll();

	int pictureWidth = width;
	int pictureHeight = height - previewToolbar->GetHeight() - sliderVideo->GetHeight();

	scrollbar->SetSize(0, 0, pictureWidth, pictureHeight);
	scrollbar->Refresh();
	previewToolbar->SetSize(0, pictureHeight, width, previewToolbar->GetHeight());
	previewToolbar->Refresh();
	sliderVideo->SetSize(0, pictureHeight + sliderVideo->GetHeight(), width, sliderVideo->GetHeight());
	sliderVideo->Refresh();
}


bool CShowPreview::SetBitmap(CImageLoadingFormat* bitmap)
{
	if (previewToolbar != nullptr)
		previewToolbar->SetTrackBarPosition(bitmapWindow->GetPosRatio());

	if (bitmapWindow != nullptr)
	{
		bitmap->SetOrientation(orientation);
		SetBitmapToViewer(bitmap, false);
	}

	return true;
}


void CShowPreview::OnViewerZoomIn(wxCommandEvent& event)
{
	if (previewToolbar != nullptr)
		previewToolbar->ChangeZoomInPos();
}

void CShowPreview::OnViewerZoomOut(wxCommandEvent& event)
{
	if (previewToolbar != nullptr)
		previewToolbar->ChangeZoomOutPos();
}
