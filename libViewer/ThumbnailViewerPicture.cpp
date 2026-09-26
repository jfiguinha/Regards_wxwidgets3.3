#include <header.h>
#include "ThumbnailViewerPicture.h"
#include "MainWindow.h"
#include <ThumbnailDataSQL.h>
#include <ScrollbarWnd.h>
#include "ThumbnailBuffer.h"
#include <SqlFindPhotos.h>
#include <wx/progdlg.h>
using namespace Regards::Viewer;
using namespace Regards::Sqlite;

//#define TEST


std::mutex CThumbnailViewerPicture::localmu;

CThumbnailViewerPicture::CThumbnailViewerPicture(wxWindow* parent, wxWindowID id, const CThemeThumbnail& themeThumbnail,
	const bool& testValidity)
	: CThumbnailVertical(parent, id, themeThumbnail, testValidity)
{
	widthThumbnail = 0;
	heightThumbnail = 0;
	preprocess_thumbnail = true;
	
}


void CThumbnailViewerPicture::OnPictureClick(const int& numPhotoId)
{
	auto mainWindow = static_cast<CMainWindow*>(this->FindWindowById(MAINVIEWERWINDOWID));
	if (mainWindow != nullptr)
	{
		wxCommandEvent evt(wxEVENT_ONPICTURECLICK);
		evt.SetExtraLong(numPhotoId);
		mainWindow->GetEventHandler()->AddPendingEvent(evt);
	}
}

vector<wxString> CThumbnailViewerPicture::GetFileList()
{
	vector<wxString> list;
	CSqlFindPhotos sqlFindPhotos;
	sqlFindPhotos.SearchPhotos(&list);

	return list;
}

void CThumbnailViewerPicture::PregenerateList(const bool& isDeleteFolder, const bool& isSqlUpdate)
{
    // [Bloc de code TEST inchangé si inutilisé]

    int size = CThumbnailBuffer::GetVectorSize();
    if (size == 0)
    {
        iconeList->EraseThumbnailListWithIcon();
        nbElementInIconeList = 0;
    }
    else
    {
        int iconWidth = themeThumbnail.themeIcone.GetWidth();
        if ((isDeleteFolder || isSqlUpdate) && nbElementInIconeList > 0)
        {
            int sizeList = iconeList->GetNbElement();
            if (sizeList > 0)
            {
                CIconeList* newIconeList = new CIconeList();
                GenerateList(newIconeList);

                if (newIconeList->GetNbElement() > 0)
                {
                    auto old = std::move(iconeList);
                    iconeList.reset(newIconeList);
                    nbElementInIconeList = iconeList->GetNbElement();
                }
                else
                {
                    delete newIconeList;
                }
            }
            else if (CThumbnailBuffer::GetVectorSize() == 0)
            {
                iconeList->EraseThumbnailListWithIcon();
            }
        }

        if (isSqlUpdate || !isDeleteFolder)
        {
            // 1. Passe Parallèle : On prépare uniquement les NOUVEAUX éléments lourds
            std::vector<CIcone*> tempNewIcones(size, nullptr);

            tbb::parallel_for(0, size, 1, [this, &tempNewIcones](int i)
                {
                    try
                    {
                        CPhotos photo = CThumbnailBuffer::GetVectorValue(i);
                        wxString filename = photo.GetPath();

                        bool find = iconeList->IfElementExistByFilename(filename);
                        if (!find)
                        {
                            auto thumbnailData = new CThumbnailDataSQL(filename, false, false);
                            thumbnailData->SetNumPhotoId(photo.GetId());

                            auto pBitmapIcone = new CIcone(thumbnailData);
                            pBitmapIcone->ShowSelectButton(true);
                            pBitmapIcone->SetFilename(filename);
                            pBitmapIcone->SetTheme(themeThumbnail.themeIcone);

                            // Stockage temporaire thread-safe
                            tempNewIcones[i] = pBitmapIcone;
                        }
                    }
                    catch (const std::exception& e)
                    {
                        std::cerr << "Error creating icon at index " << i << ": " << e.what() << std::endl;
                    }
                });

            // 2. Passe Séquentielle : Insertion sécurisée des nouveautés
            for (int i = 0; i < size; i++)
            {
                if (tempNewIcones[i] != nullptr)
                {
                    iconeList->AddElement(tempNewIcones[i]);
                }
            }
        }

        // 3. Tri global de la liste (indispensable d'attendre que tout y soit inséré)
        iconeList->SortByFilename();

        // 4. Recalcul strict des index et des positions X après le tri
        // Cette étape mono-thread est instantanée et évite tous les trous et décalages d'affichage
        nbElementInIconeList = iconeList->GetNbElement();
        for (int i = 0; i < nbElementInIconeList; i++)
        {
            CIcone* icone = iconeList->GetElement(i);
            if (icone != nullptr)
            {
                icone->SetNumElement(i);
                icone->SetWindowPos(i * iconWidth, 0); // Position X continue sans espace vide

                auto data = static_cast<CThumbnailDataSQL*>(icone->GetPtData());
                if (data != nullptr)
                {
                    data->SetNumElement(i);
                }
            }
        }
    }
}



void CThumbnailViewerPicture::ApplyListeFile(const bool& isDeleteFolder, const bool& isSqlUpdate)
{
	threadDataProcess = false;
	PregenerateList(isDeleteFolder, isSqlUpdate);
	nbElementInIconeList = iconeList->GetNbElement();
	AfterSetList();
	ResizeThumbnail();
	threadDataProcess = true;
	needToRefresh = true;
}

void CThumbnailViewerPicture::ResizeThumbnail()
{
	ResizeThumbnailWithoutVScroll();
	widthThumbnail = GetWindowWidth();
	heightThumbnail = GetWindowHeight();
	UpdateScroll();
}

void CThumbnailViewerPicture::ResizeThumbnailWithoutVScroll()
{
	int iconWidth = themeThumbnail.themeIcone.GetWidth();
#ifndef USE_TBB_VECTOR
	for (int i = 0; i < nbElementInIconeList; i++)
#else
	tbb::parallel_for(0, nbElementInIconeList, 1, [this, iconWidth](int i)
#endif    
		{
			CIcone* pBitmapIcone = iconeList->GetElement(i);
			if (pBitmapIcone != nullptr)
			{
				pBitmapIcone->SetTheme(themeThumbnail.themeIcone);
				pBitmapIcone->SetWindowPos(i * iconWidth, 0);
			}
		}
#ifdef USE_TBB_VECTOR
	);
#endif
}


void CThumbnailViewerPicture::RenderIconeWithoutVScroll(wxDC* deviceContext)
{
	for (int i = 0; i < nbElementInIconeList; i++)
	{
		try
		{
			CIcone* pBitmapIcone = iconeList->GetElement(i);
			if (pBitmapIcone != nullptr)
			{
				pBitmapIcone->SetTheme(themeThumbnail.themeIcone);
				const wxRect rc = pBitmapIcone->GetPos();

				const int left = rc.x - posLargeur;
				const int right = rc.x + rc.width - posLargeur;

				if (right >= 0 && left <= GetWindowWidth())
				{
					RenderBitmap(deviceContext, pBitmapIcone, -posLargeur, 0);
				}
			}
		}
		catch (const std::exception& e)
		{
			std::cerr << "Error rendering icon at index " << i << ": " << e.what() << std::endl;
		}
	}
}


bool CThumbnailViewerPicture::ItemCompFonct(int xPos, int yPos, CIcone* icone, CWindowMain* parent)
{
	auto viewerPicture = static_cast<CThumbnailViewerPicture*>(parent);
	const wxRect rc = icone->GetPos();

	const int left = rc.x - viewerPicture->GetLargeur();
	const int right = rc.x + rc.width - viewerPicture->GetLargeur();
	const int top = rc.y - viewerPicture->GetHauteur();
	const int bottom = rc.y + rc.height - viewerPicture->GetHauteur();

	return (left < xPos && xPos < right) && (top < yPos && yPos < bottom);
}

CIcone* CThumbnailViewerPicture::FindElement(const int& xPos, const int& yPos)
{
	pItemCompFonct _pf = &ItemCompFonct;
	return iconeList->FindElementByPosition(xPos, yPos, &_pf, this);
}
