#include <header.h>
#include <ffmpeg_transcoding.h>
#include "VideoConverterFrame.h"
#include <CompressionAudioVideoOption.h>
#include <VideoCompressOption.h>
#include <wx/filename.h>
#include <MediaInfo.h>

#include <MediaExtractor.h>
#include <ConvertUtility.h>
#include <FileUtility.h>
#include <LibResource.h>
#include <libPicture.h>

#if defined(__WXMSW__)
#include "../include/window_id.h"
#else
#include <window_id.h>
#endif


//Connect(wxEVT_MOVE, wxMoveEventHandler(Move::OnMove));
BEGIN_EVENT_TABLE(CVideoConverterFrame, wxFrame)
EVT_CLOSE(CVideoConverterFrame::OnCloseWindow)
END_EVENT_TABLE()

using namespace Regards::Picture;

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

CVideoConverterFrame::~CVideoConverterFrame()
{
	RemoveIfExists(fileOut);
	RemoveIfExists(fileOutAudio);
	RemoveIfExists(fileOutVideo);
}

void CVideoConverterFrame::OnEndDecompressFile(wxCommandEvent& event)
{
	bool result = false;
	int ret = event.GetInt();
	if (ffmpegEncoder != nullptr)
	{
		ffmpegEncoder->EndDecodeFile(ret);
		ffmpegEncoder.reset();
	}


	if (needToRemux)
	{
		RemoveIfExists(fileOutputPath);

		if (isAudio && wxFileExists(fileOut) && wxFileExists(fileOutAudio))
			result = Regards::Media::ExecuteFFmpegMuxVideoAudio(fileOut.utf8_string(), fileOutAudio.utf8_string(), fileOutputPath.utf8_string());
		else if (wxFileExists(fileOut) && wxFileExists(fileOutVideo))
			result = Regards::Media::ExecuteFFmpegMuxVideoAudio(fileOutVideo.utf8_string(), fileOut.utf8_string(), fileOutputPath.utf8_string());

		//Cleanup
		static const wxString file[] = {fileOutVideo , fileOutAudio, fileOut};
		for (auto filepath : file)
			RemoveIfExists(filepath);
	}
	else
	{
		RemoveIfExists(fileOut);
	}

	ExitApplication();
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

void CVideoConverterFrame::ExportVideo(const wxString& fileIn)
{
	CMediaInfo metadata;
	bool exit_frame = false;
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
	fileOutputPath = SelectOutputFile(filename);
	if(fileOutputPath.empty())
	{
		ExitApplication();
		return;
	}

	auto compressAudioVideoOption = new CompressionAudioVideoOption(this);
	compressAudioVideoOption->SetFile(filename, fileOutputPath);
	compressAudioVideoOption->ShowModal();
	if (compressAudioVideoOption->IsOk())
	{
		ffmpegEncoder = std::make_unique<CFFmpegTranscoding>();

		wxString timeInput = "00:00:00";
		wxString timeOutput = "00:00:00";

		auto videoCompressOption = ffmpegEncoder->GetVideoCompressionPt();
		compressAudioVideoOption->GetCompressionOption(videoCompressOption);

		if (videoCompressOption->startTime != 0 || videoCompressOption->endTime != 0)
		{
			timeInput = CConvertUtility::GetTimeLibelle(videoCompressOption->startTime);
			timeOutput = CConvertUtility::GetTimeLibelle(videoCompressOption->endTime);
		}

		if ((videoCompressOption->audioDirectCopy && videoCompressOption->videoDirectCopy) || (!videoCompressOption
			->audioDirectCopy && !videoCompressOption->videoDirectCopy))
		{
			wxFileName file_temp(fileOutputPath);
			fileOut = CFileUtility::GetTempFile("temp." + file_temp.GetExt(), true);

			wxString timeInput = CConvertUtility::GetTimeLibelle(videoCompressOption->startTime);
			wxString timeOutput = CConvertUtility::GetTimeLibelle(videoCompressOption->endTime);
			result = Regards::Media::ExecuteFFmpegCutVideo(filename.utf8_string(), timeInput.utf8_string(), timeOutput.utf8_string(), fileOut.utf8_string());

			if (videoCompressOption->audioDirectCopy && videoCompressOption->videoDirectCopy)
			{
				RemoveIfExists(fileOutputPath);
				wxCopyFile(fileOut, fileOutputPath);
				needToRemux = false;
			}
			else
			{
				RemoveIfExists(fileOutputPath);
				ret = ffmpegEncoder->EncodeFile(this, fileOut, fileOutputPath, rotation);
				needToRemux = false;
			}
		}
		else
		{
			if (videoCompressOption->audioDirectCopy)
			{

				wxFileName file_temp(fileOutputPath);
				fileOutVideo = CFileUtility::GetTempFile("temp_video." + file_temp.GetExt(), true);
				result = Regards::Media::ExecuteFFmpegExtractVideo(filename.utf8_string(), timeInput.utf8_string(), timeOutput.utf8_string(), fileOutVideo.utf8_string());

				if (result && wxFileExists(fileOutVideo))
				{
					ffmpegEncoder->EncodeFile(this, fileOutVideo, fileOut, rotation);
					isAudio = true;
					needToRemux = true;
				}
				else
					result = false;
			}
			else if (videoCompressOption->videoDirectCopy)
			{
				wxFileName file_temp(fileOutputPath);
				fileOutAudio = CFileUtility::GetTempFile("temp_audio." + file_temp.GetExt(), true);
				result = Regards::Media::ExecuteFFmpegExtractAudio(filename.utf8_string(), timeInput.utf8_string(), timeOutput.utf8_string(), fileOutAudio.utf8_string());

				if (result && wxFileExists(fileOutAudio))
				{
					ret = ffmpegEncoder->EncodeFile(this, fileOutAudio, fileOut, rotation);
					isAudio = false;
					needToRemux = true;
				}
				else
					result = false;
			}
		}

	}
	else
	{
		exit_frame = true;
	}


	if (!result)
	{
		wxString errorConversion = CLibResource::LoadStringFromResource("LBLERRORCONVERSION", 1);
		char message[255];
		av_make_error_string(message, AV_ERROR_MAX_STRING_SIZE, ret);
		wxMessageBox(message, errorConversion, wxICON_ERROR);
		exit_frame = true;
	}

	if (exit_frame)
	{
		ExitApplication();
	}

}

void CVideoConverterFrame::OnCloseWindow(wxCloseEvent& event)
{
	ExitApplication();
}
