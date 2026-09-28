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

using namespace Regards::Filter;
using namespace cv;

CPenFilter::CPenFilter()
{
	libellePenSize = "Effect.Pen Size";
	libelleColor = "Effect.Color";
	libelleTypeBrush = "Effect.Brush";
}

CPenFilter::~CPenFilter()
{}

wxColour  CPenFilter::ConvertScalarToWxColour(const cv::Scalar& scalar_color)
{
	// Extraction des canaux OpenCV (Indices standard : 0 = Blue, 1 = Green, 2 = Red, 3 = Alpha)
	int blue = static_cast<int>(scalar_color[0]);
	int green = static_cast<int>(scalar_color[1]);
	int red = static_cast<int>(scalar_color[2]);
	int alpha = static_cast<int>(scalar_color[3]);

	// Si le canal alpha est à 0 (et que les autres couleurs ne le sont pas forcément), 
	// ou si OpenCV n'a pas initialisé le 4ème canal, on force l'opacité à 100% (255)
	if (alpha == 0 && (blue > 0 || green > 0 || red > 0))
	{
		alpha = 255;
	}
	else if (alpha == 0 && blue == 0 && green == 0 && red == 0)
	{
		// Cas particulier : si c'est du noir pur, assurez-vous qu'il ne soit pas transparent par erreur
		// (Sauf si vous gérez explicitement la transparence noire)
		alpha = 255;
	}

	// Retourne la wxColour construite au format attendu (Red, Green, Blue, Alpha)
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
	CPenFilterParameter* inpaintParam = (CPenFilterParameter*)effectParameter;

	vector<int> elementSize;
	for (auto i = 0; i < 50; i++)
		elementSize.push_back(i);

	// 3. CONFIGURATION ET AJOUT DE LA COMBOBOX POUR LE TYPE DE PINCEAU
	vector<CMetadata> brushOptions;
	AddMetadataElement(brushOptions, "Standard (Rond)", 0);
	AddMetadataElement(brushOptions, "Carré", 1);
	AddMetadataElement(brushOptions, "Pointillés", 2);


	filtreInterface->AddTreeInfos(libellePenSize,
		new CTreeElementValueInt(inpaintParam->penSize), &elementSize);
	filtreInterface->AddTreeInfos(libelleColor,
		new CTreeElementValueColor(ConvertScalarToWxColour(inpaintParam->color)), &elementSize, TYPE_COLOR, TYPE_COLOR);
	// Appel de AddTreeInfos en spécifiant TYPE_COMBOBOX (type = 4) pour l'affichage d'une liste déroulante
	filtreInterface->AddTreeInfos(libelleTypeBrush, new CTreeElementValueInt(inpaintParam->typeBrush), &brushOptions, 3, TYPE_COMBOBOX);

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
	else if (key == libelleColor && valueData->GetType() == 4) // TYPE_ELEMENT_COLOR
	{
		auto colorValue = static_cast<CTreeElementValueColor*>(valueData);
		wxColour c = colorValue->GetValue();
		penParameter->color = cv::Scalar(c.Blue(), c.Green(), c.Red(), c.Alpha());
	}
	// --- INTERCEPTION DU CHANGEMENT DE LA COMBOBOX ---
	else if (key == libelleTypeBrush && valueData->GetType() == TYPE_ELEMENT_INT)
	{
		auto intValue = static_cast<CTreeElementValueInt*>(valueData);
		penParameter->typeBrush = intValue->GetValue(); // Sauvegarde de l'index de forme choisi (0, 1 ou 2)
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
		imageLoad->SetPicture(filtreEffet->GetBitmap(true));
		imageLoad->RotateExif(orientation);

		try
		{
			cv::Mat& matrix = imageLoad->GetMatImage();

			for (const auto& ligne : penParameter->listLines)
			{
				if (ligne.points.empty()) continue;

				if (ligne.points.size() == 1)
				{
					cv::circle(matrix, cv::Point(ligne.points[0].x, ligne.points[0].y), ligne.penSize, ligne.color, -1);
				}
				else if (ligne.points.size() > 1)
				{
					for (size_t i = 1; i < ligne.points.size(); ++i)
					{
						// APPLICATION DE LA LOGIQUE OPENCV SELON LE PINCEAU SÉLECTIONNÉ
						if (ligne.typeBrush == 1)
						{
							// --- Pinceau de forme Carrée ---
							// On simule une ligne carrée en dessinant un rectangle OpenCV orienté ou une boîte
							cv::line(matrix,
								cv::Point(ligne.points[i - 1].x, ligne.points[i - 1].y),
								cv::Point(ligne.points[i].x, ligne.points[i].y),
								ligne.color, ligne.penSize * 2, cv::LINE_8
							);
						}
						else if (ligne.typeBrush == 2)
						{
							// --- Pinceau à effet Pointillés ---
							// On ne dessine qu'un segment sur deux pour créer une discontinuité
							if (i % 2 == 0) {
								cv::line(matrix,
									cv::Point(ligne.points[i - 1].x, ligne.points[i - 1].y),
									cv::Point(ligne.points[i].x, ligne.points[i].y),
									ligne.color, ligne.penSize * 2, cv::LINE_AA
								);
							}
						}
						else
						{
							// --- Pinceau Standard (Rond continu) ---
							cv::line(matrix,
								cv::Point(ligne.points[i - 1].x, ligne.points[i - 1].y),
								cv::Point(ligne.points[i].x, ligne.points[i].y),
								ligne.color, ligne.penSize * 2, cv::LINE_AA
							);
						}
					}
				}
			}
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
		premierTrace.color = penParameter->color;
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

			for (const auto& ligne : penParameter->listLines)
			{
				if (ligne.points.empty()) continue;

				if (ligne.points.size() == 1)
				{
					cv::circle(matrix, cv::Point(ligne.points[0].x, ligne.points[0].y), ligne.penSize, ligne.color, -1);
				}
				else if (ligne.points.size() > 1)
				{
					for (size_t i = 1; i < ligne.points.size(); ++i)
					{
						if (ligne.typeBrush == 1) // Carré
						{
							cv::line(matrix,
								cv::Point(ligne.points[i - 1].x, ligne.points[i - 1].y),
								cv::Point(ligne.points[i].x, ligne.points[i].y),
								ligne.color, ligne.penSize * 2, cv::LINE_8
							);
						}
						else if (ligne.typeBrush == 2) // Pointillés
						{
							if (i % 2 == 0) {
								cv::line(matrix,
									cv::Point(ligne.points[i - 1].x, ligne.points[i - 1].y),
									cv::Point(ligne.points[i].x, ligne.points[i].y),
									ligne.color, ligne.penSize * 2, cv::LINE_AA
								);
							}
						}
						else // Rond standard
						{
							cv::line(matrix,
								cv::Point(ligne.points[i - 1].x, ligne.points[i - 1].y),
								cv::Point(ligne.points[i].x, ligne.points[i].y),
								ligne.color, ligne.penSize * 2, cv::LINE_AA
							);
						}
					}
				}
			}
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
void CPenFilter::Drawing(wxMemoryDC* dc, IBitmapDisplay* bitmapViewer, CDraw* m_cDessin)
{
	if (m_cDessin != nullptr && dc != nullptr && bitmapViewer != nullptr)
	{
		int hpos = bitmapViewer->GetHPos();
		int vpos = bitmapViewer->GetVPos();
		float ratio = bitmapViewer->GetRatio();

		auto penParameter = static_cast<CPenFilterParameter*>(bitmapViewer->GetEffectPointer());

		wxColour wxColor(255, 0, 0);
		int activePenSize = 4;
		int activeBrushType = 0; // Par défaut : rond

		if (penParameter != nullptr)
		{
			wxColor = ConvertScalarToWxColour(penParameter->color);
			activePenSize = penParameter->penSize;
			activeBrushType = penParameter->typeBrush; // Récupération du choix de la ComboBox
		}

		auto penDraw = static_cast<Regards::FiltreEffet::CPenDraw*>(m_cDessin);
		if (penDraw != nullptr)
		{
			// Transmet de force le type de brush sélectionné à la classe de dessin avant le tracé
			// Ajoutez cette variable membre 'm_currentBrush' ou une méthode publique SetCurrentBrush(int) dans CPenDraw
			penDraw->SetCurrentBrushType(activeBrushType);
		}

		// Rendu de l'affichage temporaire à l'écran
		m_cDessin->Dessiner(dc, hpos, vpos, ratio, wxColor, wxColor, wxColor, activePenSize);

		// Synchronisation inverse lors de la sauvegarde (on propage aussi le type de brush)
		if (penParameter != nullptr && penDraw != nullptr)
		{
			penParameter->listLines.clear();
			const auto& tracesEcran = penDraw->GetTousLesTraces();

			for (const auto& traceEcran : tracesEcran)
			{
				SLineTrace ligneOpenCV;
				ligneOpenCV.points = traceEcran.points;
				ligneOpenCV.penSize = traceEcran.penSize;
				ligneOpenCV.typeBrush = traceEcran.typeBrush; // On conserve le type historique du trait
				ligneOpenCV.color = cv::Scalar(
					traceEcran.color.Blue(),
					traceEcran.color.Green(),
					traceEcran.color.Red(),
					traceEcran.color.Alpha()
				);
				penParameter->listLines.push_back(ligneOpenCV);
			}
		}
	}
}
