#include "header.h"
#include "ffmpeg_transcoding.h"
#include "FFmpegTranscodingPimpl.h"
#include <CompressVideo.h>
#include "VideoCompressOption.h"
#include <window_id.h>
#include <LibResource.h>


CFFmpegTranscoding::CFFmpegTranscoding(Regards::OpenCL::COpenCLContext* openCLContext, CVideoOptionCompress* videoCompressOption) :
	encode_thread(nullptr),
	m_dlgProgress(nullptr),
	mainWindow(nullptr),
	openCLContext(openCLContext),
	videoCompressOption(videoCompressOption)
{
	
}

CFFmpegTranscoding::~CFFmpegTranscoding()
{
}



wxString CFFmpegTranscoding::GetOutputFilename()
{
	return output;
}

int CFFmpegTranscoding::EncodeFrame(const wxString& input, const wxString& output, const int& position)
{
	CFFmpegTranscodingPimpl ffmpegtranscoding(openCLContext);
	int ret = ffmpegtranscoding.EncodeOneFrame(nullptr, input, output, position, videoCompressOption);
	if (!ffmpegtranscoding.GetFrameOutput().empty())
	{
		data = ffmpegtranscoding.GetFrameOutput();
		data_withouteffect = ffmpegtranscoding.GetFrameOutputWithOutEffect();
	}

	return 0;
}

cv::Mat CFFmpegTranscoding::GetFrameOutput()
{
	cv::Mat bitmap;
	data.copyTo(bitmap);
	return bitmap;
}

void CFFmpegTranscoding::SetOpenCLContext(Regards::OpenCL::COpenCLContext* openCLContext)
{
	this->openCLContext = openCLContext;
}

cv::Mat CFFmpegTranscoding::GetFrameOutputWithOutEffect()
{
	cv::Mat bitmap;
	data_withouteffect.copyTo(bitmap);
	return bitmap;
}

void CFFmpegTranscoding::EncodeFileThread(void* data)
{
	auto ffmpeg_encoding = static_cast<CFFmpegTranscoding*>(data);

	std::unique_ptr<COpenCLContext> openCLContext = std::make_unique<COpenCLContext>();
	openCLContext->CreateDefaultOpenCLContext();

	CFFmpegTranscodingPimpl ffmpegtranscoding(openCLContext.get());

	int ret = ffmpegtranscoding.EncodeFile(ffmpeg_encoding->input, ffmpeg_encoding->output,
	                                       ffmpeg_encoding->m_dlgProgress.get(), ffmpeg_encoding->videoCompressOption);
	if (ret < 0)
	{
		wxString errorConversion = CLibResource::LoadStringFromResource("LBLERRORCONVERSION", 1);

		char message[255];
		av_make_error_string(message, AV_ERROR_MAX_STRING_SIZE, ret);
		wxMessageBox(message, errorConversion, wxICON_ERROR);
	}

	wxCommandEvent event(wxEVENT_ENDCOMPRESSION);
	event.SetInt(ret);
	wxPostEvent(ffmpeg_encoding->mainWindow, event);
}

int CFFmpegTranscoding::EndDecodeFile(const int& returnValue)
{
	m_dlgProgress->Close();
	encode_thread->join();

	wxSleep(1);


	if (returnValue == 0)
	{
		wxString filecompleted = CLibResource::LoadStringFromResource("LBLFILEENCODINGCOMPLETED", 1);
		wxString infos = CLibResource::LoadStringFromResource("LBLINFORMATIONS", 1);
		wxMessageBox(filecompleted, infos);
	}
	return 0;
}

int CFFmpegTranscoding::EncodeFile(wxWindow* mainWindow, const wxString& input, const wxString& output, int rotation)
{
	this->mainWindow = mainWindow;
	this->input = input;
	this->output = output;

	if (encode_thread != nullptr)
		encode_thread.reset();
	if (m_dlgProgress != nullptr)
		m_dlgProgress.reset();


	m_dlgProgress = std::make_unique<CompressVideo>(nullptr, rotation);
	m_dlgProgress->SetFocus();  // focus on my window
	m_dlgProgress->Raise();  // bring window to front
	m_dlgProgress->Show();
	encode_thread = std::make_unique<std::thread>(EncodeFileThread, this);
	return 0;
}
