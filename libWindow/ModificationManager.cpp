#include <header.h>
#include "ModificationManager.h"
#include <FilterData.h> 
#include <ImageLoadingFormat.h>
#include <EffectParameter.h>
#include <FiltreEffet.h>
#include <libPicture.h>

// Inclusion des fichiers d'en-tête de vos paramètres spécifiques
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
#include <TextEffectParameter.h>
#include <PaintBucketFilterParameter.h>
#include <GradientFilterParameter.h>

using namespace Regards::Picture;

// Fonction helper interne pour cloner le paramètre selon le type réel de l'effet
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
    case IDM_GEOMETRICFILTER:
        return std::make_unique<CRectangleFilterParameter>(*static_cast<CRectangleFilterParameter*>(effectParameter));
    case IDM_RECTANGLESELECT:
    {
        std::unique_ptr<CSelectFilterParameter> selectFilter = std::make_unique<CSelectFilterParameter>(*static_cast<CSelectFilterParameter*>(effectParameter));
        selectFilter->selectType = SELECT_RECTANGLE;
        return selectFilter;
    }
    case IDM_ELLIPSESELECT:
    {
        std::unique_ptr<CSelectFilterParameter> selectFilter = std::make_unique<CSelectFilterParameter>(*static_cast<CSelectFilterParameter*>(effectParameter));
        selectFilter->selectType = SELECT_ELLIPSE;
        return selectFilter;
    }
    case IDM_LASSOSELECT:
    {
        std::unique_ptr<CSelectFilterParameter> selectFilter = std::make_unique<CSelectFilterParameter>(*static_cast<CSelectFilterParameter*>(effectParameter));
        selectFilter->selectType = SELECT_LASSO;
        return selectFilter;
    }
    case IDM_TEXTFILTER:
        return std::make_unique<Regards::Filter::CTextFilterParameter>(*static_cast<Regards::Filter::CTextFilterParameter*>(effectParameter));
    case IDM_GRADIENTFILTER:
        return std::make_unique<CGradientFilterParameter>(*static_cast<CGradientFilterParameter*>(effectParameter));
    case IDM_PAINTBUCKETFILTER:
        return std::make_unique<CPaintBucketFilterParameter>(*static_cast<CPaintBucketFilterParameter*>(effectParameter));
    case IDM_CROP:
        return std::make_unique<CCropEffectParameter>(*static_cast<CCropEffectParameter*>(effectParameter));
    default:
        return std::make_unique<CEffectParameter>(*effectParameter);
    }
}

CModificationManager::CModificationManager(const wxString& folder)
{
    orientation = 0;
    nbModification = 0;
    numModification = 0;
    this->folder = folder;
    this->baseBitmap = nullptr;
    this->currentBitmap = nullptr;
}

CModificationManager::~CModificationManager()
{
    EraseData();
    if (baseBitmap != nullptr)
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


void CModificationManager::Init(CImageLoadingFormat* bitmap, CLayerList* initialLayers)
{
    EraseData();

    if (bitmap == nullptr || initialLayers == nullptr)
        return;

    filenameBitmap = bitmap->GetFilename();
    orientation = bitmap->GetOrientation();

    ModificationStep rootStep;
    rootStep.filterId = -1;
    rootStep.targetLayerId = 0;
    rootStep.parameter = nullptr;
    rootStep.libelle = filenameBitmap;
    // On stocke une copie profonde de la liste de calques initiale
    rootStep.layers = std::make_unique<CLayerList>(*initialLayers);
    rootStep.hasLayerState = true;

    historySteps.push_back(std::move(rootStep));
    numModification = 0;
    nbModification = 1;
}


void CModificationManager::EraseData()
{
    nbModification = 0;
    numModification = 0;
    historySteps.clear();
}

unsigned int CModificationManager::GetNbModification() const
{
    return static_cast<unsigned int>(historySteps.size());
}

unsigned int CModificationManager::GetNumModification() const
{
    return numModification;
}


wxString CModificationManager::GetCurrentKey()
{
    if (numModification < historySteps.size())
        return historySteps.at(numModification).libelle;
    return "";
}

bool CModificationManager::CanUndo() const
{
    // On peut annuler si on n'est pas déjà sur l'état initial (0)
    return numModification > 0;
}

bool CModificationManager::CanRedo() const
{
    // On peut rétablir si on n'est pas déjà sur la dernière modification disponible
    return !historySteps.empty() && numModification < (historySteps.size() - 1);
}

bool CModificationManager::Undo()
{
    if (CanUndo())
    {
        numModification--;
        return true;
    }
    return false;
}

bool CModificationManager::Redo()
{
    if (CanRedo())
    {
        numModification++;
        return true;
    }
    return false;
}


void CModificationManager::SetNumModification(const unsigned int& numModification)
{
    this->numModification = numModification;
}

wxString CModificationManager::GetModificationLibelle(const unsigned int& numModification) const
{
    if (numModification < historySteps.size())
        return historySteps.at(numModification).libelle;
    return "";
}


CLayerList* CModificationManager::GetLayersForModification(const unsigned int& targetModification)
{
    // Index invalide : nullptr (et non une liste vide, qui remplacerait puis effacerait l'affichage)
    if (targetModification >= historySteps.size())
        return nullptr;

    const auto& step = historySteps.at(targetModification);
    if (!step.layers)
        return nullptr;

    this->numModification = targetModification;

    // L'instantané stocké à cette étape contient déjà l'état exact des calques (pixels compris).
    // On ne remet PAS les calques à leur image d'origine : cela effaçait les modifications
    // propres à chaque calque (dessin, collage, effets des étapes précédentes).
    // L'appelant reçoit la propriété de cette copie profonde.
    CLayerList* layerListCopy = new CLayerList(*(step.layers));

    // Étape 0 (état source) : on garantit l'image originale brute de chaque calque
    // (originalPicture, figée au premier SetPicture), quel que soit l'état de l'instantané racine.
    if (targetModification == 0)
    {
        for (size_t i = 0; i < layerListCopy->size(); ++i)
        {
            LayerElement* layer = (*layerListCopy)[i];
            if (layer != nullptr)
                layer->ResetToOriginal();
        }
    }

    // Étapes > 0 : l'instantané est pris avant l'effet, on applique donc le filtre de l'étape
    // par-dessus l'état sauvegardé (sans remise à l'original : le contenu des calques est conservé,
    // y compris les effets des étapes précédentes déjà présents dans l'instantané).
    if (step.filterId != -1 && step.parameter != nullptr)
    {
        for (size_t i = 0; i < layerListCopy->size(); ++i)
        {
            LayerElement* layer = (*layerListCopy)[i];
            if (layer != nullptr && layer->GetPicture() != nullptr)
            {
                CFiltreEffet filtreEffet(color_quad, nullptr, layer->GetPicture());
                CFiltreData::RenderEffect(step.filterId, &filtreEffet, step.parameter.get(), false);

                CImageLoadingFormat* picture = layer->GetPicture();
                picture->SetPicture(filtreEffet.GetBitmap(true));
                //cv::Mat matrix = filtreEffet.GetBitmap(true);
                //layer->SetPicture(new CImageLoadingFormat(matrix));
                layer->InvalidatePicture();
            }
        }
    }

    layerListCopy->IsChanged(true);
    return layerListCopy;
}


void CModificationManager::AddModification(const int& numEffect, CEffectParameter* effectParameter, const wxString& libelle, CLayerList* currentLayers)
{
    if (currentLayers == nullptr) return;

    if (numModification + 1 < historySteps.size())
    {
        historySteps.erase(historySteps.begin() + (numModification + 1), historySteps.end());
    }

    ModificationStep newStep;
    newStep.filterId = numEffect;
    newStep.libelle = libelle;
    newStep.parameter = CloneEffectParameter(numEffect, effectParameter);

    // On génère une copie profonde pour isoler cette étape dans l'historique
    newStep.layers = std::make_unique<CLayerList>(*currentLayers);
    newStep.hasLayerState = true;

    historySteps.push_back(std::move(newStep));
    numModification = static_cast<unsigned int>(historySteps.size()) - 1;
    nbModification = numModification + 1;
}

void CModificationManager::AddModificationState(const wxString& libelle, CLayerList* currentLayers)
{
    if (currentLayers == nullptr) return;

    if (numModification + 1 < historySteps.size())
    {
        historySteps.erase(historySteps.begin() + (numModification + 1), historySteps.end());
    }

    ModificationStep newStep;
    newStep.filterId = -1;
    newStep.parameter = nullptr;
    newStep.libelle = libelle;
    newStep.layers = std::make_unique<CLayerList>(*currentLayers); // copie profonde, comme AddModification
    newStep.hasLayerState = true;

    historySteps.push_back(std::move(newStep));
    numModification = static_cast<unsigned int>(historySteps.size()) - 1;
    nbModification = numModification + 1;
}