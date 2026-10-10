#include <header.h>
#include "CloudsFilter.h"
#include "CloudsEffectParameter.h"
#include <LibResource.h>
#include <FiltreEffet.h>
#include <ImageLoadingFormat.h>
#include <BitmapDisplay.h>
#include <treetypeid.h>
using namespace Regards::Filter;

CCloudsFilter::CCloudsFilter()
{
	libelleCloudsFrequency = CLibResource::LoadStringFromResource(L"LBLCLOUDSFREQUENCY", 1);
	libelleCloudsAmplitude = CLibResource::LoadStringFromResource(L"LBLCLOUDSAMPLITUDE", 1);
	libelleCloudsColorFront = CLibResource::LoadStringFromResource(L"LBLCLOUDSCOLORFRONT", 1);
	libelleCloudsColorBack = CLibResource::LoadStringFromResource(L"LBLCLOUDSCOLORBACK", 1);
	libelleEffectIntensity = CLibResource::LoadStringFromResource(L"LBLEFFECTINTENSITY", 1);
}

CCloudsFilter::~CCloudsFilter()
{
}

int CCloudsFilter::TypeApplyFilter()
{
	return 2;
}

bool CCloudsFilter::IsOpenCLCompatible()
{
	return false;
}

wxString CCloudsFilter::GetFilterLabel()
{
	return CLibResource::LoadStringFromResource("LBLfilterClouds", 1);
}

int CCloudsFilter::GetNameFilter()
{
	return IDM_FILTRE_CLOUDS;
}

int CCloudsFilter::GetTypeFilter()
{
	return SPECIAL_EFFECT; // 
}

void CCloudsFilter::Filter(CEffectParameter* effectParameter, cv::Mat& source, const wxString& filename,
                           IFiltreEffectInterface* filtreInterface)
{
	auto cloudsEffectParameter = static_cast<CCloudsEffectParameter*>(effectParameter);

	this->source = source;
	this->filename = filename;
	vector<int> elementFreq;
	for (auto i = 0; i < 101; i++)
		elementFreq.push_back(i);

	vector<int> elementColor;
	for (auto i = 0; i < 256; i++)
		elementColor.push_back(i);

	filtreInterface->AddTreeInfos(libelleEffectIntensity, new CTreeElementValueInt(cloudsEffectParameter->transparency),
	                              &elementFreq);
	filtreInterface->AddTreeInfos(libelleCloudsFrequency, new CTreeElementValueInt(cloudsEffectParameter->frequence),
	                              &elementFreq);
	filtreInterface->AddTreeInfos(libelleCloudsAmplitude, new CTreeElementValueInt(cloudsEffectParameter->amplitude),
	                              &elementFreq);

	filtreInterface->AddTreeInfos(libelleCloudsColorFront, new CTreeElementValueColor(cloudsEffectParameter->ConvertScalarToWxColour(cloudsEffectParameter->colorFront, effectParameter->opacity)), nullptr, 5, TYPE_COLOR);
	filtreInterface->AddTreeInfos(libelleCloudsColorBack, new CTreeElementValueColor(cloudsEffectParameter->ConvertScalarToWxColour(cloudsEffectParameter->colorBack, effectParameter->opacity)), nullptr, 5, TYPE_COLOR);
}

void CCloudsFilter::FilterChangeParam(CEffectParameter* effectParameter, CTreeElementValue* valueData,
                                      const wxString& key)
{
	auto cloudsEffectParameter = static_cast<CCloudsEffectParameter*>(effectParameter);

	auto value = static_cast<CTreeElementValueInt*>(valueData);
	//Video Parameter
	if (key == libelleEffectIntensity)
	{
		cloudsEffectParameter->transparency = value->GetValue();
	}
	else if (key == libelleCloudsFrequency)
	{
		cloudsEffectParameter->frequence = value->GetValue();
	}
	else if (key == libelleCloudsAmplitude)
	{
		cloudsEffectParameter->amplitude = value->GetValue();
	}
	else if (key == L"Clouds.Octaves")
	{
		cloudsEffectParameter->octave = value->GetValue();
	}
	else if (key == libelleCloudsColorFront && valueData->GetType() == 4) {
		wxColour c = static_cast<CTreeElementValueColor*>(valueData)->GetValue();
		cloudsEffectParameter->colorFront = cv::Scalar(c.Blue(), c.Green(), c.Red(), effectParameter->opacity);
	}
	else if (key == libelleCloudsColorBack && valueData->GetType() == 4) {
		wxColour c = static_cast<CTreeElementValueColor*>(valueData)->GetValue();
		cloudsEffectParameter->colorBack = cv::Scalar(c.Blue(), c.Green(), c.Red(), effectParameter->opacity);
	}
}

void CCloudsFilter::RenderEffect(CFiltreEffet* filtreEffet, CEffectParameter* effectParameter, const bool& preview)
{
	if (effectParameter != nullptr && filtreEffet != nullptr)
	{
		auto cloudsParameter = static_cast<CCloudsEffectParameter*>(effectParameter);
		filtreEffet->CloudsFilter(cloudsParameter->colorFront, cloudsParameter->colorBack, cloudsParameter->amplitude,
		                          cloudsParameter->frequence, cloudsParameter->octave, cloudsParameter->transparency);
	}
}

bool CCloudsFilter::NeedPreview()
{
	return true;
}

CEffectParameter* CCloudsFilter::GetEffectPointer()
{
	return new CCloudsEffectParameter();
}

CEffectParameter* CCloudsFilter::GetDefaultEffectParameter()
{
	auto clouds = new CCloudsEffectParameter();
	clouds->colorFront = cv::Scalar(0, 0, 0);
	clouds->colorBack = cv::Scalar(255, 255, 255);
	clouds->amplitude = 1;
	clouds->frequence = 65;
	clouds->octave = 8;
	clouds->transparency = 50;
	return clouds;
}


bool CCloudsFilter::IsSourcePreview()
{
	return true;
}


void CCloudsFilter::ApplyPreviewEffectSource(CEffectParameter* effectParameter, IBitmapDisplay* bitmapViewer,
                                             CFiltreEffet* filtreEffet, CDraw* dessing)
{

	if (effectParameter != nullptr && !source.empty())
	{
		CImageLoadingFormat image;
		image.SetPicture(source);

		auto filtre = std::make_unique<CFiltreEffet>(bitmapViewer->GetBackColor(), nullptr, &image);
		auto cloudsParameter = static_cast<CCloudsEffectParameter*>(effectParameter);
		filtre->CloudsFilter(cloudsParameter->colorFront, cloudsParameter->colorBack, cloudsParameter->amplitude,
		                     cloudsParameter->frequence, cloudsParameter->octave, cloudsParameter->transparency);
		auto imageLoad = std::make_unique<CImageLoadingFormat>();
		cv::Mat mat = filtre->GetBitmap(true);
		imageLoad->SetPicture(mat);
		filtreEffet->SetBitmap(imageLoad.get());
	}
}


void CCloudsFilter::ApplyPreviewEffect(CEffectParameter* effectParameter, IBitmapDisplay* bitmapViewer,
                                       CFiltreEffet* filtreEffet, CDraw* m_cDessin, int& widthOutput, int& heightOutput)
{
}


CImageLoadingFormat* CCloudsFilter::ApplyEffect(CEffectParameter* effectParameter, IBitmapDisplay* bitmapViewer)
{
	CImageLoadingFormat* imageLoad = nullptr;
	if (effectParameter != nullptr && !source.empty() && bitmapViewer != nullptr)
	{
		auto cloudsParameter = static_cast<CCloudsEffectParameter*>(effectParameter);

		CImageLoadingFormat image;
		image.SetPicture(source);
		image.RotateExif(orientation);
		auto filtre = std::make_unique<CFiltreEffet>(bitmapViewer->GetBackColor(), nullptr, &image);
		filtre->CloudsFilter(cloudsParameter->colorFront, cloudsParameter->colorBack, cloudsParameter->amplitude,
		                     cloudsParameter->frequence, cloudsParameter->octave, cloudsParameter->transparency);
		imageLoad = new CImageLoadingFormat();
		cv::Mat mat = filtre->GetBitmap(true);
		imageLoad->SetPicture(mat);
	}

	return imageLoad;
}
