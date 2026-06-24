#pragma once
class CompressVideo;
class CVideoOptionCompress;


class CFFmpegTranscoding
{
public:
	CFFmpegTranscoding();
	~CFFmpegTranscoding();
	int EncodeFile(wxWindow* mainWindow, const wxString& input, const wxString& output, int rotation);
	int EncodeFrame(const wxString& input, const wxString& output, const int& position);
	int EndDecodeFile(const int& returnValue);
	wxString GetOutputFilename();
	cv::Mat GetFrameOutput();
	cv::Mat GetFrameOutputWithOutEffect();
	CVideoOptionCompress* GetVideoCompressionPt();
protected:
	static void EncodeFileThread(void* data);
	wxString input;
	wxString output;
	std::unique_ptr<std::thread> encode_thread;
	cv::Mat data;
	cv::Mat data_withouteffect;
	std::unique_ptr<CompressVideo> m_dlgProgress;
	wxWindow* mainWindow;
	std::unique_ptr<CVideoOptionCompress> videoCompressOption;
};
