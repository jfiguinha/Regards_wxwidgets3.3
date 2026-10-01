#include <header.h>
#include "MagicHandFilter.h"
#include "EffectParameter.h"
#include <LibResource.h>
#include <FiltreEffet.h>
#include <Draw.h>
#include <ImageLoadingFormat.h>
#include <BitmapDisplay.h>
#include <effect_id.h>
#include "MagicHandParameter.h"
#include "MagicHandDraw.h"
#include <Metadata.h>
#include <treetypeid.h>
#include <opencv2/imgproc.hpp>

using namespace Regards::Filter;
using namespace cv;

CMagicHandFilter::CMagicHandFilter()
{
	libelleTolerance = CLibResource::LoadStringFromResource("LBLTOLERANCE", 1);
}

CMagicHandFilter::~CMagicHandFilter() {}

CEffectParameter* CMagicHandFilter::GetEffectPointer() { return new CMagicHandParameter(); }
CDraw* CMagicHandFilter::GetDrawingPt() { return new Regards::FiltreEffet::CMagicHandDraw(); }
int CMagicHandFilter::GetTypeFilter() { return IDM_MAGICHANDFILTER; } // Assurez-vous d'avoir défini cet ID dans effect_id.h
int CMagicHandFilter::GetNameFilter() { return IDM_MAGICHANDFILTER; }
wxString CMagicHandFilter::GetFilterLabel() { return CLibResource::LoadStringFromResource("LBLMAGICHANDFILTER", 1); }

void CMagicHandFilter::Filter(CEffectParameter* effectParameter, cv::Mat& source, const wxString& filename, IFiltreEffectInterface* filtreInterface)
{
	this->source = source;
	this->filename = filename;
	auto param = static_cast<CMagicHandParameter*>(effectParameter);

	// Génération de la liste déroulante ou du curseur de tolérance (0 à 100 %)
	std::vector<int> elementTolerance;
	for (auto i = 0; i <= 100; i++)
		elementTolerance.push_back(i);

	filtreInterface->AddTreeInfos(libelleTolerance, new CTreeElementValueInt(param->tolerancePercent), &elementTolerance);
}

void CMagicHandFilter::FilterChangeParam(CEffectParameter* effectParameter, CTreeElementValue* valueData, const wxString& key)
{
	auto param = static_cast<CMagicHandParameter*>(effectParameter);

	if (key == libelleTolerance && valueData->GetType() == TYPE_ELEMENT_INT)
	{
		param->tolerancePercent = static_cast<CTreeElementValueInt*>(valueData)->GetValue();
	}
}

void CMagicHandFilter::RenderEffect(CFiltreEffet* filtreEffet, CEffectParameter* effectParameter, const bool& preview)
{
	// Logique de preview si vous désirez figer le traitement sur le calque OpenCV lors des calculs
}

CImageLoadingFormat* CMagicHandFilter::ApplyEffect(CEffectParameter* effectParameter, IBitmapDisplay* bitmapViewer)
{
	auto param = static_cast<CMagicHandParameter*>(effectParameter);
	if (source.empty()) return nullptr;

	CImageLoadingFormat* imageLoad = new CImageLoadingFormat();
	imageLoad->SetPicture(source);
	imageLoad->RotateExif(orientation);
	param->apply = true;
	return imageLoad;
}

void CMagicHandFilter::Drawing(cv::Mat& matrix, IBitmapDisplay* bitmapViewer, CDraw* m_cDessin)
{
	if (matrix.empty() || m_cDessin == nullptr || bitmapViewer == nullptr) return;

	auto magicDraw = static_cast<Regards::FiltreEffet::CMagicHandDraw*>(m_cDessin);
	auto param = static_cast<CMagicHandParameter*>(bitmapViewer->GetEffectPointer());

	if (magicDraw && param)
	{
		int hpos = bitmapViewer->GetHPos();
		int vpos = bitmapViewer->GetVPos();
		float ratio = bitmapViewer->GetRatio();

		// Appel du rendu ciblé sur la cv::Mat
		magicDraw->DessinerSurMat(matrix, hpos, vpos, ratio, param);
	}
}
