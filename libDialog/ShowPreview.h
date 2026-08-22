#pragma once
#include <ThemeParam.h>
#include "PreviewToolbar.h"
#include "SliderVideoPreview.h"
#include "ScrollbarWnd.h"
#include <VideoCompressOption.h>
#include <OpenCLContext.h>
using namespace Regards::Window;

class CFFmpegDecodeFrameFilter;
class CFFmpegTranscoding;

class CRegardsConfigParam;
class CImageLoadingFormat;

namespace Regards
{
	namespace Window
	{
		class CBitmapWndRender;
		class CBitmapWnd3D;
	}

	namespace Video
	{
		class CVideoThumb;
	}

	namespace Control
	{
		class CRenderPreview
		{
		public:
			CRenderPreview() = default;
			~CRenderPreview() = default;

			cv::Mat decodeFrameOriginal;
			cv::Mat decodeFrame;
			wxWindow * parent;
			CVideoOptionCompress videoOption;
			bool compressIsOK;
			wxString filename;
			wxString extension;
			int position;
			int ret;
		};

		class CShowPreview : public CWindowMain, public CSliderInterface
		{
		public:
			CShowPreview(wxWindow* parent, wxWindowID id, CThemeParam* config, CVideoOptionCompress * videoOptionPt);
			~CShowPreview() override;
			void SetParameter(const wxString& videoFilename);
			//bool SetBitmap(CImageLoadingFormat* bitmap, const bool& isThumbnail);
			//CRegardsBitmap* GetBitmap(const bool& source);
			void UpdateScreenRatio() override;
			void SlidePosChange(const int& position, const wxString& key) override;

			void ZoomPos(const int& position) override
			{
			};
			void MoveSlider(const int64_t& position) override;

			void ClickButton(const int& id) override
			{
			};

			void SetTrackBarPosition(const int& iPos) override
			{
			};
			void UpdateBitmap(const wxString& extension,
			                  const bool& updatePicture = true);


		private:
			void ShowOriginal();
			void ShowNew();
			void StopThread();
			void SetBitmapToViewer(CImageLoadingFormat* bitmap, bool isUpdate);
			void OnContextCreate(wxCommandEvent& event);
			void OnViewerZoomIn(wxCommandEvent& event);
			void OnViewerZoomOut(wxCommandEvent& event);
			void Resize() override;
			void OnControlSize(wxCommandEvent& event);
			void OnSetPosition(wxCommandEvent& event);
			void OnMoveLeft(wxCommandEvent& event);
			void OnMoveRight(wxCommandEvent& event);
			void OnMoveTop(wxCommandEvent& event);
			void OnMoveBottom(wxCommandEvent& event);
			void ShowPicture(cv::Mat& decodeFrame, const wxString& label);
			void OnShowOriginal(wxCommandEvent& event);
			void OnShowNew(wxCommandEvent& event);
			void OnUpdatePicture(wxCommandEvent& event);
			bool SetBitmap(CImageLoadingFormat* bitmap);
			static void ThreadLoading(void* data);

			std::unique_ptr<CScrollbarWnd> scrollbar;
			std::unique_ptr<CPreviewToolbar> previewToolbar;
			std::unique_ptr<CSliderVideoPreview> sliderVideo;
			std::unique_ptr<CBitmapWndRender> bitmapWindow;
			std::unique_ptr<CBitmapWnd3D> bitmapWindowRender;
			CVideoOptionCompress* videoOption;
			CRegardsConfigParam* configRegards;

			bool defaultToolbar;
			bool defaultViewer;
			//bool bitmapWndLocal;
			//std::unique_ptr<Video::CVideoThumb> videoOriginal = nullptr;
			//CFFmpegTranscoding* transcodeFFmpeg = nullptr;
			cv::Mat decodeFrame;
			cv::Mat decodeFrameOriginal;


			wxString extension;
			bool transitionEnd;
			wxString filename;
			int progressValue;
			double timeTotal;
			int position = 0;
			bool showOriginal = false;
			bool isFirstPicture = true;
			std::unique_ptr<std::thread> threadStart = nullptr;
			bool moveSlider = false;
			bool oldShowOriginal = false;
			bool firstTime = true;
			bool compressIsOK = true;
			wxString key = "";
			int orientation = 0;
		};
	}
}
