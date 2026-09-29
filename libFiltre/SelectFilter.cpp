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
	AddMetadataElement(selectOptions, CLibResource::LoadStringFromResource("LBLSELECTMAGICWAND", 1), SELECT_MAGICWAND);
	filtreInterface->AddTreeInfos(libelleSelectType, new CTreeElementValueInt(param->selectType), &selectOptions, 3, TYPE_COMBOBOX);

	vector<int> elementTolerance;
	for (auto i = 0; i <= 100; i++) // Remplissage de 0 à 100 %
		elementTolerance.push_back(i);

	filtreInterface->AddTreeInfos(libelleTolerance, new CTreeElementValueInt(param->tolerancePercent), &elementTolerance);
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

void CSelectFilter::DrawSelectionOnMat(cv::Mat& matrix, SSelectionTrace& selection)
{
	int thickness = 1; // Toujours fin contour
	int lineStyle = cv::LINE_AA;

	cv::Point p1(selection.startPoint.x, selection.startPoint.y);
	cv::Point p2(selection.endPoint.x, selection.endPoint.y);

	if (selection.selectType == SELECT_RECTANGLE) {
		cv::rectangle(matrix, p1, p2, selection.color, thickness, lineStyle);
	}
	else if (selection.selectType == SELECT_ELLIPSE) {
		cv::Point center((p1.x + p2.x) / 2, (p1.y + p2.y) / 2);
		cv::Size axes(std::abs(p2.x - p1.x) / 2, std::abs(p2.y - p1.y) / 2);
		cv::ellipse(matrix, center, axes, 0, 0, 360, selection.color, thickness, lineStyle);
	}
	else if (selection.selectType == SELECT_LASSO && selection.points.size() > 1) {
		std::vector<cv::Point> cvPoints;
		for (const auto& pt : selection.points) {
			cvPoints.push_back(cv::Point(pt.x, pt.y));
		}
		std::vector<std::vector<cv::Point>> ppt = { cvPoints };
		cv::polylines(matrix, ppt, true, selection.color, thickness, lineStyle);
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
void CSelectFilter::Drawing(cv::Mat& matrix, IBitmapDisplay* bitmapViewer, CDraw* m_cDessin)
{
	if (matrix.empty() || m_cDessin == nullptr || bitmapViewer == nullptr) return;

	auto param = static_cast<CSelectFilterParameter*>(bitmapViewer->GetEffectPointer());
	auto selectDraw = static_cast<Regards::FiltreEffet::CSelectDraw*>(m_cDessin);

	if (param && selectDraw)
	{
		long hScroll = bitmapViewer->GetHPos();
		long vScroll = bitmapViewer->GetVPos();
		float ratio = bitmapViewer->GetRatio();

		// --- LOGIQUE SPÉCIFIQUE À LA BAGUETTE MAGIQUE ---
		if (param->selectType == SELECT_MAGICWAND)
		{
			wxPoint clickPt;
			selectDraw->GetStartPoint(clickPt); // Position réelle dans l'image brute

			// Si aucun clic n'a encore été réalisé, on s'arrête
			if (clickPt.x < 0 || clickPt.y < 0) return;

			try
			{
				// Conversion des coordonnées réelles de l'image en coordonnées d'affichage écran
				int screenX = static_cast<int>(m_cDessin->XDrawingPosition(static_cast<float>(clickPt.x), hScroll, ratio));
				int screenY = static_cast<int>(m_cDessin->YDrawingPosition(static_cast<float>(clickPt.y), vScroll, ratio));

				// Vérification que le point converti est bien à l'intérieur de la matrice d'affichage
				if (screenX >= 0 && screenY >= 0 && screenX < matrix.cols && screenY < matrix.rows)
				{
					// 1. Initialisation du masque binaire OpenCV (+2 pixels de bordure requis par floodFill)
					cv::Mat mask = cv::Mat::zeros(matrix.rows + 2, matrix.cols + 2, CV_8UC1);

					// 2. Conversion du pourcentage de tolérance en intensité OpenCV (0-255)
					int activeToleranceValue = static_cast<int>((param->tolerancePercent * 255.0f) / 100.0f);
					cv::Scalar toleranceInterval(activeToleranceValue, activeToleranceValue, activeToleranceValue, activeToleranceValue);

					// 3. Remplissage par diffusion sur la matrice d'affichage à l'écran
					cv::floodFill(matrix, mask, cv::Point(screenX, screenY), cv::Scalar(255),
						nullptr, toleranceInterval, toleranceInterval, 4 | cv::FLOODFILL_MASK_ONLY | (255 << 8));

					// 4. Extraction des contours géométriques du masque écran
					std::vector<std::vector<cv::Point>> contours;
					cv::findContours(mask, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);

					param->listSelections.clear();
					selectDraw->Reset();

					// Couleur de rendu fixe pour la sélection (Blanc pur par exemple)
					cv::Scalar selectionColor(0, 0, 0, 0);
					int thickness = 1;

					// 5. CONVERSION INVERSE, SAUVEGARDE ET TRACÉ REEL SUR MATRIX
					for (const auto& contour : contours)
					{
						if (contour.size() > 1)
						{
							// --- CORRECTION : DESSIN PHYSIQUE DU CONTOUR SUR LA MATRICE OPENCV ---
							// Puisque 'contour' est déjà exprimé aux coordonnées de l'écran (matrice d'affichage),
							// on peut directement l'appliquer sur 'matrix' pour que l'afficheur OpenGL/2D le reçoive
							std::vector<std::vector<cv::Point>> ppt = { contour };
							cv::polylines(matrix, ppt, true, selectionColor, thickness, cv::LINE_AA);

							// Conversion inverse pour sauvegarder les coordonnées réelles dans les structures
							SSelectionTrace selectOpenCV;
							selectOpenCV.selectType = SELECT_MAGICWAND;
							selectOpenCV.penSize = 1;
							selectOpenCV.color = selectionColor;

							std::vector<wxPoint> pointsReelsPourEcran;

							for (const auto& cvPt : contour)
							{
								int currentScreenX = cvPt.x - 1;
								int currentScreenY = cvPt.y - 1;

								float realX = m_cDessin->XRealPosition(static_cast<float>(currentScreenX), hScroll, ratio);
								float realY = m_cDessin->YRealPosition(static_cast<float>(currentScreenY), vScroll, ratio);

								wxPoint ptReal(static_cast<int>(realX), static_cast<int>(realY));

								selectOpenCV.points.push_back(ptReal);
								pointsReelsPourEcran.push_back(ptReal);
							}

							// Enregistrement persistant des données
							param->listSelections.push_back(selectOpenCV);
							selectDraw->InjectExternalShape(SELECT_MAGICWAND, pointsReelsPourEcran);
						}
					}
				}
			}
			catch (cv::Exception& e)
			{
				std::cout << "Exception Baguette Magique : " << e.what() << std::endl;
			}
		}
		else
		{
			// --- LOGIQUE STANDARD RECTANGLE / ELLIPSE / LASSO ---
			m_cDessin->DessinerSurMat(matrix, hScroll, vScroll, ratio);

			param->listSelections.clear();
			for (const auto& s : selectDraw->GetTousLesTraces())
			{
				SSelectionTrace selectOpenCV;
				selectOpenCV.startPoint = s.startPoint;
				selectOpenCV.endPoint = s.endPoint;
				selectOpenCV.points = s.points;
				selectOpenCV.selectType = s.selectType;
				selectOpenCV.penSize = 1;
				selectOpenCV.color = cv::Scalar(255, 255, 255, 255);
				param->listSelections.push_back(selectOpenCV);
			}
		}
	}
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
