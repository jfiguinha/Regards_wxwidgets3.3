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


CThumbnailCalque::CThumbnailCalque(wxWindow* parent, const wxWindowID id, const wxWindowID frameId,
    const CThemeThumbnail& themeThumbnail)
    : CThumbnailVertical(parent, id, themeThumbnail, false) {

    this->frameId = frameId;
    processIdle = false;
    moveOnPaint = false;
    inverse = true;
    config = CParamInit::getInstance();
    background = CLibPicture::CreateCheckerboardBackground(640, 480);

}

void CThumbnailCalque::OnPictureClick(const int& numPhotoId)
{

}

cv::Mat CThumbnailCalque::OverlayImage(const cv::Mat& background, const cv::Mat& foreground) {
    cv::Mat output;
    background.copyTo(output);
    // Séparer les canaux de l'image de premier plan (BGRA)
    std::vector<cv::Mat> fgChannels;
    cv::split(foreground, fgChannels); // 0:B, 1:G, 2:R, 3:Alpha

    cv::Mat alpha = fgChannels[3];
    cv::Mat alphaF, alphaInvF;

    // Convertir l'alpha en float entre 0.0 et 1.0
    alpha.convertTo(alphaF, CV_32F, 1.0 / 255.0);
    alphaInvF = 1.0 - alphaF;

    // Traiter chaque canal B, G, R
    for (int i = 0; i < 3; ++i) {
        cv::Mat fgChannelsF, bgChannelsF, blendedF;

        fgChannels[i].convertTo(fgChannelsF, CV_32F);
        background.channels() == 3 ? background.convertTo(bgChannelsF, CV_32F) : bgChannelsF = cv::Mat::zeros(background.size(), CV_32F); // Sécurité type

        // Extraction du canal i de l'arrière-plan si c'est du multi-canal
        std::vector<cv::Mat> bgSubChannels;
        cv::split(background, bgSubChannels);
        bgSubChannels[i].convertTo(bgChannelsF, CV_32F);

        // Formule mathématique : I = alpha * Foreground + (1 - alpha) * Background
        cv::multiply(fgChannelsF, alphaF, fgChannelsF);
        cv::multiply(bgChannelsF, alphaInvF, bgChannelsF);
        cv::add(fgChannelsF, bgChannelsF, blendedF);

        // Réinsertion du canal fusionné
        blendedF.convertTo(bgSubChannels[i], CV_8U);
        cv::merge(bgSubChannels, background);
    }

    return output;
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

        cv::Mat output;
        cv::Mat matrixPicture = picture->GetMatImage();
        cv::resize(matrixPicture, output, cv::Size(640, 480));

        output = OverlayImage(output, background);
        thumbnailData->SetBitmap(output);

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

