#include <header.h>
#include "RectangleFilter.h"
#include "EffectParameter.h"
#include <LibResource.h>
#include <FiltreEffet.h>
#include <Draw.h>
#include <ImageLoadingFormat.h>
#include <BitmapDisplay.h>
#include <effect_id.h>
#include "RectangleFilterParameter.h"
#include <Metadata.h>
#include <treetypeid.h>
#include <opencv2/imgproc.hpp>
#include "RectangleDraw.h"

using namespace Regards::Filter;
using namespace cv;

CRectangleFilter::CRectangleFilter()
{
	libelleShapeType = CLibResource::LoadStringFromResource("LBLEFFECTSHAPETYPE", 1); 

	libellePenSize = CLibResource::LoadStringFromResource("LBLEFFECTPENSIZE", 1);
	libelleColor = CLibResource::LoadStringFromResource("LBLEFFECTCOLOR", 1);
	libelleTypeBrush = CLibResource::LoadStringFromResource("LBLEFFECTBRUSH", 1);
	libelleOpacity = CLibResource::LoadStringFromResource("LBLEFFECTOPACITY", 1);

	libelleIsFilled = CLibResource::LoadStringFromResource("LBLEFFECTFILLED", 1);
}
	


CRectangleFilter::~CRectangleFilter() {}

wxColour CRectangleFilter::ConvertScalarToWxColour(const cv::Scalar& scalar_color, int alpha)
{
	return wxColour(static_cast<int>(scalar_color[2]), static_cast<int>(scalar_color[1]), static_cast<int>(scalar_color[0]), alpha);
}

void CRectangleFilter::AddMetadataElement(vector<CMetadata>& element, wxString value, int key)
{
	CMetadata linear;
	linear.value = value;
	linear.depth = key;
	element.push_back(linear);
}

CEffectParameter* CRectangleFilter::GetEffectPointer() { return new CRectangleFilterParameter(); }
CDraw* CRectangleFilter::GetDrawingPt() { return new Regards::FiltreEffet::CRectangleDraw(); }
int CRectangleFilter::GetTypeFilter() { return IDM_RECTANGLEFILTER; } // Garder ID ou mettre à jour effect_id.h
int CRectangleFilter::GetNameFilter() { return IDM_RECTANGLEFILTER; }
wxString CRectangleFilter::GetFilterLabel() { return CLibResource::LoadStringFromResource("LBLRECTANGLEFILTER", 1); }

void CRectangleFilter::Filter(CEffectParameter* effectParameter, cv::Mat& source, const wxString& filename, IFiltreEffectInterface* filtreInterface)
{
	CRectangleFilterParameter* param = (CRectangleFilterParameter*)effectParameter;
	this->source = source;
	this->filename = filename;

	// Choix du dessin
	vector<CMetadata> shapeOptions;
	AddMetadataElement(shapeOptions, CLibResource::LoadStringFromResource("LBLRECTANGLE", 1) , SHAPE_RECTANGLE);
	AddMetadataElement(shapeOptions, CLibResource::LoadStringFromResource("LBLCIRCLE", 1) , SHAPE_CIRCLE);
	AddMetadataElement(shapeOptions, CLibResource::LoadStringFromResource("LBLLINE", 1), SHAPE_LINE);
	filtreInterface->AddTreeInfos(libelleShapeType, new CTreeElementValueInt(param->shapeType), &shapeOptions, 3, TYPE_COMBOBOX);

	// Taille du trait
	vector<int> elementSize;
	for (auto i = 1; i < 50; i++) elementSize.push_back(i);
	filtreInterface->AddTreeInfos(libellePenSize, new CTreeElementValueInt(param->penSize), &elementSize);

	// Couleur
	filtreInterface->AddTreeInfos(libelleColor, new CTreeElementValueColor(ConvertScalarToWxColour(param->color, effectParameter->opacity)), &elementSize, TYPE_COLOR, TYPE_COLOR);
		

	// Forme du trait
	vector<CMetadata> brushOptions;
	AddMetadataElement(brushOptions, CLibResource::LoadStringFromResource("LBLBRUSHSTD", 1), 0);
	AddMetadataElement(brushOptions, CLibResource::LoadStringFromResource("LBLBRUSHSQUARE", 1), 1);
	AddMetadataElement(brushOptions, CLibResource::LoadStringFromResource("LBLBRUSHDASHED", 1), 2);
	filtreInterface->AddTreeInfos(libelleTypeBrush, new CTreeElementValueInt(param->typeBrush), &brushOptions, 3, TYPE_COMBOBOX);

	// Transparence
	vector<int> elementOpacity;
	for (auto i = 0; i <= 255; i++) elementOpacity.push_back(i);
	filtreInterface->AddTreeInfos(libelleOpacity, new CTreeElementValueInt(param->opacity), &elementOpacity);


	filtreInterface->AddTreeInfos(libelleIsFilled, new CTreeElementValueInt(param->isFilled),
		&param->isFilled, 2, 2);

}

void CRectangleFilter::FilterChangeParam(CEffectParameter* effectParameter, CTreeElementValue* valueData, const wxString& key)
{
	auto param = static_cast<CRectangleFilterParameter*>(effectParameter);

	if (key == libelleShapeType && valueData->GetType() == TYPE_ELEMENT_INT)
		param->shapeType = static_cast<CTreeElementValueInt*>(valueData)->GetValue();
	else if (key == libellePenSize && valueData->GetType() == TYPE_ELEMENT_INT)
		param->penSize = static_cast<CTreeElementValueInt*>(valueData)->GetValue();
	else if (key == libelleColor && valueData->GetType() == 4) {
		wxColour c = static_cast<CTreeElementValueColor*>(valueData)->GetValue();
		param->color = cv::Scalar(c.Red(), c.Green(), c.Blue(), effectParameter->opacity);
	}
	else if (key == libelleTypeBrush && valueData->GetType() == TYPE_ELEMENT_INT)
		param->typeBrush = static_cast<CTreeElementValueInt*>(valueData)->GetValue();
	else if (key == libelleOpacity && valueData->GetType() == TYPE_ELEMENT_INT)
		param->opacity = static_cast<CTreeElementValueInt*>(valueData)->GetValue();
	else if (key == libelleIsFilled && valueData->GetType() == TYPE_ELEMENT_BOOL)
		param->isFilled = (static_cast<CTreeElementValueBool*>(valueData)->GetValue());
}

void CRectangleFilter::DrawShapeOnMat(cv::Mat& matrix, SShapeTrace& shape, bool rgba)
{
	int thickness = shape.isFilled ? -1 : shape.penSize * 2;
	if (shape.shapeType == SHAPE_LINE) thickness = shape.penSize * 2; // Une ligne ne peut pas être "remplie"

	int lineStyle = (shape.typeBrush == 1) ? cv::LINE_8 : cv::LINE_AA;
	if (shape.typeBrush == 2) lineStyle = cv::LINE_8; // Pointillés forcés en standard pour le calcul du saut

	cv::Point p1(shape.startPoint.x, shape.startPoint.y);
	cv::Point p2(shape.endPoint.x, shape.endPoint.y);

	cv::Scalar color = shape.color;

	if(!rgba)
		color = cv::Scalar(shape.color[2], shape.color[1], shape.color[0]); // Ignore alpha if not RGBA

	if (shape.shapeType == SHAPE_RECTANGLE) {
		cv::rectangle(matrix, p1, p2, color, thickness, lineStyle);
	}
	else if (shape.shapeType == SHAPE_CIRCLE) {
		double radius = cv::norm(p1 - p2);
		cv::circle(matrix, p1, static_cast<int>(radius), color, thickness, lineStyle);
	}
	else if (shape.shapeType == SHAPE_LINE) {
		if (shape.typeBrush == 2) {
			// Rendu basique d'une ligne pointillée en OpenCV
			cv::LineIterator it(matrix, p1, p2, 8);
			for (int i = 0; i < it.count; i++, ++it) {
				if (i % 10 < 5) { // Dessine 5 pixels, saute 5 pixels
					matrix.at<cv::Vec4b>(it.pos()) = cv::Vec4b(color[0], color[1], color[2], color[3]);
				}
			}
		}
		else {
			cv::line(matrix, p1, p2, color, thickness, lineStyle);
		}
	}
}

void CRectangleFilter::RenderEffect(CFiltreEffet* filtreEffet, CEffectParameter* effectParameter, const bool& preview)
{
	auto param = static_cast<CRectangleFilterParameter*>(effectParameter);
	if (!param->apply) return;

	CImageLoadingFormat* imageLoad = new CImageLoadingFormat();
	cv::Mat picture = filtreEffet->GetBitmap(true);
	imageLoad->SetPicture(picture);
	cv::Mat& matrix = imageLoad->GetMatImage();

	try {
		for (auto& shape : param->listShapes) {
			if (shape.opacity >= 255) {
				DrawShapeOnMat(matrix, shape, false);
			}
			else if (shape.opacity > 0) {
				cv::Mat overlay = matrix.clone();
				DrawShapeOnMat(overlay, shape, false);
				double alpha = shape.opacity / 255.0;
				cv::addWeighted(overlay, alpha, matrix, 1.0 - alpha, 0, matrix);
			}
		}
	}
	catch (cv::Exception& e) {
		std::cout << "Exception: " << e.what() << std::endl;
	}
	filtreEffet->SetBitmap(imageLoad);
}

CImageLoadingFormat* CRectangleFilter::ApplyEffect(CEffectParameter* effectParameter, IBitmapDisplay* bitmapViewer)
{
	auto param = static_cast<CRectangleFilterParameter*>(effectParameter);
	if (source.empty()) return nullptr;

	CImageLoadingFormat* imageLoad = new CImageLoadingFormat();
	imageLoad->SetPicture(source);
	cv::Mat& matrix = imageLoad->GetMatImage();

	try {
		for (auto& shape : param->listShapes) {
			if (shape.opacity >= 255) {
				DrawShapeOnMat(matrix, shape, false);
			}
			else if (shape.opacity > 0) {
				cv::Mat overlay = matrix.clone();
				DrawShapeOnMat(overlay, shape, false);
				double alpha = shape.opacity / 255.0;
				cv::addWeighted(overlay, alpha, matrix, 1.0 - alpha, 0, matrix);
			}
		}
	}
	catch (cv::Exception& e) {
		std::cout << "Exception: " << e.what() << std::endl;
	}

	param->apply = true;
	return imageLoad;
}

void CRectangleFilter::Drawing(cv::Mat& matrix, IBitmapDisplay* bitmapViewer, CDraw* m_cDessin)
{
	if (matrix.empty() || m_cDessin == nullptr || bitmapViewer == nullptr) return;
	m_cDessin->DessinerSurMat(matrix, bitmapViewer->GetHPos(), bitmapViewer->GetVPos(), bitmapViewer->GetRatio());

	auto param = static_cast<CRectangleFilterParameter*>(bitmapViewer->GetEffectPointer());
	auto rectDraw = static_cast<Regards::FiltreEffet::CRectangleDraw*>(m_cDessin);

	if (param && rectDraw) {
		param->listShapes.clear();
		for (const auto& s : rectDraw->GetTousLesTraces()) {
			SShapeTrace shapeOpenCV;
			shapeOpenCV.startPoint = s.startPoint;
			shapeOpenCV.endPoint = s.endPoint;
			shapeOpenCV.shapeType = s.shapeType;
			shapeOpenCV.penSize = s.penSize;
			shapeOpenCV.typeBrush = s.typeBrush;
			shapeOpenCV.opacity = s.opacity;
			shapeOpenCV.isFilled = s.isFilled;
			shapeOpenCV.color = cv::Scalar(s.color.Blue(), s.color.Green(), s.color.Red(), param->opacity);
			param->listShapes.push_back(shapeOpenCV);
		}
	}
}
