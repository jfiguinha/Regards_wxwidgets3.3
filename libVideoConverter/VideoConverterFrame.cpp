#include <header.h>
#include <ffmpeg_transcoding.h>
#include "VideoConverterFrame.h"
#include <CompressionAudioVideoOption.h>
#include <VideoCompressOption.h>
#include <wx/filename.h>
#include <MediaInfo.h>
#include <ffmpeg_application.h>
#include <ConvertUtility.h>
#include <FileUtility.h>
#include <LibResource.h>
#include <libPicture.h>
extern "C" {
#include <libswscale/swscale.h>
}
#if defined(__WXMSW__)
#include "../include/window_id.h"
#else
#include <window_id.h>
#endif

#ifndef wxHAS_IMAGES_IN_RESOURCES
#ifdef __WXGTK__
#include "../Resource/sample.xpm"
#else
#include "../../Resource/sample.xpm"
#endif
#endif

//Connect(wxEVT_MOVE, wxMoveEventHandler(Move::OnMove));
BEGIN_EVENT_TABLE(CVideoConverterFrame, wxFrame)
EVT_CLOSE(CVideoConverterFrame::OnCloseWindow)
END_EVENT_TABLE()

using namespace Regards::Picture;

#define LOG_ERROR(msg) \
    std::cerr << "[ERROR] " << __FUNCTION__ << " : " << msg << std::endl

template<typename Func>
bool ExecuteFFmpeg(Func&& func)
{
	try
	{
		CFFmpegApp app(false);
		func(app);
		return true;
	}
	catch (const std::exception& e)
	{
		LOG_ERROR(e.what());
		return false;
	}
}


// ---------------------------------------------------------------------------
// Internal helpers
// ---------------------------------------------------------------------------
namespace {

	void RemoveIfExists(const wxString& path)
	{
		if (wxFileExists(path))
			wxRemoveFile(path);
	}

} // namespace

// ----------------------------------------------------------------------------
// main frame
// ----------------------------------------------------------------------------

// frame constructor
CVideoConverterFrame::CVideoConverterFrame(const wxString& title, const wxPoint& pos, const wxSize& size, IVideoConverterInterface* videoInterface,  long style) :
	wxFrame(nullptr, FRAMEVIDEOCONVERTER_ID, title, pos, size, style)
{
	SetIcon(wxICON(sample));
	this->videoInterface = videoInterface;
	Connect(wxEVENT_ENDCOMPRESSION, wxCommandEventHandler(CVideoConverterFrame::OnEndDecompressFile));
	

}

void CVideoConverterFrame::OnEndDecompressFile(wxCommandEvent& event)
{
	wxString outputFile = "";
	int ret = event.GetInt();
	if (ffmpegEncoder != nullptr)
	{
		ffmpegEncoder->EndDecodeFile(ret);

		outputFile = ffmpegEncoder->GetOutputFilename();

		ffmpegEncoder.reset();
	}

	{
		static const wxString file[] = { tempAudioVideoFile , tempVideoFile, filepathVideo };
		for (auto filepath : file)
			RemoveIfExists(filepath);
	}

	if (needToRemux)
	{
		if (isAudio && wxFileExists(fileOut) && wxFileExists(fileOutAudio))
		{
			ExecuteFFmpeg([&](CFFmpegApp& app)
				{
					app.ExecuteFFmpegMuxVideoAudio(fileOut, fileOutAudio, filepathVideo);
				});
		}
		else if (wxFileExists(fileOut) && wxFileExists(fileOutVideo))
		{
			ExecuteFFmpeg([&](CFFmpegApp& app)
				{
					app.ExecuteFFmpegMuxVideoAudio(fileOutVideo, fileOut, filepathVideo);
				});
			
		}

		//Cleanup
		static const wxString file[] = {fileOutVideo , fileOutAudio,fileOut};
		for (auto filepath : file)
			RemoveIfExists(filepath);
	}
	else
	{
		RemoveIfExists(fileOut);
	}


	if (videoInterface != nullptr)
	{
		videoInterface->Close();
	}

}

wxString CVideoConverterFrame::SelectFile()
{
	wxFileDialog openFileDialog(this, _("Open video file"), "", "",
		"mp4 files (*.mp4)|*.mp4", wxFD_OPEN | wxFD_FILE_MUST_EXIST);

	wxString documentPath = CFileUtility::GetDocumentFolderPath();
	openFileDialog.SetDirectory(documentPath);

	if (openFileDialog.ShowModal() == wxID_CANCEL)
		return "";
	

	return openFileDialog.GetPath();
}

wxString CVideoConverterFrame::SelectOutputFile(wxString& filename)
{
	wxString filepath;
	wxFileName videoFilename(filename);
	wxString savevideofile = CLibResource::LoadStringFromResource(L"LBLSAVEVIDEOFILE", 1);
	wxString filename_label = CLibResource::LoadStringFromResource(L"LBLFILESNAME", 1);


	wxString filenameToSave = videoFilename.GetName();


	wxFileDialog saveFileDialog(nullptr, savevideofile, "", filenameToSave,
		"mp4 " + filename_label + " (*.mp4)|*.mp4|webm " + filename_label +
		" (*.webm)|*.webm|mov " + filename_label + " (*.mov)|*.mov|mkv " + filename_label +
		" (*.mkv)|*.mkv", wxFD_SAVE | wxFD_OVERWRITE_PROMPT);

	wxString documentPath = CFileUtility::GetDocumentFolderPath();
	saveFileDialog.SetDirectory(documentPath);

	if (saveFileDialog.ShowModal() == wxID_CANCEL)
	{
		return ""; // the user changed idea...
	}

	filepath = saveFileDialog.GetPath();
	int index = saveFileDialog.GetFilterIndex();

	wxWindow* videoWindow = this->FindWindowById(SHOWBITMAPVIEWERID);
	if (videoWindow != nullptr)
	{
		wxCommandEvent event(wxEVENT_PAUSEMOVIE);
		wxPostEvent(videoWindow, event);
	}

	wxFileName file_path(filepath);
	wxString extension = file_path.GetExt();

	const wxString ext = wxFileName(filepath).GetExt();
	if (ext != "mp4" && ext != "webm" && ext != "mov" && ext != "mkv")
	{
		static const wxString kExts[] = { "mp4", "webm", "mov", "mkv" };
		filepath += "." + kExts[std::min(index, 3)];
	}
	return filepath;
}

void CVideoConverterFrame::ExitApplication()
{
	if (videoInterface != nullptr)
	{
		videoInterface->Close();
	}
}

void CVideoConverterFrame::ExportVideo(wxString fileIn)
{
	CMediaInfo metadata;
	bool exit_frame = true;
	bool result = false;
	CLibPicture libPicture;
	fileOut = "";
	wxString filename = fileIn;

	if (!wxFileExists(filename))
		filename = SelectFile();

	if (filename.empty() || !libPicture.TestIsVideo(filename))
	{
		ExitApplication();
		return;
	}
	
	int rotation = metadata.GetVideoRotation(filename);
	int ret = 0;
	wxString filepath = SelectOutputFile(filename);
	if(filepath.empty())
	{
		ExitApplication();
		return;
	}

	wxString filename_in = filename;
	auto compressAudioVideoOption = std::make_unique<CompressionAudioVideoOption>(this);
	compressAudioVideoOption->SetFile(filename, filepath);
	compressAudioVideoOption->ShowModal();
	if (compressAudioVideoOption->IsOk())
	{
		if (ffmpegEncoder == nullptr)
		{
			ffmpegEncoder = std::make_unique<CFFmpegTranscoding>();

			auto videoCompressOption = ffmpegEncoder->GetVideoCompressionPt();
			compressAudioVideoOption->GetCompressionOption(videoCompressOption);

			if ((videoCompressOption->audioDirectCopy && videoCompressOption->videoDirectCopy) || (!videoCompressOption
				->audioDirectCopy && !videoCompressOption->videoDirectCopy))
			{
				needToRemux = false;

				if (videoCompressOption->startTime != 0 || videoCompressOption->endTime != 0)
				{
					if (wxFileExists(filename))
					{

						bool result = ExecuteFFmpeg([&](CFFmpegApp& app)
							{
								wxFileName file_temp(filepath);
								fileOut = CFileUtility::GetTempFile("temp." + file_temp.GetExt(), file_temp.GetPath(),
									true);

								wxString timeInput = CConvertUtility::GetTimeLibelle(videoCompressOption->startTime);
								wxString timeOutput = CConvertUtility::GetTimeLibelle(videoCompressOption->endTime);
								app.ExecuteFFmpegCutVideo(filename, timeInput, timeOutput, fileOut);
							});

						if (!result)
						{
							ret = -1;
						}
					}
					else
						ret = -1;
				}

				if (ret == 0)
				{
					if (wxFileExists(filename_in))
					{
						if (videoCompressOption->audioDirectCopy && videoCompressOption->videoDirectCopy)
						{
							RemoveIfExists(filepath);
							wxCopyFile(filename_in, filepath);
						}
						else if (!videoCompressOption->audioDirectCopy && !videoCompressOption->videoDirectCopy)
						{
							RemoveIfExists(filepath);

							wxString decoder = "";
							
							ffmpegEncoder->EncodeFile(this, filename_in, filepath, rotation);
						}
					}
					else
						ret = -1;
				}
			}
			else
			{
				needToRemux = true;

				wxFileName file_temp(filepath);

				filepathVideo = filepath;

				fileOut = CFileUtility::GetTempFile("temp." + file_temp.GetExt(), file_temp.GetPath(), true);

				fileOutVideo = "";
				fileOutAudio = "";

				wxString timeInput = CConvertUtility::GetTimeLibelle(videoCompressOption->startTime);
				wxString timeOutput = CConvertUtility::GetTimeLibelle(videoCompressOption->endTime);

				if (wxFileExists(filename_in))
				{
					{

						ExecuteFFmpeg([&](CFFmpegApp& app)
							{
								wxFileName file_temp(filepath);
								fileOutAudio = CFileUtility::GetTempFile("temp_audio." + file_temp.GetExt(),
									file_temp.GetPath(), true);
								app.ExecuteFFmpegExtractAudio(filename_in, timeInput, timeOutput, fileOutAudio);
							});
					}

					{

						ExecuteFFmpeg([&](CFFmpegApp& app)
							{
								wxFileName file_temp(filepath);
								fileOutVideo = CFileUtility::GetTempFile("temp_video." + file_temp.GetExt(),
									file_temp.GetPath(), true);
								app.ExecuteFFmpegExtractVideo(filename_in, timeInput, timeOutput, fileOutVideo);
							});

					}
				}
				else
					ret = -1;

				if (ret == 0)
				{
					if (videoCompressOption->audioDirectCopy)
					{
						if (wxFileExists(fileOutVideo))
						{
							ffmpegEncoder->EncodeFile(this, fileOutVideo, fileOut, rotation);
							isAudio = true;
						}
						else
							ret = -1;
					}
					else
					{
						if (wxFileExists(fileOutAudio))
						{
							ffmpegEncoder->EncodeFile(this, fileOutAudio, fileOut, rotation);
							isAudio = false;
						}
						else
							ret = -1;
					}
				}
			}


			exit_frame = false;
		}
	}



	if (ret < 0)
	{
		wxString errorConversion = CLibResource::LoadStringFromResource("LBLERRORCONVERSION", 1);
		char message[255];
		av_make_error_string(message, AV_ERROR_MAX_STRING_SIZE, ret);
		wxMessageBox(message, errorConversion, wxICON_ERROR);


		exit_frame = true;

	}

	if (exit_frame)
	{
		if (videoInterface != nullptr)
		{
			videoInterface->Close();
		}
	}

}

void CVideoConverterFrame::OnCloseWindow(wxCloseEvent& event)
{
	if (videoInterface != nullptr)
	{
		videoInterface->Close();
	}

}
