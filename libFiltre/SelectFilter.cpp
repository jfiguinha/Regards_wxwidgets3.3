#include <header.h>
#include "SelectFilter.h"
#include "EffectParameter.h"
#include <LibResource.h>
#include <FiltreEffet.h>
#include <Draw.h>
#include <ImageLoadingFormat.h>
#include <BitmapDisplay.h>
#include <effect_id.h>
#include "SelectFilterParameter.h"
#include <Metadata.h>
#include <treetypeid.h>
#include <opencv2/imgproc.hpp>
#include "SelectDraw.h"

using namespace Regards::Filter;
using namespace cv;

CSelectFilter::CSelectFilter()
{
	libelleSelectType = CLibResource::LoadStringFromResource("LBLEFFECTSELECTTYPE", 1);
	libelleTolerance = CLibResource::LoadStringFromResource("LBLTOLERANCE", 1);
}

CSelectFilter::~CSelectFilter() {}

wxColour CSelectFilter::ConvertScalarToWxColour(const cv::Scalar& scalar_color, int alpha)
{
	return wxColour(static_cast<int>(scalar_color[2]), static_cast<int>(scalar_color[1]), static_cast<int>(scalar_color[0]), alpha);
}

void CSelectFilter::AddMetadataElement(vector<CMetadata>& element, wxString value, int key)
{
	CMetadata linear;
	linear.value = value;
	linear.key = value;
	linear.depth = key;
	element.push_back(linear);
}

CEffectParameter* CSelectFilter::GetEffectPointer() { return new CSelectFilterParameter(); }
CDraw* CSelectFilter::GetDrawingPt() { return new Regards::FiltreEffet::CSelectDraw(); }
int CSelectFilter::GetTypeFilter() { return IDM_SELECTFILTER; }
int CSelectFilter::GetNameFilter() { return IDM_SELECTFILTER; }
wxString CSelectFilter::GetFilterLabel() { return CLibResource::LoadStringFromResource("LBLSELECTFILTER", 1); }

void CSelectFilter::Filter(CEffectParameter* effectParameter, cv::Mat& source, const wxString& filename, IFiltreEffectInterface* filtreInterface)
{
	CSelectFilterParameter* param = (CSelectFilterParameter*)effectParameter;
	this->source = source;
	this->filename = filename;

	// Seule la ComboBox du type de sélection apparaît dans l'arbre
	vector<CMetadata> selectOptions;
	AddMetadataElement(selectOptions, CLibResource::LoadStringFromResource("LBLSELECTRECT", 1), SELECT_RECTANGLE);
	AddMetadataElement(selectOptions, CLibResource::LoadStringFromResource("LBLSELECTELLIPSE", 1), SELECT_ELLIPSE);
	AddMetadataElement(selectOptions, CLibResource::LoadStringFromResource("LBLSELECTLASSO", 1), SELECT_LASSO);
	filtreInterface->AddTreeInfos(libelleSelectType, new CTreeElementValueInt(param->selectType), &selectOptions, 3, TYPE_COMBOBOX);
}

void CSelectFilter::FilterChangeParam(CEffectParameter* effectParameter, CTreeElementValue* valueData, const wxString& key)
{
	auto param = static_cast<CSelectFilterParameter*>(effectParameter);

	if (key == libelleSelectType && valueData->GetType() == TYPE_ELEMENT_INT)
		param->selectType = static_cast<CTreeElementValueInt*>(valueData)->GetValue();
	else if (key == libelleTolerance && valueData->GetType() == TYPE_ELEMENT_INT)
	{
		param->tolerancePercent = static_cast<CTreeElementValueInt*>(valueData)->GetValue(); // Capture du pourcentage
	}
}

void CSelectFilter::RenderEffect(CFiltreEffet* filtreEffet, CEffectParameter* effectParameter, const bool& preview)
{
}

CImageLoadingFormat* CSelectFilter::ApplyEffect(CEffectParameter* effectParameter, IBitmapDisplay* bitmapViewer)
{
	auto param = static_cast<CSelectFilterParameter*>(effectParameter);
	if (source.empty()) return nullptr;

	CImageLoadingFormat* imageLoad = new CImageLoadingFormat();
	imageLoad->SetPicture(source);
	param->apply = true;
	return imageLoad;
}


void CSelectFilter::Drawing(wxMemoryDC* dc, IBitmapDisplay* bitmapViewer, CDraw* m_cDessin)
{
	if (!m_cDessin || !dc || !bitmapViewer) return;

	auto param = static_cast<CSelectFilterParameter*>(bitmapViewer->GetEffectPointer());
	auto selectDraw = static_cast<Regards::FiltreEffet::CSelectDraw*>(m_cDessin);

	if (param && selectDraw) {
		selectDraw->SetCurrentSelectParams(param->selectType);
		wxColour color(0, 0, 0);
		m_cDessin->Dessiner(dc, bitmapViewer->GetHPos(), bitmapViewer->GetVPos(), bitmapViewer->GetRatio(), color, color, color, 1);

		param->listSelections.clear();
		for (const auto& s : selectDraw->GetTousLesTraces()) {
			SSelectionTrace selectOpenCV;
			selectOpenCV.startPoint = s.startPoint;
			selectOpenCV.endPoint = s.endPoint;
			selectOpenCV.points = s.points;
			selectOpenCV.selectType = s.selectType;
			selectOpenCV.penSize = 4;
			selectOpenCV.color = cv::Scalar(0, 0, 0, 0);
			param->listSelections.push_back(selectOpenCV);
		}
	}
}
