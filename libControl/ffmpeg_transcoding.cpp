#include "header.h"
#include "ffmpeg_transcoding.h"
#include "FFmpegTranscodingPimpl.h"
#include <CompressVideo.h>
#include "VideoCompressOption.h"
#include <window_id.h>
#include <LibResource.h>


CFFmpegTranscoding::CFFmpegTranscoding(CVideoOptionCompress* videoCompressOption) :
	videoCompressOption(videoCompressOption)
{
	
}

CFFmpegTranscoding::~CFFmpegTranscoding()
{
}

int CFFmpegTranscoding::EncodeFrame(const wxString& input, const wxString& output, const int& position, Regards::OpenCL::COpenCLContext* openCLContext)
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

cv::Mat CFFmpegTranscoding::GetFrameOutputWithOutEffect()
{
	cv::Mat bitmap;
	data_withouteffect.copyTo(bitmap);
	return bitmap;
}


