#pragma once
#include <OpenCLContext.h>
class CompressVideo;
class CVideoOptionCompress;


class CFFmpegTranscoding
{
public:
	CFFmpegTranscoding(Regards::OpenCL::COpenCLContext * openCLContext, CVideoOptionCompress* videoCompressOption);
	~CFFmpegTranscoding();
	int EncodeFile(wxWindow* mainWindow, const wxString& input, const wxString& output, int rotation);
	int EncodeFrame(const wxString& input, const wxString& output, const int& position);
	int EndDecodeFile(const int& returnValue);
	wxString GetOutputFilename();
	cv::Mat GetFrameOutput();
	cv::Mat GetFrameOutputWithOutEffect();

	void SetOpenCLContext(Regards::OpenCL::COpenCLContext* openCLContext);
protected:
	static void EncodeFileThread(void* data);
	wxString input;
	wxString output;
	std::unique_ptr<std::thread> encode_thread;
	Regards::OpenCL::COpenCLContext * openCLContext = nullptr;
	cv::Mat data;
	cv::Mat data_withouteffect;
	std::unique_ptr<CompressVideo> m_dlgProgress;
	wxWindow* mainWindow;
	CVideoOptionCompress * videoCompressOption;
};
