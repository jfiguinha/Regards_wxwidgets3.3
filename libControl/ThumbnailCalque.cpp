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
#include <unordered_set>
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
    

}

void CThumbnailCalque::OnPictureClick(const int& numPhotoId)
{
    bool hasChanged = false;
    CIconeList* list = iconeList.get();
    int numElement = 0;
    for (LayerElement* layerElement : *listOfLayer)
    {
        auto* pBitmapIcone = list->GetElement(numElement);
        if (layerElement->isVisible != pBitmapIcone->IsChecked())
        {
            layerElement->isVisible = pBitmapIcone->IsChecked();
            hasChanged = true;
        }
            
        numElement++;
    }


    if (actifLayer != numClickIcone || hasChanged)
    {
        actifLayer = numClickIcone;
        wxWindow* mainWindow = FindWindowById(frameId);
        if (mainWindow)
        {
            wxCommandEvent evt(wxEVENT_CHANGELAYERPICTURE);
            evt.SetInt(actifLayer);
            mainWindow->GetEventHandler()->AddPendingEvent(evt);
        }
    }
        
}

void CThumbnailCalque::RefreshList()
{
    CIconeList* list = iconeList.get();
    if (!list || !listOfLayer) return; // Sécurité anti-crash

    // Parcourir la liste pour mettre à jour les vignettes
    for (int element = 0; element < list->GetNbElement(); element++)
    {
        auto* pBitmapIcone = list->GetElement(element);
        LayerElement* layerElement = (*listOfLayer)[element]; // Utilisation de l'opérateur [] de CLayerList

        if (!pBitmapIcone || !layerElement) continue;

        CImageLoadingFormat* picture = layerElement->GetPicture();
        auto* thumbnailData = pBitmapIcone->GetPtData();

        if (picture && picture->IsOk() && thumbnailData)
        {
            cv::Mat output;
            cv::Mat matrixPicture = picture->GetMatImage();

            // Taille maximale définie par la configuration du thème de l'icône
            int maxWidth = themeThumbnail.themeIcone.GetWidth();
            int maxHeight = themeThumbnail.themeIcone.GetHeight();

            if (maxWidth > 0 && maxHeight > 0 && !matrixPicture.empty())
            {
                // --- CALCUL DE RECADRAGE RESPECTANT LE RATIO D'ORIGINE ---
                int srcWidth = matrixPicture.cols;
                int srcHeight = matrixPicture.rows;

                int targetWidth = maxWidth;
                int targetHeight = maxHeight;

                // Comparaison des ratios pour adapter la boîte englobante
                double ratioSrc = static_cast<double>(srcWidth) / srcHeight;
                double ratioMax = static_cast<double>(maxWidth) / maxHeight;

                if (ratioSrc > ratioMax)
                {
                    // L'image d'origine est plus large : on fixe la largeur maximale
                    targetHeight = static_cast<int>(maxWidth / ratioSrc);
                }
                else
                {
                    // L'image d'origine est plus haute : on fixe la hauteur maximale
                    targetWidth = static_cast<int>(maxHeight * ratioSrc);
                }

                // Sécurité pour éviter des dimensions nulles suite aux arrondis
                if (targetWidth <= 0) targetWidth = 1;
                if (targetHeight <= 0) targetHeight = 1;

                // Redimensionnement de la matrice OpenCV avec les nouvelles proportions proportionnelles
                cv::resize(matrixPicture, output, cv::Size(targetWidth, targetHeight));
                thumbnailData->SetBitmap(output);
            }
        }
    }

    isAllProcess = true;
    threadDataProcess = true;
    processIdle = true;

    UpdateScroll();
    ResizeThumbnail();
    needToRefresh = true;
}

void CThumbnailCalque::UpdateLayerList()
{
    auto* iconeListLocal = new CIconeList();
    int numElement = 0;
    InitScrollingPos();

    // Utilisation d'un set pour stocker les IDs trouvés (Recherche en O(1))
    std::unordered_set<uint64_t> listOfLayerFind;
    CIconeList* oldList = iconeList.get();

    // Dimensions maximales de l'icône issues du thème
    int maxWidth = themeThumbnail.themeIcone.GetWidth();
    int maxHeight = themeThumbnail.themeIcone.GetHeight();

    // Valeurs de repli par défaut si le thème n'est pas initialisé
    if (maxWidth <= 0) maxWidth = 640;
    if (maxHeight <= 0) maxHeight = 480;

    for (LayerElement* layerElement : *listOfLayer)
    {
        if (!layerElement) continue;

        CLayerIcone* pBitmapIcone = nullptr;

        // Recherche de l'icône existante par son layerId
        if (oldList)
        {
            pBitmapIcone = static_cast<CLayerIcone*>(oldList->FindElementByPhotoId(layerElement->layerId));
        }

        CImageLoadingFormat* picture = layerElement->GetPicture();

        // Calcul des dimensions de rendu respectant le ratio d'aspect original du calque
        int finalWidth = maxWidth;
        int finalHeight = maxHeight;

        if (picture && picture->IsOk())
        {
            cv::Mat matrixPicture = picture->GetMatImage();
            if (!matrixPicture.empty())
            {
                int srcWidth = matrixPicture.cols;
                int srcHeight = matrixPicture.rows;

                double ratioSrc = static_cast<double>(srcWidth) / srcHeight;
                double ratioMax = static_cast<double>(maxWidth) / maxHeight;

                if (ratioSrc > ratioMax)
                {
                    // Plus large : fixer la largeur maximale
                    finalHeight = static_cast<int>(maxWidth / ratioSrc);
                }
                else
                {
                    // Plus haut : fixer la hauteur maximale
                    finalWidth = static_cast<int>(maxHeight * ratioSrc);
                }

                if (finalWidth <= 0) finalWidth = 1;
                if (finalHeight <= 0) finalHeight = 1;
            }
        }

        if (pBitmapIcone == nullptr)
        {
            // --- CAS 1 : NOUVELLE ICÔNE ---
            auto* thumbnailData = new CThumbnailDataStorage(layerElement->layerName);
            thumbnailData->SetNumPhotoId(layerElement->layerId);

            if (picture && picture->IsOk())
            {
                cv::Mat output;
                cv::Mat matrixPicture = picture->GetMatImage();
                if (!matrixPicture.empty())
                {
                    cv::resize(matrixPicture, output, cv::Size(finalWidth, finalHeight));
                    thumbnailData->SetBitmap(output);
                }
            }

            pBitmapIcone = new CLayerIcone(thumbnailData);
            pBitmapIcone->SetNumElement(numElement);
            pBitmapIcone->SetTheme(themeThumbnail.themeIcone);
            pBitmapIcone->SetLibelle(layerElement->layerName);
            pBitmapIcone->SetChecked(layerElement->isVisible);
            pBitmapIcone->SetSelected(numElement == numClickIcone);
        }
        else
        {
            // --- CAS 2 : RÉUTILISATION DE L'ICÔNE ---
            listOfLayerFind.insert(layerElement->layerId);
            pBitmapIcone->SetNumElement(numElement);
            pBitmapIcone->SetLibelle(layerElement->layerName);
            pBitmapIcone->SetChecked(layerElement->isVisible);
            pBitmapIcone->SetSelected(numElement == numClickIcone);

            auto* thumbnailData = pBitmapIcone->GetPtData();
            if (thumbnailData && picture && picture->IsOk())
            {
                cv::Mat output;
                cv::Mat matrixPicture = picture->GetMatImage();
                if (!matrixPicture.empty())
                {
                    cv::resize(matrixPicture, output, cv::Size(finalWidth, finalHeight));
                    thumbnailData->SetBitmap(output);
                }
            }
        }

        iconeListLocal->AddElement(pBitmapIcone);
        numElement++;
    }

    // --- NETTOYAGE SÉCURISÉ DES ANCIENNES ICÔNES NON REPRISES ---
    if (oldList)
    {
        for (int i = 0; i < oldList->GetNbElement(); i++)
        {
            CLayerIcone* oldIcone = static_cast<CLayerIcone*>(oldList->GetElement(i));
            if (oldIcone && oldIcone->GetPtData())
            {
                auto* thumbnailData = oldIcone->GetPtData();
                if (listOfLayerFind.find(thumbnailData->GetNumPhotoId()) == listOfLayerFind.end())
                {
                    delete oldIcone;
                }
            }
        }
    }

    isAllProcess = true;

    iconeList.reset(iconeListLocal);
    nbElementInIconeList = iconeList->GetNbElement();

    threadDataProcess = true;
    processIdle = true;

    UpdateScroll();
    ResizeThumbnail();
    needToRefresh = true;
}


void CThumbnailCalque::SetLayer(CLayerList * listOfLayer)
{
    this->listOfLayer = listOfLayer;
    UpdateLayerList();
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


void CThumbnailCalque::SetActifLayer(const int& numElement)
{

    
    {
        CIcone* icone = iconeList->GetElement(numClickIcone);
        if (icone)
            icone->SetSelected(false);
    }

    {
        CIcone* icone = iconeList->GetElement(numElement);
        if (icone)
            icone->SetSelected(true);
    }
}

int64_t CThumbnailCalque::GetActifLayerId()
{
    CIcone* icone = iconeList->GetElement(numClickIcone);
    if (icone)
    {
        auto* thumbnailData = icone->GetPtData();
        return thumbnailData->GetNumPhotoId();
    }
    return -1;
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

