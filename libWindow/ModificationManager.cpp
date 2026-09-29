#include <header.h>
#include "ModificationManager.h"
#include <FilterData.h> 
#include <ImageLoadingFormat.h>
#include <EffectParameter.h>
#include <FiltreEffet.h>
#include <libPicture.h>

// Inclusion des fichiers d'en-tête de vos paramètres spécifiques
// (Ajustez les noms des fichiers si nécessaire)
#include <BilateralEffectParameter.h>
#include <BlurEffectParameter.h>
#include <BokehEffectParameter.h>
#include <BrightAndContrastEffectParameter.h>
#include <CartoonEffectParameter.h>
#include <CloudsEffectParameter.h>
#include <RgbEffectParameter.h>
#include <GaussianBlurEffectParameter.h>
#include <hqdn3dEffectParameter.h>
#include <LensDistortionEffectParameter.h>
#include <LensFlareEffectParameter.h>
#include <MosaicFilterParameter.h>
#include <MotionBlurEffectParameter.h>
#include <NlmeansEffectParameter.h>
#include <OilPaintingEffectParameter.h>
#include <PhotoFiltreEffectParameter.h>
#include <PosterisationEffectParameter.h>
#include <FreeRotateEffectParameter.h>
#include <SharpenMaskingParameter.h>
#include <SolarisationEffectParameter.h>
#include <StylizationParameter.h>
#include <SwirlEffectParameter.h>
#include <VignetteEffectParameter.h>
#include <InpaintFilterParam.h>
#include <WaveEffectParameter.h>
#include <effect_id.h>
#include <CropEffectParameter.h>
#include <SelectFilterParameter.h>
#include <RectangleFilterParameter.h>
#include <PenEffectParameter.h>
using namespace Regards::Picture;


// Fonction helper interne pour cloner le paramètre selon le type réel de l'effet
// afin d'éviter le découpage (slicing) de l'objet lors de la copie.
static std::unique_ptr<CEffectParameter> CloneEffectParameter(const int& numEffect, CEffectParameter* effectParameter)
{
	if (effectParameter == nullptr)
		return nullptr;

	switch (numEffect)
	{
	case IDM_FILTRE_BILATERAL:
		return std::make_unique<CBilateralEffectParameter>(*static_cast<CBilateralEffectParameter*>(effectParameter));
	case IDM_FILTRE_FLOU:
		return std::make_unique<CBlurEffectParameter>(*static_cast<CBlurEffectParameter*>(effectParameter));
	case IDM_FILTRE_BOKEH:
		return std::make_unique<CBokehEffectParameter>(*static_cast<CBokehEffectParameter*>(effectParameter));
	case IDM_IMAGE_LIGHTCONTRAST:
		return std::make_unique<CBrightAndContrastEffectParameter>(*static_cast<CBrightAndContrastEffectParameter*>(effectParameter));
	case IDM_FILTER_CARTOON:
		return std::make_unique<CCartoonEffectParameter>(*static_cast<CCartoonEffectParameter*>(effectParameter));
	case IDM_FILTRE_CLOUDS:
		return std::make_unique<CCloudsEffectParameter>(*static_cast<CCloudsEffectParameter*>(effectParameter));
	case IDM_FILTRE_FLOUGAUSSIEN:
		return std::make_unique<CGaussianBlurEffectParameter>(*static_cast<CGaussianBlurEffectParameter*>(effectParameter));
	case IDM_FILTREHQDN3D:
		return std::make_unique<Chqdn3dEffectParameter>(*static_cast<Chqdn3dEffectParameter*>(effectParameter));
	case IDM_FILTRELENSCORRECTION:
		return std::make_unique<CLensDistortionEffectParameter>(*static_cast<CLensDistortionEffectParameter*>(effectParameter));
	case IDM_FILTRELENSFLARE:
		return std::make_unique<CLensFlareEffectParameter>(*static_cast<CLensFlareEffectParameter*>(effectParameter));
	case IDM_FILTRE_MOSAIQUE:
		return std::make_unique<CMosaicEffectParameter>(*static_cast<CMosaicEffectParameter*>(effectParameter));
	case IDM_FILTRE_MOTIONBLUR:
		return std::make_unique<CMotionBlurEffectParameter>(*static_cast<CMotionBlurEffectParameter*>(effectParameter));
	case IDM_FILTRE_NLMEAN:
		return std::make_unique<CNlmeansEffectParameter>(*static_cast<CNlmeansEffectParameter*>(effectParameter));
	case IDM_FILTER_OILPAINTING:
		return std::make_unique<COilPaintingEffectParameter>(*static_cast<COilPaintingEffectParameter*>(effectParameter));
	case ID_AJUSTEMENT_PHOTOFILTRE:
		return std::make_unique<CPhotoFiltreEffectParameter>(*static_cast<CPhotoFiltreEffectParameter*>(effectParameter));
	case ID_AJUSTEMENT_POSTERISATION:
		return std::make_unique<CPosterisationEffectParameter>(*static_cast<CPosterisationEffectParameter*>(effectParameter));
	case IDM_ROTATE_FREE:
		return std::make_unique<CFreeRotateEffectParameter>(*static_cast<CFreeRotateEffectParameter*>(effectParameter));
	case IDM_SHARPENMASKING:
		return std::make_unique<CSharpenMaskingEffectParameter>(*static_cast<CSharpenMaskingEffectParameter*>(effectParameter));
	case IDM_AJUSTEMENT_SOLARISATION:
		return std::make_unique<CSolarisationEffectParameter>(*static_cast<CSolarisationEffectParameter*>(effectParameter));
	case IDM_FILTRE_STYLISATION:
		return std::make_unique<CStylizationEffectParameter>(*static_cast<CStylizationEffectParameter*>(effectParameter));
	case IDM_FILTRE_SWIRL:
		return std::make_unique<CSwirlEffectParameter>(*static_cast<CSwirlEffectParameter*>(effectParameter));
	case IDM_FILTRE_VIGNETTE:
		return std::make_unique<CVignetteEffectParameter>(*static_cast<CVignetteEffectParameter*>(effectParameter));
	case IDM_WAVE_EFFECT:
		return std::make_unique<CWaveEffectParameter>(*static_cast<CWaveEffectParameter*>(effectParameter));
	case IDM_INPAINT:
		return std::make_unique<CInpaintFilterParameter>(*static_cast<CInpaintFilterParameter*>(effectParameter));
	case IDM_PENFILTER:
		return std::make_unique<CPenFilterParameter>(*static_cast<CPenFilterParameter*>(effectParameter));
	case IDM_RECTANGLEFILTER:
		return std::make_unique<CRectangleFilterParameter>(*static_cast<CRectangleFilterParameter*>(effectParameter));
	case IDM_SELECTFILTER:
		return std::make_unique<CSelectFilterParameter>(*static_cast<CSelectFilterParameter*>(effectParameter));
	case IDM_CROP:
		return std::make_unique<CCropEffectParameter>(*static_cast<CCropEffectParameter*>(effectParameter));
		// Ajoutez ici d'autres cas si le paramètre rgbEffectParameter correspond à d'autres filtres (ex: IDM_COLOR_BALANCE)
		// case IDM_COLOR_BALANCE:
		//     return std::make_unique<CRgbEffectParameter>(*static_cast<CRgbEffectParameter*>(effectParameter));

	default:
		// Secours par défaut pour les filtres n'ayant pas de variables spécifiques
		return std::make_unique<CEffectParameter>(*effectParameter);
	}
}

CModificationManager::CModificationManager(const wxString& folder)
{
	orientation = 0;
	nbModification = 0;
	numModification = 0;
	this->folder = folder;
	this->baseBitmap = nullptr;    //
	this->currentBitmap = nullptr; // 
}

void CModificationManager::Init(CImageLoadingFormat* bitmap)
{
	EraseData();
	filenameBitmap = bitmap->GetFilename();
	orientation = bitmap->GetOrientation();

	//On clone l'image de base en RAM pour ne plus JAMAIS faire de LoadPicture sur le disque
	CLibPicture picture;
	baseBitmap = picture.LoadPicture(filenameBitmap);

	// L'image courante au départ est l'image de base
	if (baseBitmap != nullptr) {
		currentBitmap = new CImageLoadingFormat(*baseBitmap);
	}

	ModificationStep rootStep;
	rootStep.filterId = -1;
	rootStep.parameter = nullptr;
	rootStep.libelle = bitmap->GetFilename();

	historySteps.push_back(std::move(rootStep));
}

CModificationManager::~CModificationManager()
{
	EraseData();
	if (baseBitmap != nullptr) // Clean de la base
	{
		delete baseBitmap;
		baseBitmap = nullptr;
	}
	if (currentBitmap != nullptr)
	{
		delete currentBitmap;
		currentBitmap = nullptr;
	}
}

void CModificationManager::EraseData()
{
	nbModification = 0;
	numModification = 0;
	historySteps.clear();
	// Note: Ne pas supprimer baseBitmap ici si EraseData est appelé dans Init avant l'affectation
}

unsigned int CModificationManager::GetNbModification()
{
	return nbModification;
}

unsigned int CModificationManager::GetNumModification()
{
	return numModification;
}

void CModificationManager::SetNumModification(const unsigned int& numModification)
{
	this->numModification = numModification;
}

wxString CModificationManager::GetModificationLibelle(const unsigned int& numModification)
{
	if (numModification < historySteps.size())
		return historySteps.at(numModification).libelle;
	return "";
}

CImageLoadingFormat* CModificationManager::GetModification(const unsigned int& targetModification)
{
	if (targetModification >= historySteps.size() || baseBitmap == nullptr)
		return nullptr;

	// Cas 1 : On demande l'image d'origine
	if (targetModification == 0)
	{
		this->numModification = 0;
		return new CImageLoadingFormat(*baseBitmap);
	}

	// Cas 2 : L'image demandée est EXACTEMENT l'image courante qu'on a déjà en RAM
	if (targetModification == this->numModification && currentBitmap != nullptr)
	{
		return new CImageLoadingFormat(*currentBitmap);
	}

	CImageLoadingFormat* processBitmap = nullptr;
	unsigned int startStep = 1;

	// Cas 3 : Optimisation incrémentale (L'utilisateur avance d'étapes)
	// Si on demande une étape future (ex: étape 4) et qu'on est à l'étape 3, 
	// on repart de l'image de l'étape 3 au lieu de repartir de zéro !
	if (targetModification > this->numModification && currentBitmap != nullptr)
	{
		processBitmap = new CImageLoadingFormat(*currentBitmap);
		startStep = this->numModification + 1; // On ne rejoue que les filtres manquants
	}
	else
	{
		// Cas 4 : L'utilisateur revient en arrière, on est obligé de repartir de la base en RAM
		processBitmap = new CImageLoadingFormat(*baseBitmap);
		startStep = 1;
	}

	CFiltreEffet filtreEffet(color_quad, nullptr, processBitmap);

	// Rejeu séquentiel uniquement sur le différentiel nécessaire
	for (unsigned int i = startStep; i <= targetModification; ++i)
	{
		const auto& step = historySteps.at(i);
		if (step.filterId != -1)
		{
			CFiltreData::RenderEffect(step.filterId, &filtreEffet, step.parameter.get(), false);
		}
	}

	cv::Mat output = filtreEffet.GetBitmap(false);

	// Mettre à jour le cache de l'étape courante pour les prochains appels
	if (currentBitmap != nullptr) {
		delete currentBitmap;
	}
	currentBitmap = new CImageLoadingFormat();
	currentBitmap->SetPicture(output);
	currentBitmap->SetOrientation(0);
	currentBitmap->SetFilename(filenameBitmap);

	this->numModification = targetModification;

	// Nettoyage
	delete processBitmap;

	// Renvoie une copie pour l'affichage/utilisation externe
	return new CImageLoadingFormat(*currentBitmap);
}

void CModificationManager::AddModification(const int& numEffect, CEffectParameter* effectParameter, const wxString& libelle)
{
	if (numModification < nbModification)
	{
		historySteps.erase(historySteps.begin() + (numModification + 1), historySteps.end());
	}

	nbModification = numModification;
	numModification++;
	nbModification++;

	ModificationStep newStep;
	newStep.filterId = numEffect;
	newStep.libelle = libelle;

	// Utilisation du Helper de clonage polymorphique pour conserver l'entièreté des données
	newStep.parameter = CloneEffectParameter(numEffect, effectParameter);

	historySteps.push_back(std::move(newStep));
}
