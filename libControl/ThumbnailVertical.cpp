// ReSharper disable All
#include <header.h>
#include "ThumbnailVertical.h"
#include "ScrollbarWnd.h"
#include "ThumbnailBuffer.h"
#include <RegardsConfigParam.h>
#include <wx/progdlg.h>
using namespace Regards::Control;

CThumbnailVertical::CThumbnailVertical(wxWindow* parent, const wxWindowID id, const CThemeThumbnail& themeThumbnail,
                                       const bool& testValidity)
	: CThumbnail(parent, id, themeThumbnail, testValidity), test_validity_(testValidity),
	  theme_thumbnail_(themeThumbnail), id_(id)
{
	noVscroll = false;
}


CThumbnailVertical::~CThumbnailVertical(void)
= default;

void CThumbnailVertical::GenerateList(CIconeList* & newIconeList)
{
	int size = iconeList->GetNbElement();

	for (int i = 0; i < size; i++)
	{
		CIcone* ico = iconeList->GetElement(i);
		if (ico != nullptr)
		{
			bool find = CThumbnailBuffer::FindValidFile(ico->GetFilename());
			if (!find)
			{
				iconeList->RemoveElement(i);
				--i;
			}
			else
				newIconeList->AddElement(ico);
		}
	}
}

void CThumbnailVertical::OnScrollBarH(wxCommandEvent& event)
{
	int isScrollBarH = event.GetInt();
	long scrollBarHSize = event.GetExtraLong();
	if (isScrollBarH)
		themeThumbnail.themeIcone.SetHeight(themeIconeHeight);
	else
		themeThumbnail.themeIcone.SetHeight(themeIconeHeight + scrollBarHSize);

	ResizeThumbnail();
}

CIcone * CThumbnailVertical::FindElementWithVScroll(const int& xPos, const int& yPos)
{
	int x = posLargeur + xPos;
	int y = posHauteur + yPos;

	int nbElementByX = thumbnailSizeX / themeThumbnail.themeIcone.GetWidth();
	int numY = y / themeThumbnail.themeIcone.GetHeight();
	int numX = x / themeThumbnail.themeIcone.GetWidth();
	int numElement = numY * nbElementByX + numX;

	if (numElement >= nbElementInIconeList)
		return nullptr;

	return iconeList->GetElement(numElement);
}

CIcone * CThumbnailVertical::FindElementWithoutVScroll(const int& xPos, const int& yPos)
{
	int x = posLargeur + xPos;
	if (x > thumbnailSizeX)
		return nullptr;

	int numElement = x / themeThumbnail.themeIcone.GetWidth();

	if (numElement >= nbElementInIconeList)
		return nullptr;

	return iconeList->GetElement(numElement);
}

CIcone * CThumbnailVertical::FindElement(const int& xPos, const int& yPos)
{
	if (noVscroll)
		return FindElementWithoutVScroll(xPos, yPos);

	return FindElementWithVScroll(xPos, yPos);
}

void CThumbnailVertical::SetNoVScroll(const bool& noVscroll)
{
	this->noVscroll = noVscroll;
	needToRefresh = true;
}
void CThumbnailVertical::RenderIconeWithVScroll(wxDC* deviceContext)
{
	if (nbElementInIconeList == 0)
		return;

	int iconeWidth = themeThumbnail.themeIcone.GetWidth();
	int iconeHeight = themeThumbnail.themeIcone.GetHeight();
	int windowWidth = GetWindowWidth();
	int windowHeight = GetWindowHeight();

	// Nombre de colonnes par ligne
	int nbElementByRow = windowWidth / iconeWidth;
	if ((nbElementByRow * iconeWidth) < windowWidth)
		nbElementByRow++;

	if (nbElementByRow <= 0) return;

	// 1. Calcul mathématique O(1) des lignes visibles
	int firstVisibleRow = posHauteur / iconeHeight;
	int lastVisibleRow = (posHauteur + windowHeight) / iconeHeight;

	// Convertir les lignes en index d'éléments de la liste
	int firstVisibleIdx = firstVisibleRow * nbElementByRow;
	int lastVisibleIdx = ((lastVisibleRow + 1) * nbElementByRow) - 1;

	// Sécurisation des index
	if (firstVisibleIdx < 0) firstVisibleIdx = 0;
	if (lastVisibleIdx >= nbElementInIconeList) lastVisibleIdx = nbElementInIconeList - 1;

	// Éviter de réallouer les variables de dimension dans la boucle
	int realWidth = themeThumbnail.themeIcone.GetRealWidth();
	int realHeight = themeThumbnail.themeIcone.GetRealHeight();

	// 2. La boucle n'évalue QUE les vignettes présentes dans le rectangle de l'écran
	for (int i = firstVisibleIdx; i <= lastVisibleIdx; i++)
	{
		CIcone* pBitmapIcone = iconeList->GetElement(i);
		if (pBitmapIcone != nullptr)
		{
			// Calcul de la position absolue de l'icône sur la toile virtuelle
			int row = i / nbElementByRow;
			int col = i % nbElementByRow;
			int absX = col * iconeWidth;
			int absY = row * iconeHeight;

			pBitmapIcone->SetTheme(themeThumbnail.themeIcone);
			pBitmapIcone->SetPos(absX, absY);
			pBitmapIcone->SetWindowPos(-posLargeur, -posHauteur);
			pBitmapIcone->SetSizeIcone(realWidth, realHeight);
			pBitmapIcone->SetVisibility(true);

			if (numActifPhotoId != -1 && pBitmapIcone->GetNumElement() == 0)
			{
				numActifPhotoId = iconeList->GetPhotoId(i);
				pBitmapIcone->SetActive(true);
			}

			RenderBitmap(deviceContext, pBitmapIcone, 0, 0);
		}
	}
}

void CThumbnailVertical::RenderIconeWithoutVScroll(wxDC* deviceContext)
{
	if (nbElementInIconeList == 0)
		return;

	int iconeWidth = themeThumbnail.themeIcone.GetWidth();
	int windowWidth = GetWindowWidth();

	int firstVisibleIdx = posLargeur / iconeWidth;
	int lastVisibleIdx = (posLargeur + windowWidth) / iconeWidth;

	if (firstVisibleIdx < 0) firstVisibleIdx = 0;
	if (lastVisibleIdx >= nbElementInIconeList) lastVisibleIdx = nbElementInIconeList - 1;

	for (int i = firstVisibleIdx; i <= lastVisibleIdx; i++)
	{
		CIcone* pBitmapIcone = iconeList->GetElement(i);
		if (pBitmapIcone != nullptr)
		{
			pBitmapIcone->SetTheme(themeThumbnail.themeIcone);
			pBitmapIcone->SetPos(i * iconeWidth, 0);
			pBitmapIcone->SetWindowPos(-posLargeur, 0);
			pBitmapIcone->SetVisibility(true);

			RenderBitmap(deviceContext, pBitmapIcone, 0, 0);
		}
	}
}

void CThumbnailVertical::RenderIcone(wxDC* deviceContext)
{
	//ResizeThumbnail();
	if (noVscroll)
		RenderIconeWithoutVScroll(deviceContext);
	else
		RenderIconeWithVScroll(deviceContext);

	//scrollbar->Refresh();
}

void CThumbnailVertical::UpdateScrollWithVScroll()
{
	int oldthumbnailSizeX = thumbnailSizeX;
	int oldthumbnailSizeY = thumbnailSizeY;

	thumbnailSizeX = 0;
	thumbnailSizeY = 0;

	//bool update = false;
	int nbElement = nbElementInIconeList;

	if (nbElement > 0)
	{
		int nbElementByRow = (GetWindowWidth()) / themeThumbnail.themeIcone.GetWidth();
		if ((nbElementByRow * themeThumbnail.themeIcone.GetWidth()) < (GetWindowWidth()))
			nbElementByRow++;

		int nbElementEnY = nbElementInIconeList / nbElementByRow;
		if (nbElementEnY * nbElementByRow < nbElementInIconeList)
			nbElementEnY++;

		if (nbElement < nbElementByRow)
			nbElementByRow = nbElement;

		thumbnailSizeX = nbElementByRow * themeThumbnail.themeIcone.GetWidth();
		thumbnailSizeY = nbElementEnY * themeThumbnail.themeIcone.GetHeight();
	}

	//printf("CThumbnailVertical::UpdateScrollWithVScroll old %d %d \n", oldthumbnailSizeX, oldthumbnailSizeY);
	//printf("CThumbnailVertical::UpdateScrollWithVScroll new %d %d \n", thumbnailSizeX, thumbnailSizeY);

	//bool refresh = false;
	if (nbElementInIconeList >= 0)
	{
		//int oldLargeur = posLargeur;
		//int oldHauteur = posHauteur;

		float xRatio = 1.0;
		float yRatio = 1.0;

		if (oldthumbnailSizeX != 0)
			xRatio = static_cast<float>(thumbnailSizeX) / static_cast<float>(oldthumbnailSizeX);

		if (oldthumbnailSizeY != 0)
			yRatio = static_cast<float>(thumbnailSizeY) / static_cast<float>(oldthumbnailSizeY);

		float posX = static_cast<float>(posLargeur) * xRatio;
		float posY = static_cast<float>(posHauteur) * yRatio;

		wxWindow* parent = this->GetParent();

		if (parent != nullptr)
		{
			auto controlSize = new CControlSize();
			wxCommandEvent evt(wxEVENT_SETCONTROLSIZE);
			controlSize->controlWidth = thumbnailSizeX;
			controlSize->controlHeight = thumbnailSizeY;
			evt.SetClientData(controlSize);
			parent->GetEventHandler()->AddPendingEvent(evt);
		}

		if (parent != nullptr)
		{
			auto size = new wxSize();
			wxCommandEvent evt(wxEVENT_SETPOSITION);
			size->x = static_cast<int>(posX);
			size->y = static_cast<int>(posY);
			evt.SetClientData(size);
			parent->GetEventHandler()->AddPendingEvent(evt);
		}

		posLargeur = posX;
		posHauteur = posY;
	}
}

void CThumbnailVertical::UpdateScrollWithoutVScroll()
{
	int nbElement = nbElementInIconeList;
	if (nbElement > 0)
	{
		nbLigneY = 1;
		nbLigneX = nbElement;
		thumbnailSizeX = nbLigneX * themeThumbnail.themeIcone.GetWidth();
		thumbnailSizeY = themeThumbnail.themeIcone.GetHeight();

		wxWindow* parent = this->GetParent();

		if (parent != nullptr)
		{
			auto controlSize = new CControlSize();
			wxCommandEvent evt(wxEVENT_SETCONTROLSIZE);
			controlSize->controlWidth = thumbnailSizeX;
			controlSize->controlHeight = thumbnailSizeY;
			evt.SetClientData(controlSize);
			parent->GetEventHandler()->AddPendingEvent(evt);
		}

		if (parent != nullptr)
		{
			auto size = new wxSize();
			wxCommandEvent evt(wxEVENT_SETPOSITION);
			size->x = posLargeur;
			size->y = posHauteur;
			evt.SetClientData(size);
			parent->GetEventHandler()->AddPendingEvent(evt);
		}

		//UpdateScrollBar(update);
	}
}

void CThumbnailVertical::UpdateScroll()
{
	if (GetWindowWidth() <= 0)
		return;

	//printf("CThumbnailVertical::UpdateScroll \n");
	if (noVscroll)
	{
		UpdateScrollWithoutVScroll();
	}
	else
	{
		UpdateScrollWithVScroll();
	}
}
