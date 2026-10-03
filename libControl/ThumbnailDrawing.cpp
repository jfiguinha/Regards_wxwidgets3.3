#include <header.h>
#include "ThumbnailDrawing.h"

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



CThumbnailDrawing::CThumbnailDrawing(wxWindow* parent, const wxWindowID id,
    const CThemeThumbnail& themeThumbnail,
    const bool& testValidity, const int & panelInfosId)
    : CThumbnailVerticalSeparator(parent, id, themeThumbnail, testValidity) {

    processIdle = false;
    moveOnPaint = false;
    barseparationHeight = 40;
	this->panelInfosId = panelInfosId;

    config = CParamInit::getInstance();
    drawingEffect = CLibResource::LoadStringFromResource("LBLDRAWINGEFFECT", 1);

}

CThumbnailDrawing::~CThumbnailDrawing(void) { listSeparator.clear(); }


bool CThumbnailDrawing::ItemCompFonct(int x, int y, CIcone* icone,
    CWindowMain* parent) {
    if (icone == nullptr) return false;
    wxRect rc = icone->GetPos();
    return (rc.x < x && x < (rc.width + rc.x)) &&
        (rc.y < y && y < (rc.height + rc.y));
}

CIcone* CThumbnailDrawing::FindElement(const int& xPos, const int& yPos) {
    int x = xPos + posLargeur;
    int y = yPos + posHauteur;
    pItemCompFonct _pf = &ItemCompFonct;
    return iconeList->FindElementByPosition(x, y, &_pf, this);
}

CInfosSeparationBarEffect* CThumbnailDrawing::CreateNewSeparatorBar(
    const wxString& libelle) {
    auto infosSeparationBar = std::make_unique<CInfosSeparationBarEffect>(
        themeThumbnail.themeSeparation);
    infosSeparationBar->SetTitle(libelle);
    infosSeparationBar->SetWidth(GetWindowWidth());
    CInfosSeparationBarEffect* result = infosSeparationBar.get();
    listSeparator.push_back(std::move(infosSeparationBar));
    return result;
}

void CThumbnailDrawing::OnPictureClick(const int& numPhotoId)
{
    wxWindow* panelInfos = this->FindWindowById(panelInfosId);
    if (panelInfos != nullptr)
    {

        wxCommandEvent evt(wxEVENT_APPLYEFFECT);
        evt.SetInt(numPhotoId);
        panelInfos->GetEventHandler()->AddPendingEvent(evt);
    }
}

void CThumbnailDrawing::Init() {
    auto* iconeListLocal = new CIconeList();

    processIdle = false;


    CLoadingResource loadingResource;
    InitScrollingPos();
    listSeparator.clear();


    CInfosSeparationBarEffect* infosSeparationDrawingTools =
        CreateNewSeparatorBar(drawingEffect);

    int i = 0;
    for (int numEffect = FILTER_START; numEffect < FILTER_END; numEffect++) {
        int numElement = iconeListLocal->GetNbElement();
        auto* thumbnailData = new CThumbnailDataStorage("drawing");
        thumbnailData->SetNumElement(i++);
        thumbnailData->SetNumPhotoId(numEffect);

        switch (numEffect) {
/*
        case IDM_CROP: {
            cv::Mat pBitmap = loadingResource.LoadResourceCV("IDB_CROP");
            thumbnailData->SetFilename(CFiltreData::GetFilterLabel(numEffect));
            infosSeparationDrawingTools->AddPhotoToList(numElement);
            thumbnailData->SetBitmap(pBitmap);
            break;
        }*/
        case IDM_PENFILTER: {
            cv::Mat pBitmap = loadingResource.LoadResourceCV("IDB_PEN");
            thumbnailData->SetFilename(CFiltreData::GetFilterLabel(numEffect));
            infosSeparationDrawingTools->AddPhotoToList(numElement);
            thumbnailData->SetBitmap(pBitmap);
            break;
        }
        case IDM_GRADIENTFILTER: {
            cv::Mat pBitmap = loadingResource.LoadResourceCV("IDB_GRADIENT");
            thumbnailData->SetFilename(CFiltreData::GetFilterLabel(numEffect));
            infosSeparationDrawingTools->AddPhotoToList(numElement);
            thumbnailData->SetBitmap(pBitmap);
            break;
        }
        case IDM_PAINTBUCKETFILTER: {
            cv::Mat pBitmap = loadingResource.LoadResourceCV("IDB_PAINTBUCKET");
            thumbnailData->SetFilename(CFiltreData::GetFilterLabel(numEffect));
            infosSeparationDrawingTools->AddPhotoToList(numElement);
            thumbnailData->SetBitmap(pBitmap);
            break;
        }
        case IDM_TEXTFILTER: {
            cv::Mat pBitmap = loadingResource.LoadResourceCV("IDB_TEXT");
            thumbnailData->SetFilename(CFiltreData::GetFilterLabel(numEffect));
            infosSeparationDrawingTools->AddPhotoToList(numElement);
            thumbnailData->SetBitmap(pBitmap);
            break;
        }
        case IDM_MAGICHANDFILTER: {
            cv::Mat pBitmap = loadingResource.LoadResourceCV("IDB_MAGICHAND");
            thumbnailData->SetFilename(CFiltreData::GetFilterLabel(numEffect));
            infosSeparationDrawingTools->AddPhotoToList(numElement);
            thumbnailData->SetBitmap(pBitmap);
            break;
        }
        case IDM_RECTANGLEFILTER: {
            cv::Mat pBitmap = loadingResource.LoadResourceCV("IDB_RECTANGLE");
            thumbnailData->SetFilename(CFiltreData::GetFilterLabel(numEffect));
            infosSeparationDrawingTools->AddPhotoToList(numElement);
            thumbnailData->SetBitmap(pBitmap);
            break;
        }
        case IDM_SELECTFILTER: {
            cv::Mat pBitmap = loadingResource.LoadResourceCV("IDB_SELECTFILTER");
            thumbnailData->SetFilename(CFiltreData::GetFilterLabel(numEffect));
            infosSeparationDrawingTools->AddPhotoToList(numElement);
            thumbnailData->SetBitmap(pBitmap);
            break;
        }

        default:
            break;
        }
        thumbnailData->SetNumPhotoId(numEffect);
        auto* pBitmapIcone = new CIcone(thumbnailData);
        pBitmapIcone->SetTheme(themeThumbnail.themeIcone);
        iconeListLocal->AddElement(pBitmapIcone);
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


void CThumbnailDrawing::UpdateScroll() {
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