#include <header.h>
#include "PenFilter.h"
#include "EffectParameter.h"
#include <LibResource.h>
#include <FiltreEffet.h>
#include <Draw.h>
#include <ImageLoadingFormat.h>
#include <BitmapDisplay.h>
#include "FiltreEffetCPU.h"
#include <effect_id.h>
#include "PenEffectParameter.h"
#include <wx/busyinfo.h>
#include <Metadata.h>
#include <treetypeid.h>
// Inclusions indispensables
#include <opencv2/imgproc.hpp>
#include "PenDraw.h"
#include "PenStrokeRenderer.h"

using namespace Regards::Filter;
using namespace cv;

namespace
{
	// Applique tous les tracés sur l'image source (échelle 1, coordonnées image).
	// Même rendu que la preview (RenderPenStroke).
	template <typename Lines>
	void DrawLinesOnSource(cv::Mat& matrix, Lines& lines)
	{
		const auto identity = [](const wxPoint& point) { return cv::Point(point.x, point.y); };

		for (auto& ligne : lines)
		{
			if (ligne.points.empty()) continue;

			ligne.rgba = false;
			const cv::Scalar color = ligne.GetColorWithTransparancy();

			Regards::FiltreEffet::RenderPenStroke(matrix, ligne.points, ligne.penSize, ligne.typeBrush,
				ligne.opacity, color, 1.0f, identity);
		}
	}
}

CPenFilter::CPenFilter()
{
	libellePenSize = CLibResource::LoadStringFromResource("LBLEFFECTPENSIZE", 1);
	libelleColor = CLibResource::LoadStringFromResource("LBLEFFECTCOLOR", 1);
	libelleTypeBrush = CLibResource::LoadStringFromResource("LBLEFFECTBRUSH", 1);
	libelleOpacity = CLibResource::LoadStringFromResource("LBLEFFECTOPACITY", 1);
}

CPenFilter::~CPenFilter()
{}

wxColour  CPenFilter::ConvertScalarToWxColour(const cv::Scalar& scalar_color, int alpha)
{
	// Extraction des canaux OpenCV (Indices standard : 0 = Blue, 1 = Green, 2 = Red, 3 = Alpha)
	int blue = static_cast<int>(scalar_color[0]);
	int green = static_cast<int>(scalar_color[1]);
	int red = static_cast<int>(scalar_color[2]);

	return wxColour(red, green, blue, alpha);
}

void CPenFilter::AddMetadataElement(vector<CMetadata>& element, wxString value, int key)
{
	CMetadata linear;
	linear.value = value;
	linear.depth = key;
	element.push_back(linear);
}

CEffectParameter* CPenFilter::GetEffectPointer()
{
	return new CPenFilterParameter();
}

CDraw* CPenFilter::GetDrawingPt()
{
	return new Regards::FiltreEffet::CPenDraw();
}

void CPenFilter::Filter(CEffectParameter* effectParameter, cv::Mat& source, const wxString& filename,
	IFiltreEffectInterface* filtreInterface)
{
	this->source = source;
	this->filename = filename;
	CPenFilterParameter* penParam = (CPenFilterParameter*)effectParameter;

	vector<int> elementSize;
	for (auto i = 0; i < 50; i++)
		elementSize.push_back(i);

	// 3. CONFIGURATION ET AJOUT DE LA COMBOBOX POUR LE TYPE DE PINCEAU
	vector<CMetadata> brushOptions;
	AddMetadataElement(brushOptions, CLibResource::LoadStringFromResource("LBLBRUSHSTD", 1), 0);
	AddMetadataElement(brushOptions, CLibResource::LoadStringFromResource("LBLBRUSHSQUARE", 1), 1);
	AddMetadataElement(brushOptions, CLibResource::LoadStringFromResource("LBLBRUSHDASHED", 1), 2);

	filtreInterface->AddTreeInfos(libellePenSize,
		new CTreeElementValueInt(penParam->penSize), &elementSize);
	// Appel de AddTreeInfos en spécifiant TYPE_COMBOBOX (type = 4) pour l'affichage d'une liste déroulante
	filtreInterface->AddTreeInfos(libelleTypeBrush, new CTreeElementValueInt(penParam->typeBrush), &brushOptions, 3, TYPE_COMBOBOX);

	/*
	// Remplissage des valeurs du curseur de 0 à 255
	vector<int> elementOpacity;
	for (auto i = 0; i <= 255; i++)
		elementOpacity.push_back(i);

	// Ajout du curseur d'opacité dans l'arbre
	filtreInterface->AddTreeInfos(libelleOpacity, new CTreeElementValueInt(penParam->opacity), &elementOpacity);

	*/
}

void CPenFilter::FilterChangeParam(CEffectParameter* effectParameter, CTreeElementValue* valueData,
	const wxString& key)
{
	auto penParameter = static_cast<CPenFilterParameter*>(effectParameter);

	if (key == libellePenSize && valueData->GetType() == TYPE_ELEMENT_INT)
	{
		auto intValue = static_cast<CTreeElementValueInt*>(valueData);
		penParameter->penSize = intValue->GetValue();
	}
	// --- INTERCEPTION DU CHANGEMENT DE LA COMBOBOX ---
	else if (key == libelleTypeBrush && valueData->GetType() == TYPE_ELEMENT_INT)
	{
		auto intValue = static_cast<CTreeElementValueInt*>(valueData);
		penParameter->typeBrush = intValue->GetValue(); // Sauvegarde de l'index de forme choisi (0, 1 ou 2)
	}
	else if (key == libelleOpacity && valueData->GetType() == TYPE_ELEMENT_INT)
	{
		auto intValue = static_cast<CTreeElementValueInt*>(valueData);
		penParameter->opacity = intValue->GetValue(); // Sauvegarde de la transparence choisie
	}

}

int CPenFilter::GetTypeFilter()
{
	return IDM_PENFILTER;
}

int CPenFilter::GetNameFilter()
{
	return IDM_PENFILTER;
}

wxString CPenFilter::GetFilterLabel()
{
	return CLibResource::LoadStringFromResource("LBLPENFILTER", 1);
}

void CPenFilter::RenderEffect(CFiltreEffet* filtreEffet, CEffectParameter* effectParameter, const bool& preview)
{
	//const wxString libelle = CLibResource::LoadStringFromResource(L"LBLBUSYINFO", 1);
	//wxBusyInfo wait(libelle, nullptr);

	CImageLoadingFormat* imageLoad = nullptr;
	auto penParameter = static_cast<CPenFilterParameter*>(effectParameter);

	if (penParameter->apply)
	{
		imageLoad = new CImageLoadingFormat();
		cv::Mat picture = filtreEffet->GetBitmap(true);
		imageLoad->SetPicture(picture);
		imageLoad->RotateExif(orientation);

		try
		{
			cv::Mat& matrix = imageLoad->GetMatImage();

			DrawLinesOnSource(matrix, penParameter->listLines);
		}
		catch (cv::Exception& e)
		{
			const char* err_msg = e.what();
			std::cout << "exception caught: " << err_msg << std::endl;
		}


		filtreEffet->SetBitmap(imageLoad);
	}
}

CImageLoadingFormat* CPenFilter::ApplyEffect(CEffectParameter* effectParameter, IBitmapDisplay* bitmapViewer)
{
	CImageLoadingFormat* imageLoad = nullptr;
	auto penParameter = static_cast<CPenFilterParameter*>(effectParameter);

	if (penParameter->listLines.empty()) {
		SLineTrace premierTrace;
		premierTrace.color = penParameter->color1;
		premierTrace.penSize = penParameter->penSize;
		premierTrace.typeBrush = penParameter->typeBrush; // <--- AJOUT CRUCIAL : On applique le type de brush courant
		penParameter->listLines.push_back(premierTrace);
	}


	if (!source.empty())
	{
		imageLoad = new CImageLoadingFormat();
		imageLoad->SetPicture(source);
		imageLoad->RotateExif(orientation);

		try
		{
			cv::Mat& matrix = imageLoad->GetMatImage();

			DrawLinesOnSource(matrix, penParameter->listLines);
		}
		catch (cv::Exception& e)
		{
			const char* err_msg = e.what();
			std::cout << "exception caught: " << err_msg << std::endl;
		}


		penParameter->apply = true;
	}

	return imageLoad;
}

void CPenFilter::Drawing(cv::Mat& matrix, IBitmapDisplay* bitmapViewer, CDraw* m_cDessin)
{
	if (matrix.empty() || m_cDessin == nullptr || bitmapViewer == nullptr) return;

	// 1. Récupération des paramètres de défilement (Scroll) et de zoom (Ratio) depuis le viewer
	int hpos = bitmapViewer->GetHPos();
	int vpos = bitmapViewer->GetVPos();
	float ratio = bitmapViewer->GetRatio();
	m_cDessin->DessinerSurMat(matrix, hpos, vpos, ratio);

	auto penDraw = static_cast<Regards::FiltreEffet::CPenDraw*>(m_cDessin);
	auto penParameter = static_cast<CPenFilterParameter*>(bitmapViewer->GetEffectPointer());

	if (penParameter != nullptr && penDraw != nullptr)
	{
		// On vide l'ancien conteneur pour y copier l'état exact et complet de l'écran
		penParameter->listLines.clear();

		// Récupération du tableau de structures m_tousLesTraces depuis CPenDraw
		const auto& tracesEcran = penDraw->GetTousLesTraces();

		for (const auto& traceEcran : tracesEcran)
		{
			SLineTrace ligneOpenCV;
			ligneOpenCV.points = traceEcran.points;
			ligneOpenCV.penSize = traceEcran.penSize;
			ligneOpenCV.typeBrush = traceEcran.typeBrush;
			ligneOpenCV.opacity = traceEcran.opacity; // Sauvegarde de l'opacité historique de la ligne

			// Conversion inverse : wxColour (RGB) vers cv::Scalar (BGR)
			ligneOpenCV.color = cv::Scalar(
				traceEcran.color.Blue(),
				traceEcran.color.Green(),
				traceEcran.color.Red(),
				traceEcran.opacity // Alpha figé du trait (et non l'opacité courante du paramètre)
			);

			// Stockage définitif pour le moteur OpenCV (RenderEffect / ApplyEffect)
			penParameter->listLines.push_back(ligneOpenCV);
		}
	}
}