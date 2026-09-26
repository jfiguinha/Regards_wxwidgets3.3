#include <header.h>
#include "ThumbnailEffect.h"

#include <FilterData.h>
#include <FiltreEffet.h>
#include <ImageLoadingFormat.h>
#include <LibResource.h>
#include <LoadingResource.h>
#include <ParamInit.h>
#include <RegardsConfigParam.h>
#include <ThumbnailDataStorage.h>
#include <effect_id.h>

#include <libPicture.h>

#include <algorithm>

#include "InfosSeparationBarEffect.h"
#include "ScrollbarWnd.h"
#include "wx/stdpaths.h"

using namespace Regards::Window;
using namespace Regards::FiltreEffet;
using namespace Regards::Control;
using namespace Regards::Picture;

wxDEFINE_EVENT(EVENT_ICONEUPDATE, wxCommandEvent);

class CThreadBitmapEffect {
public:
    CThreadBitmapEffect() : numIcone(0), photoId(0), numProcessId(0) {
        thumbnailData = nullptr;
        thumbnail = nullptr;
        // imageLoading est désormais copié ou partagé pour éviter les invalidations
        // de mémoire
        imageLoading = nullptr;
    }

    ~CThreadBitmapEffect() = default;

    wxString filename;
    wxString filepath;
    int numIcone;
    int photoId;
    int numProcessId;  // ID unique du traitement pour éviter les collisions
    // graphiques
    cv::Mat picture;
    CThumbnailDataStorage* thumbnailData;
    std::shared_ptr<CImageLoadingFormat> imageLoading;
    CThumbnailEffect* thumbnail;
};

CThumbnailEffect::CThumbnailEffect(wxWindow* parent, const wxWindowID id,
    const CThemeThumbnail& themeThumbnail,
    const bool& testValidity)
    : CThumbnailVerticalSeparator(parent, id, themeThumbnail, testValidity) {
    isAllProcess = true;
    processIdle = false;
    moveOnPaint = false;
    barseparationHeight = 40;
    currentProcessId = 0;

    config = CParamInit::getInstance();
    colorEffect = CLibResource::LoadStringFromResource("LBLCOLOREFFECT", 1);
    convolutionEffect =
        CLibResource::LoadStringFromResource("LBLCONVOLUTIONEFFECT", 1);
    specialEffect = CLibResource::LoadStringFromResource("LBLSPECIALEFFECT", 1);
    histogramEffect =
        CLibResource::LoadStringFromResource("LBLHISTOGRAMEFFECT", 1);
    blackRoomEffect = CLibResource::LoadStringFromResource("LBLBLACKROOM", 1);
    videoLabelEffect = CLibResource::LoadStringFromResource("LBLVIDEOEFFECT", 1);
    rotateEffect = CLibResource::LoadStringFromResource("LBLROTATEEFFECT", 1);

    Connect(EVENT_ICONEUPDATE,
        wxCommandEventHandler(CThumbnailEffect::UpdateRenderIcone));
}

CThumbnailEffect::~CThumbnailEffect(void) { listSeparator.clear(); }

wxString CThumbnailEffect::GetFilename() { return filename; }

bool CThumbnailEffect::ItemCompFonct(int x, int y, CIcone* icone,
    CWindowMain* parent) {
    if (icone == nullptr) return false;
    wxRect rc = icone->GetPos();
    return (rc.x < x && x < (rc.width + rc.x)) &&
        (rc.y < y && y < (rc.height + rc.y));
}

CIcone* CThumbnailEffect::FindElement(const int& xPos, const int& yPos) {
    int x = xPos + posLargeur;
    int y = yPos + posHauteur;
    pItemCompFonct _pf = &ItemCompFonct;
    return iconeList->FindElementByPosition(x, y, &_pf, this);
}

CInfosSeparationBarEffect* CThumbnailEffect::CreateNewSeparatorBar(
    const wxString& libelle) {
    auto infosSeparationBar = std::make_unique<CInfosSeparationBarEffect>(
        themeThumbnail.themeSeparation);
    infosSeparationBar->SetTitle(libelle);
    infosSeparationBar->SetWidth(GetWindowWidth());
    CInfosSeparationBarEffect* result = infosSeparationBar.get();
    listSeparator.push_back(std::move(infosSeparationBar));
    return result;
}

void CThumbnailEffect::SetFile(const wxString& filename,
    CImageLoadingFormat* imageLoading) {
    auto* iconeListLocal = new CIconeList();
    threadDataProcess = false;
    processIdle = false;

    // On passe sur un shared_ptr partagé avec les threads en arrière-plan
    this->sharedImageLoading = std::shared_ptr<CImageLoadingFormat>(imageLoading);

    // Incrémentation de l'identifiant de session pour rejeter les calculs
    // obsolètes
    currentProcessId++;

    CLoadingResource loadingResource;
    this->filename = filename;
    InitScrollingPos();
    listSeparator.clear();
    isAllProcess = false;

    CLibPicture picture;
    int format = picture.TestImageFormat(filename);
    if (picture.TestIsVideo(filename)) {
        CInfosSeparationBarEffect* videoEffect =
            CreateNewSeparatorBar(videoLabelEffect);
        int numElement = iconeListLocal->GetNbElement();
        videoEffect->AddPhotoToList(numElement);

        wxImage pBitmap = loadingResource.LoadImageResource("IDB_BLACKROOM");
        auto* thumbnailData = new CThumbnailDataStorage(
            CFiltreData::GetFilterLabel(IDM_FILTRE_VIDEO));
        thumbnailData->SetNumPhotoId(IDM_FILTRE_VIDEO);
        thumbnailData->SetBitmap(CLibPicture::mat_from_wx(pBitmap));

        auto* pBitmapIcone = new CIcone(thumbnailData);
        pBitmapIcone->SetTheme(themeThumbnail.themeIcone);
        iconeListLocal->AddElement(pBitmapIcone);

        isAllProcess = true;
    }
    else {
        CInfosSeparationBarEffect* infosSeparationColorEffect =
            CreateNewSeparatorBar(colorEffect);
        CInfosSeparationBarEffect* infosSeparationConvolutionEffect =
            CreateNewSeparatorBar(convolutionEffect);
        CInfosSeparationBarEffect* infosSeparationSpecialEffect =
            CreateNewSeparatorBar(specialEffect);
        CInfosSeparationBarEffect* infosSeparationHistogramEffect =
            CreateNewSeparatorBar(histogramEffect);
        CInfosSeparationBarEffect* infosSeparationRotateEffect =
            CreateNewSeparatorBar(rotateEffect);

        int i = 0;
        for (int numEffect = FILTER_START; numEffect < FILTER_END; numEffect++) {
            int numElement = iconeListLocal->GetNbElement();
            auto* thumbnailData = new CThumbnailDataStorage(filename);
            thumbnailData->SetNumElement(i++);
            thumbnailData->SetNumPhotoId(numEffect);

            switch (numEffect) {
            case IDM_INPAINT: {
                cv::Mat pBitmap = loadingResource.LoadResourceCV("IDB_INPAINT");
                thumbnailData->SetFilename(CFiltreData::GetFilterLabel(numEffect));
                infosSeparationSpecialEffect->AddPhotoToList(numElement);
                thumbnailData->SetBitmap(pBitmap);
                break;
            }
            case IDM_REDEYE: {
                cv::Mat pBitmap = loadingResource.LoadResourceCV("IDB_REDEYE");
                thumbnailData->SetFilename(CFiltreData::GetFilterLabel(numEffect));
                infosSeparationSpecialEffect->AddPhotoToList(numElement);
                thumbnailData->SetBitmap(pBitmap);
                break;
            }
            case IDM_FILTRE_COLORISATION: {
                cv::Mat pBitmap = loadingResource.LoadResourceCV("IDB_COLORISATION");
                thumbnailData->SetFilename(CFiltreData::GetFilterLabel(numEffect));
                infosSeparationSpecialEffect->AddPhotoToList(numElement);
                thumbnailData->SetBitmap(pBitmap);
                break;
            }
            case IDM_FILTRE_RESTORE: {
                cv::Mat pBitmap = loadingResource.LoadResourceCV("IDB_RESTORE");
                thumbnailData->SetFilename(CFiltreData::GetFilterLabel(numEffect));
                infosSeparationSpecialEffect->AddPhotoToList(numElement);
                thumbnailData->SetBitmap(pBitmap);
                break;
            }
            case IDM_CROP: {
                cv::Mat pBitmap = loadingResource.LoadResourceCV("IDB_CROP");
                thumbnailData->SetFilename(CFiltreData::GetFilterLabel(numEffect));
                infosSeparationSpecialEffect->AddPhotoToList(numElement);
                thumbnailData->SetBitmap(pBitmap);
                break;
            }
            case IDM_WAVE_EFFECT:
                thumbnailData->SetFilename(CFiltreData::GetFilterLabel(numEffect));
                infosSeparationSpecialEffect->AddPhotoToList(numElement);
                break;
            case IDM_FILTRELENSFLARE:
                thumbnailData->SetFilename(CFiltreData::GetFilterLabel(numEffect));
                infosSeparationSpecialEffect->AddPhotoToList(numElement);
                break;
            case IDM_FILTRELENSCORRECTION:
                thumbnailData->SetFilename(CFiltreData::GetFilterLabel(numEffect));
                infosSeparationSpecialEffect->AddPhotoToList(numElement);
                break;
            default:
                thumbnailData->SetFilename(CFiltreData::GetFilterLabel(numEffect));
                int typeEffect = CFiltreData::GetTypeEffect(numEffect);
                switch (typeEffect) {
                case SPECIAL_EFFECT:
                    infosSeparationSpecialEffect->AddPhotoToList(numElement);
                    break;
                case COLOR_EFFECT:
                    infosSeparationColorEffect->AddPhotoToList(numElement);
                    break;
                case CONVOLUTION_EFFECT:
                    infosSeparationConvolutionEffect->AddPhotoToList(numElement);
                    break;
                case HISTOGRAM_EFFECT:
                    infosSeparationHistogramEffect->AddPhotoToList(numElement);
                    break;
                case ROTATE_EFFECT:
                    infosSeparationRotateEffect->AddPhotoToList(numElement);
                    break;
                default:
                    break;
                }
                break;
            }
            thumbnailData->SetNumPhotoId(numEffect);
            auto* pBitmapIcone = new CIcone(thumbnailData);
            pBitmapIcone->SetTheme(themeThumbnail.themeIcone);
            iconeListLocal->AddElement(pBitmapIcone);
        }

        if (format == 4) {
            CInfosSeparationBarEffect* blackRoom =
                CreateNewSeparatorBar(blackRoomEffect);
            int numElement = iconeListLocal->GetNbElement();
            blackRoom->AddPhotoToList(numElement);

            cv::Mat image = loadingResource.LoadResourceCV("IDB_BLACKROOM");
            auto* thumbnailData = new CThumbnailDataStorage(
                CFiltreData::GetFilterLabel(IDM_DECODE_RAW));
            thumbnailData->SetNumPhotoId(IDM_DECODE_RAW);
            thumbnailData->SetBitmap(image);

            auto* pBitmapIcone = new CIcone(thumbnailData);
            pBitmapIcone->SetTheme(themeThumbnail.themeIcone);
            iconeListLocal->AddElement(pBitmapIcone);
        }
    }

    auto old = std::move(iconeList);
    iconeList.reset(iconeListLocal);
    nbElementInIconeList = iconeList->GetNbElement();
    old->EraseThumbnailListWithIcon();

    threadDataProcess = true;
    processIdle = true;

    UpdateScroll();
    ResizeThumbnail();
    needToRefresh = true;
}

void CThumbnailEffect::LoadPicture(void* param) {
    auto* threadLoadingBitmap = static_cast<CThreadBitmapEffect*>(param);
    if (threadLoadingBitmap == nullptr) return;

    CSqlThumbnail sqlThumbnail;
    std::unique_ptr<CImageLoadingFormat> _thumbnail;

    if (threadLoadingBitmap->imageLoading == nullptr)
        _thumbnail.reset(
            sqlThumbnail.GetPictureThumbnail(threadLoadingBitmap->filepath));

    CImageLoadingFormat* picture = (threadLoadingBitmap->imageLoading == nullptr)
        ? _thumbnail.get()
        : threadLoadingBitmap->imageLoading.get();

    if (picture != nullptr) {
        auto color_quad = CRgbaquad(
            threadLoadingBitmap->thumbnail->themeThumbnail.colorBack.Red(),
            threadLoadingBitmap->thumbnail->themeThumbnail.colorBack.Green(),
            threadLoadingBitmap->thumbnail->themeThumbnail.colorBack.Blue());

        auto filtre = std::make_unique<CFiltreEffet>(color_quad, nullptr, picture);

        switch (threadLoadingBitmap->photoId) {
        case IDM_WAVE_EFFECT:
            filtre->WaveFilter(20, 20, picture->GetHeight() / 2, 2, 20);
            break;

        case IDM_FILTRELENSFLARE:
            filtre->LensFlare(20, 20, 20, 1, 20, 45, 20);
            break;

        default: {
            std::unique_ptr<CEffectParameter> effect;
            effect.reset(CFiltreData::GetDefaultEffectParameter(
                threadLoadingBitmap->thumbnailData->GetNumPhotoId()));
            filtre->RenderEffect(
                threadLoadingBitmap->thumbnailData->GetNumPhotoId(), effect.get());
        } break;
        }
        threadLoadingBitmap->picture = filtre->GetBitmap(true);
    }
    auto* event = new wxCommandEvent(EVENT_ICONEUPDATE);
    event->SetClientData(threadLoadingBitmap);
    wxQueueEvent(threadLoadingBitmap->thumbnail, event);
}
void CThumbnailEffect::ProcessIdle() {
    int nbProcesseur = 1;
    CRegardsConfigParam* pConfig = CParamInit::getInstance();
    if (pConfig != nullptr) nbProcesseur = pConfig->GetThumbnailProcess();
    if (isAllProcess) {
        processIdle = false;
        return;
    }
    // Correction de la boucle : On vérifie la limite avant d'allouer
    for (auto i = 0; i < nbElementInIconeList && nbProcess < nbProcesseur; i++) {
        CIcone* icone = iconeList->GetElement(i);
        if (icone != nullptr) {
            CThumbnailData* pThumbnailData = icone->GetPtData();
            if (pThumbnailData != nullptr) {
                if (!pThumbnailData->IsLoad() && !pThumbnailData->IsProcess()) {
                    auto* pLoadBitmap = new CThreadBitmapEffect();
                    pLoadBitmap->thumbnail = this;
                    pLoadBitmap->thumbnailData =
                        static_cast<CThumbnailDataStorage*>(pThumbnailData);
                    pLoadBitmap->filepath = filename;
                    pLoadBitmap->filename = pThumbnailData->GetFilename();
                    pLoadBitmap->numIcone = i;
                    pLoadBitmap->photoId = pThumbnailData->GetNumPhotoId();
                    pLoadBitmap->imageLoading = sharedImageLoading;
                    // Copie safe du shared_ptr
                    pLoadBitmap->numProcessId = currentProcessId;
                    nbProcess++;
                    pThumbnailData->SetIsProcess(true);
                    // CORRECTION : Lancement autonome du thread via detach() pour
                    // éliminer le join() bloquant
                    std::thread t(LoadPicture, pLoadBitmap);
                    t.detach();
                }
            }
        }
    }
    // Évaluation de l'état global
    isAllProcess = true;
    for (int i = 0; i < nbElementInIconeList; i++) {
        CIcone* icone = iconeList->GetElement(i);
        if (icone != nullptr) {
            CThumbnailData* pThumbnailData = icone->GetPtData();
            if (pThumbnailData != nullptr && !pThumbnailData->IsLoad() &&
                !pThumbnailData->IsProcess()) {
                isAllProcess = false;
                break;
            }
        }
    }
}
void CThumbnailEffect::UpdateRenderIcone(wxCommandEvent& event) {
    nbProcess--;
    if (nbProcess < 0) nbProcess = 0;
    auto* threadLoadingBitmap =
        static_cast<CThreadBitmapEffect*>(event.GetClientData());
    if (threadLoadingBitmap == nullptr) return;
    // Protection : Si l'identifiant ne correspond plus à la session courante, on
    // rejette le résultat du vieux thread
    if (threadDataProcess &&
        threadLoadingBitmap->numProcessId == currentProcessId &&
        filename == threadLoadingBitmap->filepath) {
        if (!threadLoadingBitmap->picture.empty() &&
            threadLoadingBitmap->numIcone < nbElementInIconeList) {
            CIcone* icone = iconeList->GetElement(threadLoadingBitmap->numIcone);
            if (icone != nullptr) {
                CThumbnailData* pThumbnailData = icone->GetPtData();
                if (pThumbnailData != nullptr &&
                    pThumbnailData->GetFilename() == threadLoadingBitmap->filename) {
                    pThumbnailData->SetIsProcess(false);
                    pThumbnailData->SetBitmap(threadLoadingBitmap->picture);
                    pThumbnailData->SetIsLoading(false);
                    needToRefresh = true;
                }
            }
        }
    }
    delete threadLoadingBitmap;
    this->Refresh();
}

void CThumbnailEffect::UpdateScroll() {
    thumbnailSizeX = 0;
    thumbnailSizeY = 0;
    if (GetWindowWidth() <= 0) return;
    for (auto& infosSeparationBar : listSeparator) {
        int nbElement = static_cast<int>(infosSeparationBar->listElement.size());
        int iconeWidth = std::max<int>(1, themeThumbnail.themeIcone.GetWidth());
        int iconeHeight = std::max<int>(1, themeThumbnail.themeIcone.GetHeight());
        int nbElementByRow = GetWindowWidth() / iconeWidth;
        if ((nbElementByRow * iconeWidth) < GetWindowWidth()) nbElementByRow++;
        if (nbElementByRow <= 0) nbElementByRow = 1;
        int nbElementEnY = nbElement / nbElementByRow;
        if (nbElementEnY * nbElementByRow < nbElement) nbElementEnY++;
        int sizeX = std::min<int>(nbElement, nbElementByRow) * iconeWidth;
        if (sizeX > thumbnailSizeX) thumbnailSizeX = sizeX;
        thumbnailSizeY +=
            (nbElementEnY * iconeHeight) + infosSeparationBar->GetHeight();
    }
    wxWindow* parent = this->GetParent();
    if (parent != nullptr) {
        auto controlSize = std::make_unique<CControlSize>();
        controlSize->controlWidth = thumbnailSizeX;
        controlSize->controlHeight = thumbnailSizeY;
        wxCommandEvent evt(wxEVENT_SETCONTROLSIZE);
        evt.SetClientData(controlSize.release());
        parent->GetEventHandler()->AddPendingEvent(evt);
        auto size = std::make_unique<wxSize>(posLargeur, posHauteur);
        wxCommandEvent evtPos(wxEVENT_SETPOSITION);
        evtPos.SetClientData(size.release());
        parent->GetEventHandler()->AddPendingEvent(evtPos);
    }
}