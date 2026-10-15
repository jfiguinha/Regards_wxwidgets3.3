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
using namespace Regards::Window;
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
{}

CImageLoadingFormat* CSelectFilter::ApplyEffect(CEffectParameter* effectParameter, IBitmapDisplay* bitmapViewer)
{
	auto param = static_cast<CSelectFilterParameter*>(effectParameter);
	if (source.empty()) return nullptr;

	CImageLoadingFormat* imageLoad = new CImageLoadingFormat();
	imageLoad->SetPicture(source);
	param->apply = true;
	return imageLoad;
}

// MODIFICATION : Utilisation de cv::Mat au lieu de wxMemoryDC pour s'aligner sur CRectangleFilter
void CSelectFilter::Drawing(cv::Mat& matrix, IBitmapDisplay* bitmapViewer, CDraw* m_cDessin)
{
	if (matrix.empty() || m_cDessin == nullptr || bitmapViewer == nullptr) return;

	auto param = static_cast<CSelectFilterParameter*>(bitmapViewer->GetEffectPointer());
	auto selectDraw = static_cast<Regards::FiltreEffet::CSelectDraw*>(m_cDessin);

	// 1. Si le dessin demande une mise à jour des tracés depuis le paramètre partagé
	if (m_cDessin->GetUpdateTrace() && param && selectDraw)
	{
		// On réinitialise l'état du moteur de dessin
		selectDraw->Reset();

		// Si le paramètre global contient des sélections, on simule un clic pour réinitialiser
		if (!param->listSelections.empty())
		{
			//selectDraw->MouseDown(param);
			std::vector<Regards::FiltreEffet::SSelectionStyleDraw> list = selectDraw->GetTousLesTraces();
			// On vide la liste par défaut créée par MouseDown pour injecter nos données réelles
			// Note : Si l'architecture interdit l'accès en écriture directe sur m_tousLesTraces,
			// vous pouvez créer une méthode publique SetTousLesTraces() dans CSelectDraw.
			// Ici, nous reconstruisons les tracés via l'API géométrique standard.
			for (const auto& s : param->listSelections)
			{
				Regards::FiltreEffet::SSelectionStyleDraw selection;
				selection.startPoint = s.startPoint;
				selection.endPoint = s.endPoint;
				selection.points = s.points;
				selection.selectType = s.selectType;
				selection.penSize = s.penSize;

				// Reconversion cv::Scalar (BGR) vers wxColour (RGB)
				selection.color = wxColour(
					static_cast<unsigned char>(s.color[2]),
					static_cast<unsigned char>(s.color[1]),
					static_cast<unsigned char>(s.color[0])
				);
				selection.opacity = static_cast<int>(s.color[3]);
				selection.isFilled = false;

				selectDraw->m_tousLesTraces.push_back(selection);

				// Si m_tousLesTraces de CSelectDraw est accessible ou si vous disposez d'un accesseur non-const :
				// (Code basé sur l'alignement strict de la structure SSelectionStyleDraw)
				// selectDraw->AjouterTrace(selection); 
			}
		}
	}

	m_cDessin->SetUpdateTrace(false);

	// 2. Rendu matériel accéléré OpenCL sur la Matrice de type cv::UMat
	m_cDessin->DessinerSurMat(matrix, bitmapViewer->GetHPos(), bitmapViewer->GetVPos(), bitmapViewer->GetRatio());

	// 3. Sauvegarde et mise à jour vectorielle synchrone vers le paramètre de filtre
	if (param && selectDraw)
	{
		param->listSelections.clear();
		for (const auto& s : selectDraw->GetTousLesTraces())
		{
			SSelectionTrace selectOpenCV;
			selectOpenCV.startPoint = s.startPoint;
			selectOpenCV.endPoint = s.endPoint;
			selectOpenCV.points = s.points;
			selectOpenCV.selectType = s.selectType;
			selectOpenCV.penSize = s.penSize;

			// Stockage au format de couleur OpenCV natif (Scalar : B, G, R, Alpha)
			selectOpenCV.color = cv::Scalar(s.color.Blue(), s.color.Green(), s.color.Red(), s.opacity);
			param->listSelections.push_back(selectOpenCV);
		}
	}

}


// MODIFICATION : Utilisation de cv::UMat pour le pipeline de rendu OpenCL / GPU
void CSelectFilter::Drawing(cv::UMat& matrix, IBitmapDisplay* bitmapViewer, CDraw* m_cDessin)
{
	if (matrix.empty() || m_cDessin == nullptr || bitmapViewer == nullptr) return;

	auto param = static_cast<CSelectFilterParameter*>(bitmapViewer->GetEffectPointer());
	auto selectDraw = static_cast<Regards::FiltreEffet::CSelectDraw*>(m_cDessin);

	// 1. Si le dessin demande une mise à jour des tracés depuis le paramètre partagé
	if (m_cDessin->GetUpdateTrace() && param && selectDraw)
	{
		// On réinitialise l'état du moteur de dessin
		selectDraw->Reset();

		// Si le paramètre global contient des sélections, on simule un clic pour réinitialiser
		if (!param->listSelections.empty())
		{
			
			//std::vector<Regards::FiltreEffet::SSelectionStyleDraw> list = selectDraw->GetTousLesTraces();
			//selectDraw->MouseDown(param);

			// On vide la liste par défaut créée par MouseDown pour injecter nos données réelles
			// Note : Si l'architecture interdit l'accès en écriture directe sur m_tousLesTraces,
			// vous pouvez créer une méthode publique SetTousLesTraces() dans CSelectDraw.
			// Ici, nous reconstruisons les tracés via l'API géométrique standard.
			for (const auto& s : param->listSelections)
			{
				Regards::FiltreEffet::SSelectionStyleDraw selection;
				selection.startPoint = s.startPoint;
				selection.endPoint = s.endPoint;
				selection.points = s.points;
				selection.selectType = s.selectType;
				selection.penSize = s.penSize;

				// Reconversion cv::Scalar (BGR) vers wxColour (RGB)
				selection.color = wxColour(
					static_cast<unsigned char>(s.color[2]),
					static_cast<unsigned char>(s.color[1]),
					static_cast<unsigned char>(s.color[0])
				);
				selection.opacity = static_cast<int>(s.color[3]);
				selection.isFilled = false;

				selectDraw->m_tousLesTraces.push_back(selection);

				// Si m_tousLesTraces de CSelectDraw est accessible ou si vous disposez d'un accesseur non-const :
				// (Code basé sur l'alignement strict de la structure SSelectionStyleDraw)
				// selectDraw->AjouterTrace(selection); 
			}
		}
	}

	m_cDessin->SetUpdateTrace(false);

	// 2. Rendu matériel accéléré OpenCL sur la Matrice de type cv::UMat
	m_cDessin->DessinerSurMat(matrix, bitmapViewer->GetHPos(), bitmapViewer->GetVPos(), bitmapViewer->GetRatio());

	// 3. Sauvegarde et mise à jour vectorielle synchrone vers le paramètre de filtre
	if (param && selectDraw)
	{
		param->listSelections.clear();
		for (const auto& s : selectDraw->GetTousLesTraces())
		{
			SSelectionTrace selectOpenCV;
			selectOpenCV.startPoint = s.startPoint;
			selectOpenCV.endPoint = s.endPoint;
			selectOpenCV.points = s.points;
			selectOpenCV.selectType = s.selectType;
			selectOpenCV.penSize = s.penSize;

			// Stockage au format de couleur OpenCV natif (Scalar : B, G, R, Alpha)
			selectOpenCV.color = cv::Scalar(s.color.Blue(), s.color.Green(), s.color.Red(), s.opacity);
			param->listSelections.push_back(selectOpenCV);
		}
	}
}
