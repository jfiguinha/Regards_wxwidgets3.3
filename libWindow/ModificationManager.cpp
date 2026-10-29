#include <header.h>
#include "ModificationManager.h"

#include <EffectParameter.h>
#include <FileUtility.h>
#include <FilterData.h>
#include <FiltreEffet.h>
#include <ImageLoadingFormat.h>

#include <libPicture.h>
#include <wx/dir.h>
#include <wx/file.h>

#include <fstream>



// Inclusion des fichiers d'en-tête de vos paramètres spécifiques
#include <BilateralEffectParameter.h>
#include <BlurEffectParameter.h>
#include <BokehEffectParameter.h>
#include <BrightAndContrastEffectParameter.h>
#include <CartoonEffectParameter.h>
#include <CloudsEffectParameter.h>
#include <CropEffectParameter.h>
#include <FreeRotateEffectParameter.h>
#include <GaussianBlurEffectParameter.h>
#include <GradientFilterParameter.h>
#include <InpaintFilterParam.h>
#include <LensDistortionEffectParameter.h>
#include <LensFlareEffectParameter.h>
#include <MosaicFilterParameter.h>
#include <MotionBlurEffectParameter.h>
#include <NlmeansEffectParameter.h>
#include <OilPaintingEffectParameter.h>
#include <PaintBucketFilterParameter.h>
#include <PenEffectParameter.h>
#include <PhotoFiltreEffectParameter.h>
#include <PosterisationEffectParameter.h>
#include <RectangleFilterParameter.h>
#include <RgbEffectParameter.h>
#include <SelectFilterParameter.h>
#include <SharpenMaskingParameter.h>
#include <SolarisationEffectParameter.h>
#include <StylizationParameter.h>
#include <SwirlEffectParameter.h>
#include <TextEffectParameter.h>
#include <VignetteEffectParameter.h>
#include <WaveEffectParameter.h>
#include <effect_id.h>
#include <hqdn3dEffectParameter.h>

using namespace Regards::Picture;

// Clone un paramètre en connaissant son type réel T
template <class T>
static std::unique_ptr<CEffectParameter> CloneAs(
    CEffectParameter* effectParameter) {
    return std::make_unique<T>(*static_cast<T*>(effectParameter));
}

// Les trois sélections ne diffèrent que par selectType.
template <class TSelectType>
static std::unique_ptr<CEffectParameter> CloneSelect(
    CEffectParameter* effectParameter, TSelectType selectType) {
    auto selectFilter = std::make_unique<CSelectFilterParameter>(
        *static_cast<CSelectFilterParameter*>(effectParameter));
    selectFilter->selectType = selectType;
    return selectFilter;
}

// Fonction helper interne pour cloner le paramètre selon le type réel de
// l'effet
static std::unique_ptr<CEffectParameter> CloneEffectParameter(
    const int& numEffect, CEffectParameter* effectParameter) {
    if (effectParameter == nullptr) return nullptr;

    switch (numEffect) {
    case IDM_FILTRE_BILATERAL:
        return CloneAs<CBilateralEffectParameter>(effectParameter);
    case IDM_FILTRE_FLOU:
        return CloneAs<CBlurEffectParameter>(effectParameter);
    case IDM_FILTRE_BOKEH:
        return CloneAs<CBokehEffectParameter>(effectParameter);
    case IDM_IMAGE_LIGHTCONTRAST:
        return CloneAs<CBrightAndContrastEffectParameter>(effectParameter);
    case IDM_FILTER_CARTOON:
        return CloneAs<CCartoonEffectParameter>(effectParameter);
    case IDM_FILTRE_CLOUDS:
        return CloneAs<CCloudsEffectParameter>(effectParameter);
    case IDM_FILTRE_FLOUGAUSSIEN:
        return CloneAs<CGaussianBlurEffectParameter>(effectParameter);
    case IDM_FILTREHQDN3D:
        return CloneAs<Chqdn3dEffectParameter>(effectParameter);
    case IDM_FILTRELENSCORRECTION:
        return CloneAs<CLensDistortionEffectParameter>(effectParameter);
    case IDM_FILTRELENSFLARE:
        return CloneAs<CLensFlareEffectParameter>(effectParameter);
    case IDM_FILTRE_MOSAIQUE:
        return CloneAs<CMosaicEffectParameter>(effectParameter);
    case IDM_FILTRE_MOTIONBLUR:
        return CloneAs<CMotionBlurEffectParameter>(effectParameter);
    case IDM_FILTRE_NLMEAN:
        return CloneAs<CNlmeansEffectParameter>(effectParameter);
    case IDM_FILTER_OILPAINTING:
        return CloneAs<COilPaintingEffectParameter>(effectParameter);
    case ID_AJUSTEMENT_PHOTOFILTRE:
        return CloneAs<CPhotoFiltreEffectParameter>(effectParameter);
    case ID_AJUSTEMENT_POSTERISATION:
        return CloneAs<CPosterisationEffectParameter>(effectParameter);
    case IDM_ROTATE_FREE:
        return CloneAs<CFreeRotateEffectParameter>(effectParameter);
    case IDM_SHARPENMASKING:
        return CloneAs<CSharpenMaskingEffectParameter>(effectParameter);
    case IDM_AJUSTEMENT_SOLARISATION:
        return CloneAs<CSolarisationEffectParameter>(effectParameter);
    case IDM_FILTRE_STYLISATION:
        return CloneAs<CStylizationEffectParameter>(effectParameter);
    case IDM_FILTRE_SWIRL:
        return CloneAs<CSwirlEffectParameter>(effectParameter);
    case IDM_FILTRE_VIGNETTE:
        return CloneAs<CVignetteEffectParameter>(effectParameter);
    case IDM_WAVE_EFFECT:
        return CloneAs<CWaveEffectParameter>(effectParameter);
    case IDM_INPAINT:
        return CloneAs<CInpaintFilterParameter>(effectParameter);
    case IDM_PENFILTER:
        return CloneAs<CPenFilterParameter>(effectParameter);
    case IDM_GEOMETRICFILTER:
        return CloneAs<CRectangleFilterParameter>(effectParameter);
    case IDM_RECTANGLESELECT:
        return CloneSelect(effectParameter, SELECT_RECTANGLE);
    case IDM_ELLIPSESELECT:
        return CloneSelect(effectParameter, SELECT_ELLIPSE);
    case IDM_LASSOSELECT:
        return CloneSelect(effectParameter, SELECT_LASSO);
    case IDM_TEXTFILTER:
        return CloneAs<Regards::Filter::CTextFilterParameter>(effectParameter);
    case IDM_GRADIENTFILTER:
        return CloneAs<CGradientFilterParameter>(effectParameter);
    case IDM_PAINTBUCKETFILTER:
        return CloneAs<CPaintBucketFilterParameter>(effectParameter);
    case IDM_CROP:
        return CloneAs<CCropEffectParameter>(effectParameter);
    default:
        return std::make_unique<CEffectParameter>(*effectParameter);
    }
}

CModificationManager::CModificationManager(const wxString& folder) {
    // Récupère l'accès au répertoire d'historique général et crée une session
    // dédiée
    m_sessionFolder = CFileUtility::GetHistoryPath() + folder;
    if (!wxDir::Exists(m_sessionFolder)) {
        wxDir::Make(m_sessionFolder, wxS_DIR_DEFAULT, wxPATH_MKDIR_FULL);
    }
}

CModificationManager::~CModificationManager() = default;

void CModificationManager::Init(CImageLoadingFormat* bitmap,
    CLayerList* initialLayers) {
    EraseData();

    if (bitmap == nullptr || initialLayers == nullptr) return;
    filePath = bitmap->GetFilename();
    // L'étape initiale (root) n'applique aucun effet à sa création
    AddStep(-1, nullptr, 0, bitmap->GetFilename(), initialLayers, 0);
    numModification = 0;
}

void CModificationManager::EraseData() {
    for (const auto& step : historySteps) {
        DeleteStepFiles(step);
    }
    historySteps.clear();
    numModification = 0;
}

void CModificationManager::DeleteStepFiles(const ModificationStep& step) {
    if (!step.layersFilePath.IsEmpty() && wxFileExists(step.layersFilePath))
        wxRemoveFile(step.layersFilePath);
    if (!step.parameterFilePath.IsEmpty() && wxFileExists(step.parameterFilePath))
        wxRemoveFile(step.parameterFilePath);
}

unsigned int CModificationManager::GetNbModification() const {
    return static_cast<unsigned int>(historySteps.size());
}

unsigned int CModificationManager::GetNumModification() const {
    return numModification;
}

wxString CModificationManager::GetCurrentKey() {
    if (numModification < historySteps.size())
        return historySteps.at(numModification).libelle;
    return "";
}

bool CModificationManager::CanUndo() const { return numModification > 0; }

bool CModificationManager::CanRedo() const {
    return numModification + 1 < historySteps.size();
}

bool CModificationManager::Undo() {
    if (CanUndo()) {
        numModification--;
        return true;
    }
    return false;
}

bool CModificationManager::Redo() {
    if (CanRedo()) {
        numModification++;
        return true;
    }
    return false;
}

void CModificationManager::SetNumModification(
    const unsigned int& numModification) {
    this->numModification = numModification;
}

wxString CModificationManager::GetModificationLibelle(
    const unsigned int& numModification) const {
    if (numModification < historySteps.size())
        return historySteps.at(numModification).libelle;
    return "";
}

CLayerList* CModificationManager::GetLayersForModification(
    const unsigned int& targetModification) {
    if (targetModification >= historySteps.size()) return nullptr;

    const auto& step = historySteps.at(targetModification);

    // 1. Instanciation du conteneur et restauration polymorphique complète via
    // OpenCV xml/yml/json
    CLayerList* layerListCopy = new CLayerList();



    if (!layerListCopy->LoadFromFile(step.layersFilePath)) {
        delete layerListCopy;
        return nullptr;
    }

    if (targetModification == 0)
    {
        CLibPicture libPicture;
        LayerElement* layer = (*layerListCopy)[0];
        CImageLoadingFormat* picture = libPicture.LoadPicture(filePath);
        layer->SetPicture(picture);
        return layerListCopy;
    }

    // Étapes > 0 : Restauration dynamique du paramètre et application du filtre
    // d'étape
    if (step.filterId != -1 && !step.parameterFilePath.IsEmpty()) {
        CEffectParameter dummyBase;
        std::unique_ptr<CEffectParameter> parameter =
            CloneEffectParameter(step.filterId, &dummyBase);

        std::ifstream inParam(step.parameterFilePath.fn_str(), std::ios::binary);
        if (inParam.is_open() && parameter != nullptr) {
            inParam.read(reinterpret_cast<char*>(parameter.get()),
                sizeof(*parameter.get()));
            inParam.close();
        }

        if (parameter != nullptr) {
            for (size_t i = 0; i < layerListCopy->size(); ++i) {
                LayerElement* layer = (*layerListCopy)[i];
                if (layer != nullptr && layer->GetPicture() != nullptr &&
                    layer->GetLayerId() == step.targetLayerId) {
                    CFiltreEffet filtreEffet(color_quad, nullptr, layer->GetPicture());
                    CFiltreData::RenderEffect(step.filterId, &filtreEffet,
                        parameter.get(), false);
                    CImageLoadingFormat* picture = layer->GetPicture();
                    cv::Mat matrix = filtreEffet.GetBitmap(true);
                    picture->SetPicture(matrix);
                    layer->InvalidatePicture();
                }
            }
        }
    }
    layerListCopy->IsChanged(true);
    return layerListCopy;
}



void CModificationManager::AddStep(int filterId, CEffectParameter* parameter,
    size_t parameterSize, const wxString& libelle, CLayerList* currentLayers, int64_t targetLayerId) {

    if (currentLayers == nullptr) return;

    // Élimination de la branche Redo obsolète
    if (numModification + 1 < historySteps.size()) {
        for (auto it = historySteps.begin() + (numModification + 1); it != historySteps.end(); ++it) {
            DeleteStepFiles(*it);
        }
        historySteps.erase(historySteps.begin() + (numModification + 1), historySteps.end());
    }

    if (numModification > 0)
    {
        ModificationStep newStep;
        newStep.filterId = filterId;
        newStep.libelle = libelle;
        newStep.targetLayerId = targetLayerId;
        unsigned int stepIndex = historySteps.size();

        // Génère le fichier d'index spécifique à cette étape d'annulation/rétablissement (indique quel état utiliser en retour arrière)
        newStep.layersFilePath = m_sessionFolder + wxFILE_SEP_PATH + wxString::Format("step_%u_layers.bin", stepIndex);

        // Sauvegarde incrémentale gérée en interne par la liste de calques
        currentLayers->SaveToFile(newStep.layersFilePath);

        // Sauvegarde binaire du paramètre si applicable
        if (parameter != nullptr && parameterSize > 0) {
            newStep.parameterFilePath = m_sessionFolder + wxFILE_SEP_PATH + wxString::Format("step_%u_param.bin", stepIndex);
            std::ofstream outParam(newStep.parameterFilePath.fn_str(), std::ios::binary);
            if (outParam.is_open()) {
                outParam.write(reinterpret_cast<const char*>(parameter), parameterSize);
                outParam.close();
            }
        }

        historySteps.push_back(std::move(newStep));
    }
    else
    {

        ModificationStep newStep;
        newStep.filterId = filterId;
        newStep.libelle = libelle;
        newStep.targetLayerId = targetLayerId;
        unsigned int stepIndex = historySteps.size();
        // Génère le fichier d'index spécifique à cette étape d'annulation/rétablissement (indique quel état utiliser en retour arrière)
        newStep.layersFilePath = m_sessionFolder + wxFILE_SEP_PATH + wxString::Format("step_%u_layers.bin", stepIndex);
        // Sauvegarde incrémentale gérée en interne par la liste de calques
        currentLayers->SaveToFile(newStep.layersFilePath, false);
        historySteps.push_back(std::move(newStep));
    }
    numModification = static_cast<int>(historySteps.size()) - 1;
}


void CModificationManager::AddModification(const int& numEffect,
    CEffectParameter* effectParameter,
    const wxString& libelle,
    CLayerList* currentLayers,
    int64_t targetLayerId) {

    numModification++;
    auto cloned = CloneEffectParameter(numEffect, effectParameter);
    size_t paramSize = (cloned != nullptr) ? sizeof(*cloned.get()) : 0;
    AddStep(numEffect, cloned.get(), paramSize, libelle, currentLayers,
        targetLayerId);
}

void CModificationManager::AddModificationState(const wxString& libelle,
    CLayerList* currentLayers,
    int64_t targetLayerId) {
    AddStep(-1, nullptr, 0, libelle, currentLayers, targetLayerId);
}