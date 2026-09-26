#include <header.h>
#include "CropFilter.h"
#include "EffectParameter.h"
#include <LibResource.h>
#include <FiltreEffet.h>
#include <Draw.h>
#include <ImageLoadingFormat.h>
#include <BitmapDisplay.h>
#include <Crop.h>
#include "CropEffectParameter.h"
#include <effect_id.h>
using namespace Regards::Filter;

CCropFilter::CCropFilter()
{
}

CCropFilter::~CCropFilter()
{
}

CDraw* CCropFilter::GetDrawingPt()
{
	return new CCrop();
}

bool CCropFilter::IsOpenCLCompatible()
{
	return false;
}

void CCropFilter::SetCursor()
{
	wxSetCursor(wxCursor(wxCURSOR_CROSS));
}

bool CCropFilter::SupportMouseClick()
{
	return true;
}

bool CCropFilter::SupportMouseSelection()
{
	return true;
}

int CCropFilter::GetTypeFilter()
{
	return IDM_CROP;
}

int CCropFilter::TypeApplyFilter()
{
	return 2;
}

int CCropFilter::GetNameFilter()
{
	return IDM_CROP;
}


wxString CCropFilter::GetFilterLabel()
{
	return CLibResource::LoadStringFromResource("LBLCROP", 1);
}

void CCropFilter::Filter(CEffectParameter* effectParameter, cv::Mat& source, const wxString& filename,
                         IFiltreEffectInterface* filtreInterface)
{
	this->source = source;
	this->filename = filename;
}

void CCropFilter::FilterChangeParam(CEffectParameter* effectParameter, CTreeElementValue* valueData,
                                    const wxString& key)
{
}

CEffectParameter* CCropFilter::GetEffectPointer()
{
	return new CCropEffectParameter();
}

CEffectParameter* CCropFilter::GetDefaultEffectParameter()
{
	return new CCropEffectParameter();
}

CImageLoadingFormat* CCropFilter::ApplyEffect(CEffectParameter* effectParameter, IBitmapDisplay* bitmapViewer)
{
	cv::Mat out;
	CImageLoadingFormat* imageLoad = nullptr;

	auto cropEffectParameter = static_cast<CCropEffectParameter*>(effectParameter);

	bitmapViewer->GetDessinPt()->GetPos(cropEffectParameter->rcZoom);
	if (!source.empty())
	{
		imageLoad = new CImageLoadingFormat();
		imageLoad->SetPicture(source);
		//imageLoad->Flip();
		imageLoad->RotateExif(orientation);

		try
		{
			cv::Mat& matrix = imageLoad->GetMatImage();
			cv::Rect rect;
			rect.x = cropEffectParameter->rcZoom.x;
			rect.y = cropEffectParameter->rcZoom.y;
			rect.width = cropEffectParameter->rcZoom.width;
			rect.height = cropEffectParameter->rcZoom.height;
			out = matrix(rect);
		}
		catch (cv::Exception& e)
		{
			const char* err_msg = e.what();
			std::cout << "exception caught: " << err_msg << std::endl;
			std::cout << "wrong file format, please input the name of an IMAGE file" << std::endl;
		}

		cropEffectParameter->cropApply = true;
		imageLoad->SetPicture(out);
		//imageLoad->Flip();
	}

	return imageLoad;
}

CImageLoadingFormat* CCropFilter::ApplyMouseMoveEffect(CEffectParameter* effectParameter, IBitmapDisplay* bitmapViewer,
                                                       CDraw* dessin)
{
	return nullptr;
}

void CCropFilter::Drawing(wxMemoryDC* dc, IBitmapDisplay* bitmapViewer, CDraw* m_cDessin)
{
	int hpos = bitmapViewer->GetHPos();
	int vpos = bitmapViewer->GetVPos();

	if (m_cDessin != nullptr)
		m_cDessin->Dessiner(dc, hpos, vpos, bitmapViewer->GetRatio(), wxColour(0, 0, 0), wxColour(0, 0, 0),
		                    wxColour(0, 0, 0), 2);
}

void CCropFilter::ApplyPreviewEffect(CEffectParameter* effectParameter, IBitmapDisplay* bitmapViewer,
                                     CFiltreEffet* filtreEffet, CDraw* m_cDessin, int& widthOutput, int& heightOutput)
{
	DrawingToPicture(effectParameter, bitmapViewer, filtreEffet, m_cDessin);
}

void CCropFilter::RenderEffect(CFiltreEffet* filtreEffet, CEffectParameter* effectParameter, const bool& preview)
{
	cv::Mat out;
	CImageLoadingFormat* imageLoad = nullptr;

	auto cropEffectParameter = static_cast<CCropEffectParameter*>(effectParameter);

	if (cropEffectParameter->cropApply)
	{
		imageLoad = new CImageLoadingFormat();
		imageLoad->SetPicture(filtreEffet->GetBitmap(true));
		//imageLoad->Flip();
		imageLoad->RotateExif(orientation);

		try
		{
			cv::Mat& matrix = imageLoad->GetMatImage();
			cv::Rect rect;
			rect.x = cropEffectParameter->rcZoom.x;
			rect.y = cropEffectParameter->rcZoom.y;
			rect.width = cropEffectParameter->rcZoom.width;
			rect.height = cropEffectParameter->rcZoom.height;
			out = matrix(rect);
		}
		catch (cv::Exception& e)
		{
			const char* err_msg = e.what();
			std::cout << "exception caught: " << err_msg << std::endl;
			std::cout << "wrong file format, please input the name of an IMAGE file" << std::endl;
		}

		imageLoad->SetPicture(out);
		filtreEffet->SetBitmap(imageLoad);
		//imageLoad->Flip();
	}
}

bool CCropFilter::NeedToUpdateSource()
{
	return true;
}

bool CCropFilter::NeedPreview()
{
	return true;
}
