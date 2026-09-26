#include <header.h>
#include "ThumbnailVideo.h"
#include <ConvertUtility.h>
#include <ImageVideoThumbnail.h>
#include <SqlPhotos.h>
#include <SqlThumbnail.h>
#include <SqlThumbnailVideo.h>
#include <Thumbnail.h>
#include <ThumbnailDataStorage.h>
#include <appcontext.h>

#include <libPicture.h>

#include "ScrollbarWnd.h"


using namespace Regards::Control;
using namespace Regards::Window;
using namespace Regards::Picture;
using namespace Regards::Sqlite;
#define WX_TIMER_PROCESS 1001
#define wxEVENT_ENDTHUMBNAIL 1002
#define wxEVENT_ENDUPDATEVIDEOTHUMBNAIL 1003

extern AppContext application_context;

CThumbnailVideo::CThumbnailVideo(wxWindow* parent, const wxWindowID id,
    const CThemeThumbnail& themeThumbnail,
    const bool& testValidity)
    : CThumbnailHorizontal(parent, id, themeThumbnail, testValidity) {
    threadPool = std::make_unique<ThreadPool>(1);
    numItemSelected = -1;
    process_end = true;
    nbProcess = 0;
    enableTimer = false;

    Connect(wxEVENT_REFRESHVIDEOTHUMBNAIL,
        wxCommandEventHandler(CThumbnailVideo::UpdateVideoThumbnail));
    Connect(wxEVENT_ICONEUPDATE,
        wxCommandEventHandler(CThumbnailVideo::UpdateThumbnailIcone));
}

void CThumbnailVideo::UpdateThumbnailIcone(wxCommandEvent& event) {
    // Récupération sécurisée du conteneur de thread
    auto* threadLoadingBitmap =
        static_cast<CThreadLoadingBitmap*>(event.GetClientData());
    if (threadLoadingBitmap == nullptr) return;

    nbProcess--;

    // Protection : On vérifie que la fenêtre n'a pas changé de cible entre temps
    if (threadLoadingBitmap->filename == videoFilename) {
        UpdateVideoThumbnail();
    }
    else {
        GenerateThumbnail(videoFilename);
    }

    delete threadLoadingBitmap;  // Libération propre du paramètre
    needToRefresh = true;
}

bool CThumbnailVideo::ItemCompFonct(int videoPos, int y, CIcone* icone,
    CWindowMain* parent) {
    if (icone != nullptr && parent != nullptr) {
        CThumbnailData* data = icone->GetPtData();
        if (data != nullptr && data->GetTimePosition() >= videoPos) {
            return true;
        }
    }
    return false;
}

int CThumbnailVideo::FindNumItem(const int& videoPos) {
    int numItem = -1;
    pItemCompFonct _pf = &ItemCompFonct;
    CIcone* icone = iconeList->FindElementByPosition(videoPos, 0, &_pf, this);
    if (icone != nullptr) {
        if (iFormat < 100) {
            numItem = icone->GetNumElement();
        }
        else {
            CThumbnailData* data = icone->GetPtData();
            if (data != nullptr) {
                numItem = (data->GetTimePosition() == videoPos)
                    ? icone->GetNumElement()
                    : (icone->GetNumElement() - 1);
            }
        }
    }
    return numItem;
}

void CThumbnailVideo::SetVideoPosition(const int64_t& videoPos) {
    const int nbIconeElement = nbElementInIconeList;
    if (nbIconeElement == 0) return;

    int numItem = FindNumItem(videoPos);

    if (numItem == -1 && videoPos > 0) {
        CIcone* icone = iconeList->GetLastElement();
        if (icone != nullptr)
            numItem = icone->GetNumElement();
        else
            return;
    }

    if (numItem == numItemSelected && videoPos > 0) return;

    if (numSelectPhotoId != -1) {
        CIcone* numSelect = GetIconeById(numSelectPhotoId);
        if (numSelect != nullptr) numSelect->SetSelected(false);
    }

    CIcone* pIcone = iconeList->GetElement(numItem);
    if (pIcone != nullptr) {
        pIcone->SetSelected(true);
    }

    if (!isMoving && pIcone != nullptr) {
        wxRect rect = pIcone->GetPos();
        rect.x = posLargeur + rect.x;
        rect.y = posHauteur + rect.y;

        wxWindow* parent = this->GetParent();
        if (parent != nullptr) {
            auto* size = new wxSize();
            wxCommandEvent evt(wxEVENT_SETPOSITION);

            posHauteur = std::max((int)(rect.y - this->GetWindowHeight() / 2), 0);
            posLargeur = std::max((int)(rect.x - this->GetWindowWidth() / 2), 0);

            size->x = posLargeur;
            size->y = posHauteur;
            evt.SetClientData(size);
            parent->GetEventHandler()->AddPendingEvent(evt);
        }
    }

    numSelectPhotoId = iconeList->GetPhotoId(numItem);
    numItemSelected = numItem;
    needToRefresh = true;
}

void CThumbnailVideo::GenerateThumbnail(const wxString& szFileName) {
    if (nbProcess == 0) {
        auto* pLoadBitmap = new CThreadLoadingBitmap();
        pLoadBitmap->filename = szFileName;
        pLoadBitmap->window = this;

        threadPool->Enqueue([pLoadBitmap]() { LoadVideoThumbnail(pLoadBitmap); });

        process_end = false;
        nbProcess++;
    }
}

void CThumbnailVideo::InitWithDefaultPicture(const wxString& szFileName,
    const int& size) {
    int x = 0;
    int y = 0;
    int typeElement = TYPEVIDEO;
    threadDataProcess = false;

    // Utilisation d'un unique_ptr pour éviter toute fuite en cas d'exception de
    // la boucle
    auto iconeListLocal = std::make_unique<CIconeList>();
    CLibPicture libPicture;

    if (iFormat < 100)
        typeElement = TYPEMULTIPAGE;

    CSqlThumbnailVideo sqlThumbnailVideo;
    int nbResult = sqlThumbnailVideo.GetNbThumbnail(szFileName);
    CSqlPhotos SqlPhotos;
    int photoId = SqlPhotos.GetPhotoId(szFileName);

    if (nbResult > 0) {
        for (int i = 0; i < nbResult; i++) {
            auto thumbnail = std::make_unique<CImageVideoThumbnail>();
            sqlThumbnailVideo.GetPictureThumbnail(photoId, szFileName, i,
                thumbnail.get());

            float percent =
                (static_cast<float>(i) / static_cast<float>(size)) * 100.0f;
            auto* thumbnailData = new CThumbnailDataStorage(szFileName);
            thumbnailData->SetNumPhotoId(i);
            thumbnailData->SetNumElement(i);
            thumbnailData->SetTypeElement(typeElement);
            thumbnailData->SetPercent(percent);

            if (typeElement == TYPEMULTIPAGE)
                thumbnailData->SetLibelle("Page : " + std::to_string(i + 1) + "/" +
                    std::to_string(nbResult));

            thumbnailData->SetTimePosition(thumbnail->timePosition);

            if (!thumbnail->image.empty()) {
                thumbnailData->SetBitmap(thumbnail->image);
                thumbnailData->SetIsDefault(false);
            }
            else {
                thumbnailData->SetIsDefault(true);
            }

            auto* pBitmapIcone = new CIcone(thumbnailData);
            pBitmapIcone->SetNumElement(i);
            pBitmapIcone->SetTheme(themeThumbnail.themeIcone);
            pBitmapIcone->SetWindowPos(x, y);

            if (i == 0) {
                pBitmapIcone->SetSelected(true);
                numSelectPhotoId = i;
            }

            iconeListLocal->AddElement(pBitmapIcone);
            x += themeThumbnail.themeIcone.GetWidth();
        }
    }
    else {
        GenerateThumbnail(szFileName);
        std::vector<std::unique_ptr<CImageVideoThumbnail>> listThumbnail =
            libPicture.LoadDefaultVideoThumbnail(szFileName, size);

        for (auto j = 0; j < size; j++) {
            float percent =
                (static_cast<float>(j) / static_cast<float>(size)) * 100.0f;
            CImageVideoThumbnail* thumbnail = listThumbnail[j].get();
            auto* thumbnailData = new CThumbnailDataStorage(szFileName);
            thumbnailData->SetNumPhotoId(j);
            thumbnailData->SetNumElement(j);
            thumbnailData->SetTypeElement(typeElement);
            thumbnailData->SetPercent(percent);
            thumbnailData->SetTimePosition(thumbnail->timePosition);

            if (thumbnail->image.empty()) {
                thumbnail->image = application_context.GetDefaultPicture();
            }

            thumbnailData->SetBitmap(thumbnail->image);
            if (typeElement == TYPEMULTIPAGE)
                thumbnailData->SetLibelle("Page : " + std::to_string(j + 1) + "/" +
                    std::to_string(size));

            auto* pBitmapIcone = new CIcone(thumbnailData);
            pBitmapIcone->SetNumElement(j);
            pBitmapIcone->SetTheme(themeThumbnail.themeIcone);
            pBitmapIcone->SetWindowPos(x, y);

            if (j == 0) {
                pBitmapIcone->SetSelected(true);
                numSelectPhotoId = j;
            }

            iconeListLocal->AddElement(pBitmapIcone);
            x += themeThumbnail.themeIcone.GetWidth();
        }
    }

    // Permutation sécurisée des conteneurs
    if (iconeList) {
        iconeList->EraseThumbnailListWithIcon();
    }
    iconeList = std::move(iconeListLocal);

    nbElementInIconeList = iconeList->GetNbElement();
    threadDataProcess = true;
    UpdateScroll();
    processThumbnailVideo = true;
    needToRefresh = true;
}

void CThumbnailVideo::LoadVideoThumbnail(void* param) {
    CLibPicture libPicture;
    auto* threadLoadingBitmap = static_cast<CThreadLoadingBitmap*>(param);
    if (threadLoadingBitmap == nullptr) return;

    std::vector<std::unique_ptr<CImageVideoThumbnail>> listVideo =
        libPicture.LoadAllVideoThumbnail(threadLoadingBitmap->filename, true,
            true);

    if (!listVideo.empty()) {
        CSqlThumbnailVideo sqlThumbnailVideo;
        CSqlPhotos SqlPhotos;
        wxString filename = threadLoadingBitmap->filename;
        int photoId = SqlPhotos.GetPhotoId(filename);
        if (photoId != -1) {
            for (size_t i = 0; i < listVideo.size(); i++) {
                CImageVideoThumbnail* bitmap = listVideo[i].get();
                if (!bitmap->image.empty()) {
                    wxString localName = sqlThumbnailVideo.InsertThumbnail(
                        photoId, filename, bitmap->image.size().width,
                        bitmap->image.size().height, static_cast<int>(i),
                        bitmap->rotation, bitmap->percent, bitmap->timePosition);

                    cv::imwrite(CConvertUtility::ConvertToStdString(localName),
                        bitmap->image);
                }

                if (i == 0) threadLoadingBitmap->bitmapIcone = bitmap->image;
            }
        }
        threadLoadingBitmap->isAnimationOrVideo = true;
    }
    else {
        threadLoadingBitmap->bitmapIcone = application_context.GetDefaultPicture();
        wxString filename = threadLoadingBitmap->filename;

        CSqlPhotos SqlPhotos;
        int photoId = SqlPhotos.GetPhotoId(filename);
        if (photoId != -1) {
            CSqlThumbnailVideo sqlThumbnailVideo;
            wxString localName = sqlThumbnailVideo.InsertThumbnail(
                photoId, filename,
                application_context.GetWxDefaultPicture().GetWidth(),
                application_context.GetWxDefaultPicture().GetHeight(), 0, 0, 0, 0);
            application_context.GetWxDefaultPicture().SaveFile(localName,
                wxBITMAP_TYPE_JPEG);
        }
    }

    auto* event = new wxCommandEvent(wxEVENT_ICONEUPDATE);
    event->SetClientData(threadLoadingBitmap);
    wxQueueEvent(threadLoadingBitmap->window, event);
}

void CThumbnailVideo::UpdateVideoThumbnail() {
    if (!videoFilename.IsEmpty()) {
        CSqlThumbnailVideo sqlThumbnailVideo;
        int nbResult = sqlThumbnailVideo.GetNbThumbnail(videoFilename);
        CSqlPhotos SqlPhotos;
        int photoId = SqlPhotos.GetPhotoId(videoFilename);
        if (nbResult > 0) {
            for (int i = 0; i < nbResult; i++) {
                CIcone* pBitmapIcone = iconeList->GetElement(i);
                if (pBitmapIcone != nullptr) {
                    // Remplacement du vieux cast C par un dynamic_cast sécurisé
                    auto* thumbnailData =
                        dynamic_cast<CThumbnailDataStorage*>(pBitmapIcone->GetPtData());
                    if (thumbnailData != nullptr) {
                        auto thumbnail = std::make_unique<CImageVideoThumbnail>();
                        sqlThumbnailVideo.GetPictureThumbnail(photoId, videoFilename, i,
                            thumbnail.get());
                        thumbnail->percent = static_cast<float>(i) / static_cast<float>(nbResult) * 100.0f;
                        if (!thumbnail->image.empty()) {
                            thumbnailData->SetIsDefault(false);
                            thumbnailData->SetBitmap(thumbnail->image);
                        }
                        thumbnailData->SetTimePosition(thumbnail->timePosition);
                    }
                }
            }
        }
    }
    process_end = true;
}

void CThumbnailVideo::EraseThumbnail(long value) {
    if (value == 1) {
        CSqlPhotos SqlPhotos;
        int photoId = SqlPhotos.GetPhotoId(videoFilename);
        CSqlThumbnailVideo sqlThumbnailvideo;
        sqlThumbnailvideo.DeleteThumbnail(photoId);
        CSqlThumbnail sqlThumbnail;
        sqlThumbnail.DeleteThumbnail(videoFilename);
    }
    thumbnailPos = 0;
    for (int i = 0; i < nbElementInIconeList; i++) {
        if (videoFilename == iconeList->GetFilename(i)) {
            CIcone* pIcone = iconeList->GetElement(i);
            if (pIcone != nullptr) {
                auto* pThumbnailData =
                    dynamic_cast<CThumbnailDataStorage*>(pIcone->GetPtData());
                if (pThumbnailData != nullptr) {
                    pThumbnailData->InitLoadState();
                    pThumbnailData->SetIsDefault(true);
                    pThumbnailData->SetIsProcess(false);
                    pThumbnailData->SetIsLoading(false);
                }
            }
        }
    }
    CLibPicture libPicture;
    int nbImage = libPicture.GetNbImage(videoFilename);
    InitScrollingPos();
    InitWithDefaultPicture(videoFilename, nbImage);
    process_end = false;
    threadDataProcess = true;
    needToRefresh = true;
    wxWindow* window = this->FindWindowById(MAINVIEWERWINDOWID);
    if (window != nullptr) {
        auto* localName = new wxString(videoFilename);
        wxCommandEvent evt(wxEVENT_ICONETHUMBNAILGENERATION);
        evt.SetClientData(localName);
        evt.SetInt(1);
        evt.SetExtraLong(localid);
        window->GetEventHandler()->AddPendingEvent(evt);
    }
}
void CThumbnailVideo::UpdateVideoThumbnail(const wxString& videoFile) {
    if (videoFilename == videoFile)
        UpdateVideoThumbnail();
    else
        process_end = true;
    needToRefresh = true;
}
void CThumbnailVideo::UpdateVideoThumbnail(wxCommandEvent& event) {
    UpdateVideoThumbnail();
}
void CThumbnailVideo::ResizeThumbnail() {
    UpdateScroll();
    UpdateVideoThumbnail();
}
void CThumbnailVideo::EraseThumbnail(wxCommandEvent& event) {
    EraseThumbnail(event.GetExtraLong());
}
void CThumbnailVideo::SetFile(const wxString& videoFile, const int& size) {
    process_end = false;
    CLibPicture libPicture;
    iFormat = libPicture.TestImageFormat(videoFile);
    videoFilename = videoFile;
    InitScrollingPos();
    InitWithDefaultPicture(videoFile, size);
    process_end = false;
    threadDataProcess = true;
    needToRefresh = true;
}