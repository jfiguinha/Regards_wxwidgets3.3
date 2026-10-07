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
#include "LayerIcone.h"
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
    : CThumbnailVertical(parent, id, themeThumbnail, false) {

    processIdle = false;
    moveOnPaint = false;

    config = CParamInit::getInstance();

}

void CThumbnailCalque::OnPictureClick(const int& numPhotoId)
{

}

CThumbnailCalque::~CThumbnailCalque(void) { }

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



void CThumbnailCalque::SetFile(const wxString& filename,
    CImageLoadingFormat* imageLoading) {
    auto* iconeListLocal = new CIconeList();
    threadDataProcess = false;
    processIdle = false;


    CLoadingResource loadingResource;
    this->filename = filename;
    InitScrollingPos();

    CRgbaquad color;
    CFiltreEffet filtreEffet(color, nullptr, imageLoading);
    cv::Mat output = filtreEffet.Resize(640, 480, 1);

    CLibPicture picture;
    int format = picture.TestImageFormat(filename);

    int numElement = iconeListLocal->GetNbElement();

    wxImage pBitmap = loadingResource.LoadImageResource("IDB_BLACKROOM");
    auto* thumbnailData = new CThumbnailDataStorage(
        CFiltreData::GetFilterLabel(IDM_FILTRE_VIDEO));
    thumbnailData->SetNumPhotoId(IDM_FILTRE_VIDEO);
    thumbnailData->SetBitmap(output);

    auto* pBitmapIcone = new CLayerIcone(thumbnailData);
    pBitmapIcone->SetTheme(themeThumbnail.themeIcone);
    pBitmapIcone->SetLibelle("First Layer");
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

void CThumbnailCalque::Resize()
{
	this->SetIconeSize(this->GetWindowWidth(), 50);
	this->ResizeThumbnail();    
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

