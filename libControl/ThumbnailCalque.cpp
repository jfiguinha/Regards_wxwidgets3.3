#include <header.h>
#include "ThumbnailCalque.h"

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


CThumbnailCalque::CThumbnailCalque(wxWindow* parent, const wxWindowID id,
    const CThemeThumbnail& themeThumbnail)
    : CThumbnailVerticalSeparator(parent, id, themeThumbnail, false) {

    processIdle = false;
    moveOnPaint = false;
    barseparationHeight = 40;

    config = CParamInit::getInstance();
    calqueLibelle = "Calque";

}

void CThumbnailCalque::OnPictureClick(const int& numPhotoId)
{

}

CThumbnailCalque::~CThumbnailCalque(void) { listSeparator.clear(); }

wxString CThumbnailCalque::GetFilename() { return filename; }

bool CThumbnailCalque::ItemCompFonct(int x, int y, CIcone* icone,
    CWindowMain* parent) {
    if (icone == nullptr) return false;
    wxRect rc = icone->GetPos();
    return (rc.x < x && x < (rc.width + rc.x)) &&
        (rc.y < y && y < (rc.height + rc.y));
}

CIcone* CThumbnailCalque::FindElement(const int& xPos, const int& yPos) {
    int x = xPos + posLargeur;
    int y = yPos + posHauteur;
    pItemCompFonct _pf = &ItemCompFonct;
    return iconeList->FindElementByPosition(x, y, &_pf, this);
}

CInfosSeparationBarEffect* CThumbnailCalque::CreateNewSeparatorBar(
    const wxString& libelle) {
    auto infosSeparationBar = std::make_unique<CInfosSeparationBarEffect>(
        themeThumbnail.themeSeparation);
    infosSeparationBar->SetTitle(libelle);
    infosSeparationBar->SetWidth(GetWindowWidth());
    CInfosSeparationBarEffect* result = infosSeparationBar.get();
    listSeparator.push_back(std::move(infosSeparationBar));
    return result;
}

void CThumbnailCalque::SetFile(const wxString& filename,
    CImageLoadingFormat* imageLoading) {
    auto* iconeListLocal = new CIconeList();
    threadDataProcess = false;
    processIdle = false;


    CLoadingResource loadingResource;
    this->filename = filename;
    InitScrollingPos();
    listSeparator.clear();


    CLibPicture picture;
    int format = picture.TestImageFormat(filename);

    CInfosSeparationBarEffect* calqueEffect = CreateNewSeparatorBar(calqueLibelle);
    int numElement = iconeListLocal->GetNbElement();
    calqueEffect->AddPhotoToList(numElement);

    wxImage pBitmap = loadingResource.LoadImageResource("IDB_BLACKROOM");
    auto* thumbnailData = new CThumbnailDataStorage(
        CFiltreData::GetFilterLabel(IDM_FILTRE_VIDEO));
    thumbnailData->SetNumPhotoId(IDM_FILTRE_VIDEO);
    thumbnailData->SetBitmap(CLibPicture::mat_from_wx(pBitmap));

    auto* pBitmapIcone = new CIcone(thumbnailData);
    pBitmapIcone->SetTheme(themeThumbnail.themeIcone);
    iconeListLocal->AddElement(pBitmapIcone);

    isAllProcess = true;


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

void CThumbnailCalque::ProcessIdle()
{
    int nbProcesseur = 1;
    CRegardsConfigParam* pConfig = CParamInit::getInstance();
    if (pConfig != nullptr) nbProcesseur = pConfig->GetThumbnailProcess();
    if (isAllProcess) {
        processIdle = false;
        return;
    }
}


void CThumbnailCalque::UpdateScroll() {
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