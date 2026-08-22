#pragma once
#include <OpenCLContext.h>
class CompressVideo;
class CVideoOptionCompress;


class CFFmpegTranscoding
{
public:
	CFFmpegTranscoding(CVideoOptionCompress* videoCompressOption);
	~CFFmpegTranscoding();
	int EncodeFrame(const wxString& input, const wxString& output, const int& position, Regards::OpenCL::COpenCLContext* openCLContext);
	cv::Mat GetFrameOutput();
	cv::Mat GetFrameOutputWithOutEffect();

protected:


	Regards::OpenCL::COpenCLContext * openCLContext = nullptr;
	cv::Mat data;
	cv::Mat data_withouteffect;
	CVideoOptionCompress * videoCompressOption;
};
