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

void CPenFilter::Drawing(wxMemoryDC* dc, IBitmapDisplay* bitmapViewer, FiltreEffet::CDraw* m_cDessin)
{
	if (m_cDessin != nullptr && dc != nullptr && bitmapViewer != nullptr)
	{
		int hpos = bitmapViewer->GetHPos();
		int vpos = bitmapViewer->GetVPos();
		float ratio = bitmapViewer->GetRatio();

		// 1. Récupération des paramètres du filtre
		auto penParameter = static_cast<CPenFilterParameter*>(bitmapViewer->GetEffectPointer());

		wxColour wxColor(255, 0, 0); // Couleur par défaut
		int activePenSize = 4;       // Taille par défaut

		if (penParameter != nullptr)
		{
			// Extraction BGR (OpenCV) vers RGB (wxWidgets) pour l'affichage courant
			int r = static_cast<int>(penParameter->color[2]); // R est à l'indice 2 en BGR
			int g = static_cast<int>(penParameter->color[1]); // G est à l'indice 1
			int b = static_cast<int>(penParameter->color[0]); // B est à l'indice 0
			int a = static_cast<int>(penParameter->color[3]);

			wxColor.Set(r, g, b, a > 0 ? a : 255);
			activePenSize = penParameter->penSize;
		}

		// 2. On demande à CPenDraw d'exécuter son rendu à l'écran
		m_cDessin->Dessiner(dc, hpos, vpos, ratio, wxColor, wxColor, wxColor, activePenSize);

		// 3. SAUVEGARDE : Synchronisation des données de l'écran vers le penParameter
		auto penDraw = static_cast<Regards::FiltreEffet::CPenDraw*>(m_cDessin);
		if (penParameter != nullptr && penDraw != nullptr)
		{
			// On vide l'ancien historique du paramètre pour y copier la version fraîche à jour
			penParameter->listLines.clear();

			// Récupération des tracés depuis la classe de dessin
			const auto& tracesEcran = penDraw->GetTousLesTraces();

			for (const auto& traceEcran : tracesEcran)
			{
				SLineTrace ligneOpenCV;
				ligneOpenCV.points = traceEcran.points; // Copie du vecteur de wxPoint
				ligneOpenCV.penSize = traceEcran.penSize;

				// Conversion inverse : wxColour (RGB) vers cv::Scalar (BGR)
				ligneOpenCV.color = cv::Scalar(
					traceEcran.color.Blue(),
					traceEcran.color.Green(),
					traceEcran.color.Red(),
					traceEcran.color.Alpha()
				);

				// Sauvegarde définitive dans les paramètres du filtre
				penParameter->listLines.push_back(ligneOpenCV);
			}
		}
	}
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

	filtreInterface->AddTreeInfos(libellePenSize,
		new CTreeElementValueInt(inpaintParam->penSize), &elementSize);
	filtreInterface->AddTreeInfos(libelleColor,
		new CTreeElementValueColor(ConvertScalarToWxColour(inpaintParam->color)), &elementSize, TYPE_COLOR, TYPE_COLOR);
}

void CPenFilter::FilterChangeParam(CEffectParameter* effectParameter, CTreeElementValue* valueData,
	const wxString& key)
{
	auto penParameter = static_cast<CPenFilterParameter*>(effectParameter);

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
	case TYPE_ELEMENT_COLOR:
	{
		auto colorValue = static_cast<CTreeElementValueColor*>(valueData);
		wxColour c = colorValue->GetValue();
	}
	break;
	default:;
	}

	if (key == libellePenSize)
	{
		penParameter->penSize = static_cast<int>(value);
	}
	else if (key == libelleColor)
	{
		auto colorValue = static_cast<CTreeElementValueColor*>(valueData);
		wxColour c = colorValue->GetValue();
		penParameter->color = cv::Scalar(c.Blue(), c.Green(), c.Red(), c.Alpha());
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
	const wxString libelle = CLibResource::LoadStringFromResource(L"LBLBUSYINFO", 1);
	wxBusyInfo wait(libelle, nullptr);

	CImageLoadingFormat* imageLoad = nullptr;
	auto penParameter = static_cast<CPenFilterParameter*>(effectParameter);

	if (penParameter->apply)
	{
		imageLoad = new CImageLoadingFormat();
		imageLoad->SetPicture(filtreEffet->GetBitmap(true));
		imageLoad->RotateExif(orientation);

		// Remplacer les boucles de traitement dans RenderEffect et ApplyEffect :
		try
		{
			cv::Mat& matrix = imageLoad->GetMatImage();

			// On boucle sur nos objets structures contenant l'historique complet
			for (const auto& ligne : penParameter->listLines)
			{
				if (ligne.points.empty()) continue;

				if (ligne.points.size() == 1)
				{
					// Utilisation de la taille et de la couleur propres à ce tracé précis
					cv::circle(matrix, cv::Point(ligne.points[0].x, ligne.points[0].y), ligne.penSize, ligne.color, -1);
				}
				else if (ligne.points.size() > 1)
				{
					for (size_t i = 1; i < ligne.points.size(); ++i)
					{
						cv::line(matrix,
							cv::Point(ligne.points[i - 1].x, ligne.points[i - 1].y),
							cv::Point(ligne.points[i].x, ligne.points[i].y),
							ligne.color,            // Sa propre couleur
							ligne.penSize * 2,      // Sa propre épaisseur
							cv::LINE_AA
						);
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

	// Si l'utilisateur clique pour la première fois, on initialise le premier conteneur
	if (penParameter->listLines.empty()) {
		SLineTrace premierTrace;
		premierTrace.color = penParameter->color;
		premierTrace.penSize = penParameter->penSize;
		penParameter->listLines.push_back(premierTrace);
	}

	// Si le dessin à l'écran (CPenDraw) a créé une nouvelle ligne vide suite à un MouseDown,
	// on synchronise en ajoutant un calque de tracé correspondant avec les styles actuels du filtre
	if (bitmapViewer->GetDessinPt() != nullptr && penParameter->listLines.size() < 2) {
		// (Optionnel selon l'état de votre gestionnaire d'événements externe)
	}


	if (!source.empty())
	{
		imageLoad = new CImageLoadingFormat();
		imageLoad->SetPicture(source);
		imageLoad->RotateExif(orientation);

		// Remplacer les boucles de traitement dans RenderEffect et ApplyEffect :
		try
		{
			cv::Mat& matrix = imageLoad->GetMatImage();

			// On boucle sur nos objets structures contenant l'historique complet
			for (const auto& ligne : penParameter->listLines)
			{
				if (ligne.points.empty()) continue;

				if (ligne.points.size() == 1)
				{
					// Utilisation de la taille et de la couleur propres à ce tracé précis
					cv::circle(matrix, cv::Point(ligne.points[0].x, ligne.points[0].y), ligne.penSize, ligne.color, -1);
				}
				else if (ligne.points.size() > 1)
				{
					for (size_t i = 1; i < ligne.points.size(); ++i)
					{
						cv::line(matrix,
							cv::Point(ligne.points[i - 1].x, ligne.points[i - 1].y),
							cv::Point(ligne.points[i].x, ligne.points[i].y),
							ligne.color,            // Sa propre couleur
							ligne.penSize * 2,      // Sa propre épaisseur
							cv::LINE_AA
						);
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
