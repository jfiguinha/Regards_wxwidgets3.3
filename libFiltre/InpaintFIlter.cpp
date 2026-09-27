#include <header.h>
#include "InpaintFilter.h"
#include "EffectParameter.h"
#include <LibResource.h>
#include <FiltreEffet.h>
#include <Draw.h>
#include <ImageLoadingFormat.h>
#include <BitmapDisplay.h>
#include "FiltreEffetCPU.h"
#include <effect_id.h>
#include "InpaintFilterParam.h"
#include <wx/busyinfo.h>
#include <Metadata.h>
using namespace Regards::Filter;
using namespace cv;

CInpaintFilter::CInpaintFilter()
{
	libelleAlgorithm = "Effect.Algorithm";
}

CInpaintFilter::~CInpaintFilter()
{
}

void CInpaintFilter::AddMetadataElement(vector<CMetadata>& element, wxString value, int key)
{
	CMetadata linear;
	linear.value = value;
	linear.depth = key;
	element.push_back(linear);
}

CEffectParameter* CInpaintFilter::GetEffectPointer()
{
	return new CInpaintFilterParameter();
}



void CInpaintFilter::Filter(CEffectParameter* effectParameter, cv::Mat& source, const wxString& filename,
	IFiltreEffectInterface* filtreInterface)
{
	this->source = source;
	this->filename = filename;
	CInpaintFilterParameter* inpaintParam = (CInpaintFilterParameter*)effectParameter;

	vector<CMetadata> formatPicture;
	AddMetadataElement(formatPicture, "SHIFTMAP", 0);
	AddMetadataElement(formatPicture, "FSR FAST", 1);
	AddMetadataElement(formatPicture, "FSR BEST", 2);

	filtreInterface->AddTreeInfos(libelleAlgorithm, new CTreeElementValueInt(inpaintParam->algo),
		&formatPicture, 3, 3);
}

void CInpaintFilter::FilterChangeParam(CEffectParameter* effectParameter, CTreeElementValue* valueData,
	const wxString& key)
{
	auto videoEffectParameter = static_cast<CInpaintFilterParameter*>(effectParameter);

	float value = 0.0;
	switch (valueData->GetType())
	{
	case TYPE_ELEMENT_INT:
	{
		auto intValue = static_cast<CTreeElementValueInt*>(valueData);
		value = intValue->GetValue();
	}
	break;
	case TYPE_ELEMENT_FLOAT:
	{
		auto intValue = static_cast<CTreeElementValueFloat*>(valueData);
		value = intValue->GetValue();
	}
	break;
	case TYPE_ELEMENT_BOOL:
	{
		auto intValue = static_cast<CTreeElementValueBool*>(valueData);
		value = intValue->GetValue();
	}
	break;
	default:;
	}

	//Video Parameter

	if (key == libelleAlgorithm)
	{
		videoEffectParameter->algo = value;
	}
}



int CInpaintFilter::GetTypeFilter()
{
	return IDM_INPAINT;
}

int CInpaintFilter::GetNameFilter()
{
	return IDM_INPAINT;
}


wxString CInpaintFilter::GetFilterLabel()
{
	return CLibResource::LoadStringFromResource("LBLINPAINT", 1);
}


void CInpaintFilter::RenderEffect(CFiltreEffet* filtreEffet, CEffectParameter* effectParameter, const bool& preview)
{
	const wxString libelle =
		CLibResource::LoadStringFromResource(L"LBLBUSYINFO", 1);

	wxBusyInfo wait(libelle, nullptr);

	CImageLoadingFormat* imageLoad = nullptr;
	auto videoEffectParameter = static_cast<CInpaintFilterParameter*>(effectParameter);

	if (videoEffectParameter->cropApply)
	{
		imageLoad = new CImageLoadingFormat();
		imageLoad->SetPicture(filtreEffet->GetBitmap(true));
		//imageLoad->Flip();
		imageLoad->RotateExif(orientation);

		try
		{
			cv::Mat& matrix = imageLoad->GetMatImage();

			cv::Rect rect;
			rect.x = videoEffectParameter->rcZoom.x;
			rect.y = videoEffectParameter->rcZoom.y;
			rect.width = videoEffectParameter->rcZoom.width;
			rect.height = videoEffectParameter->rcZoom.height;


			cv::Mat mask = GenerateMaskFromZone(rect, matrix);
			filtreEffet->Inpaint(mask, videoEffectParameter->algo);

			imageLoad = new CImageLoadingFormat();
			cv::Mat bitmapOut = filtreEffet->GetBitmap(true);
			imageLoad->SetPicture(bitmapOut);
			filtreEffet->SetBitmap(imageLoad);

		}
		catch (cv::Exception& e)
		{
			const char* err_msg = e.what();
			std::cout << "exception caught: " << err_msg << std::endl;
			std::cout << "wrong file format, please input the name of an IMAGE file" << std::endl;
		}
	}
}

CImageLoadingFormat* CInpaintFilter::ApplyEffect(CEffectParameter* effectParameter, IBitmapDisplay* bitmapViewer)
{
	const wxString libelle =
		CLibResource::LoadStringFromResource(L"LBLBUSYINFO", 1);

	wxBusyInfo wait(libelle, nullptr);

	CImageLoadingFormat* imageLoad = nullptr;

	auto videoEffectParameter = static_cast<CInpaintFilterParameter*>(effectParameter);
	bitmapViewer->GetDessinPt()->GetPos(videoEffectParameter->rcZoom);
	if (!source.empty())
	{
		CImageLoadingFormat image;
		image.SetPicture(source);
		image.RotateExif(orientation);

		CRgbaquad rgbaQuad;
		CFiltreEffetCPU filtreCPU(rgbaQuad, &image);

		try
		{
			cv::Rect rect;
			rect.x = videoEffectParameter->rcZoom.x;
			rect.y = videoEffectParameter->rcZoom.y;
			rect.width = videoEffectParameter->rcZoom.width;
			rect.height = videoEffectParameter->rcZoom.height;
           
            cv::Mat mask = GenerateMaskFromZone(rect, source);          
			filtreCPU.Inpaint(mask, videoEffectParameter->algo);
            
			imageLoad = new CImageLoadingFormat();
			cv::Mat bitmapOut = filtreCPU.GetBitmap(true);
			imageLoad->SetPicture(bitmapOut);
			videoEffectParameter->cropApply = true;
            
		}
		catch (cv::Exception& e)
		{
			const char* err_msg = e.what();
			std::cout << "exception caught: " << err_msg << std::endl;
			std::cout << "wrong file format, please input the name of an IMAGE file" << std::endl;
		}


	}

	return imageLoad;
}

 cv::Mat CInpaintFilter::GenerateMaskFromZone(const cv::Rect & rect, const cv::Mat & src)
 {
    vector<vector<cv::Point>> contours;
    vector<cv::Point> pts;
    pts.push_back(cv::Point(rect.x,rect.y));
    pts.push_back(cv::Point(rect.x + rect.width,rect.y));
    pts.push_back(cv::Point(rect.x + rect.width,rect.y + rect.height));
    pts.push_back(cv::Point(rect.x,rect.y + rect.height));
    contours.push_back(pts);
    
    cv::Mat mask(src.size(),CV_8UC1);
    mask = 0;

	//Rectangle
	{
		cv::Mat out = src(rect);
		cvtColor(out, out, cv::COLOR_BGR2GRAY);
		out = 255;
		out.copyTo(mask(rect));
	}

	cv::bitwise_not(mask, mask);

    return mask;
 }
