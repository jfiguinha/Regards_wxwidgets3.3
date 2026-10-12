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

            // Correction : Utiliser la taille définie par le thème
            int targetWidth = themeThumbnail.themeIcone.GetWidth();
            int targetHeight = themeThumbnail.themeIcone.GetHeight();

            if (targetWidth > 0 && targetHeight > 0 && !matrixPicture.empty())
            {
                cv::resize(matrixPicture, output, cv::Size(targetWidth, targetHeight));
                // CORRECTION ICI : On passe la matrice redimensionnée 'output' et non 'matrixPicture'
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

    int targetWidth = themeThumbnail.themeIcone.GetWidth();
    int targetHeight = themeThumbnail.themeIcone.GetHeight();

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

        if (pBitmapIcone == nullptr)
        {
            // --- CAS 1 : NOUVELLE ICÔNE ---
            auto* thumbnailData = new CThumbnailDataStorage(layerElement->layerName);
            thumbnailData->SetNumPhotoId(layerElement->layerId);

            if (picture && picture->IsOk() && targetWidth > 0 && targetHeight > 0)
            {
                cv::Mat output;
                cv::Mat matrixPicture = picture->GetMatImage();
                if (!matrixPicture.empty())
                {
                    cv::resize(matrixPicture, output, cv::Size(targetWidth, targetHeight));
                    thumbnailData->SetBitmap(output); // CORRIGÉ : On passe 'output' et non 'matrixPicture'
                }
            }

            pBitmapIcone = new CLayerIcone(thumbnailData);
            pBitmapIcone->SetNumElement(numElement);
            pBitmapIcone->SetTheme(themeThumbnail.themeIcone);
            pBitmapIcone->SetLibelle(layerElement->layerName);
            pBitmapIcone->SetChecked(layerElement->isVisible); // CORRIGÉ : Initialise avec la vraie visibilité
            pBitmapIcone->SetSelected(numElement == numClickIcone);
        }
        else
        {
            // --- CAS 2 : RÉUTILISATION DE L'ICÔNE ---
            listOfLayerFind.insert(layerElement->layerId);
            pBitmapIcone->SetNumElement(numElement);
            pBitmapIcone->SetChecked(layerElement->isVisible); // Synchro de l'état de visibilité
            pBitmapIcone->SetSelected(numElement == numClickIcone);

            // Optionnel : Ne recalculer la vignette que si le calque a changé (ex: pictureVersion)
            auto* thumbnailData = pBitmapIcone->GetPtData();
            if (thumbnailData && picture && picture->IsOk() && targetWidth > 0 && targetHeight > 0)
            {
                cv::Mat output;
                cv::Mat matrixPicture = picture->GetMatImage();
                if (!matrixPicture.empty())
                {
                    cv::resize(matrixPicture, output, cv::Size(targetWidth, targetHeight));
                    thumbnailData->SetBitmap(output); // CORRIGÉ : On passe 'output' et non 'matrixPicture'
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
                // Si l'ancienne icône n'a pas été marquée comme réutilisée, on la supprime
                if (listOfLayerFind.find(thumbnailData->GetNumPhotoId()) == listOfLayerFind.end())
                {
                    delete oldIcone;
                }
            }
        }
        // ATTENTION : Ne pas appeler EraseThumbnailList() ici si cela supprime à l'aveugle
        // des pointeurs que nous venons de transférer à iconeListLocal. 
        // Videz simplement le conteneur interne de oldList sans delete si votre API le permet.
    }

    isAllProcess = true;

    // Remplacement de l'ancienne liste par la nouvelle
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

