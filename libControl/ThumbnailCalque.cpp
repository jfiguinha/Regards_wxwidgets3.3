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
    inverse = true;
    config = CParamInit::getInstance();

}

void CThumbnailCalque::OnPictureClick(const int& numPhotoId)
{

}


void CThumbnailCalque::RefreshList()
{
    auto* iconeListLocal = new CIconeList();
    int numElement = 0;
    InitScrollingPos();



    for (LayerElement* layerElement : *listOfLayer)
    {
        CImageLoadingFormat* picture = layerElement->GetPicture();
        auto* thumbnailData = new CThumbnailDataStorage(
            layerElement->layerName);
        thumbnailData->SetNumPhotoId(numElement);
        thumbnailData->SetBitmap(picture->GetMatImage().clone());

        auto* pBitmapIcone = new CLayerIcone(thumbnailData);
        pBitmapIcone->SetNumElement(numElement);
        pBitmapIcone->SetTheme(themeThumbnail.themeIcone);
        pBitmapIcone->SetLibelle(layerElement->layerName);
        pBitmapIcone->SetChecked(true);
        iconeListLocal->AddElement(pBitmapIcone);

        numElement++;
    }


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

void CThumbnailCalque::SetLayer(CLayerList * listOfLayer)
{
    this->listOfLayer = listOfLayer;
    RefreshList();
}

CThumbnailCalque::~CThumbnailCalque(void) { }

//wxString CThumbnailCalque::GetFilename() { return filename; }

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


vector<int> CThumbnailCalque::GetSelectLayer()
{
    vector<int> listCheck;
    int nbCheck = 0;
    int nbElement = nbElementInIconeList;
    for (int i = 0; i < nbElement; i++)
    {
        CIcone* icone = iconeList->GetElement(i);
        if (icone->IsChecked())
            listCheck.push_back(icone->GetNumElement());
    }
    return listCheck;
}


int CThumbnailCalque::GetActifLayer()
{
    return numClickIcone;
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

