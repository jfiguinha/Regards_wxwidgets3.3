#include <header.h>
#include "ThumbnailVertical.h"
#include "ScrollbarWnd.h"
#include "ThumbnailBuffer.h"
#include <RegardsConfigParam.h>
#include <wx/progdlg.h>
#include <algorithm> // Pour std::max

using namespace Regards::Control;

CThumbnailVertical::CThumbnailVertical(wxWindow* parent, const wxWindowID id, const CThemeThumbnail& themeThumbnail, const bool& testValidity)
	: CThumbnail(parent, id, themeThumbnail, testValidity), test_validity_(testValidity),
	theme_thumbnail_(themeThumbnail), id_(id)
{
	noVscroll = false;
}

CThumbnailVertical::~CThumbnailVertical(void) = default;

void CThumbnailVertical::GenerateList(CIconeList*& newIconeList)
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
				size--; // Ajustement dynamique pour éviter les débordements
			}
			else
			{
				newIconeList->AddElement(ico);
			}
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

CIcone* CThumbnailVertical::FindElementWithVScroll(const int& xPos, const int& yPos)
{
	int x = posLargeur + xPos;
	int y = posHauteur + yPos;

	// Sécurisation stricte contre la division par zéro
	int iconeWidth = std::max(1, themeThumbnail.themeIcone.GetWidth());
	int iconeHeight = std::max(1, themeThumbnail.themeIcone.GetHeight());

	int nbElementByX = thumbnailSizeX / iconeWidth;
	if (nbElementByX <= 0) return nullptr;

	int numY = y / iconeHeight;
	int numX = x / iconeWidth;
	int numElement = numY * nbElementByX + numX;

	if (numElement >= nbElementInIconeList || numElement < 0)
		return nullptr;

	return iconeList->GetElement(numElement);
}

CIcone* CThumbnailVertical::FindElementWithoutVScroll(const int& xPos, const int& yPos)
{
	int x = posLargeur + xPos;
	if (x > thumbnailSizeX)
		return nullptr;

	int iconeWidth = std::max(1, themeThumbnail.themeIcone.GetWidth());
	int numElement = x / iconeWidth;

	if (numElement >= nbElementInIconeList || numElement < 0)
		return nullptr;

	return iconeList->GetElement(numElement);
}

CIcone* CThumbnailVertical::FindElement(const int& xPos, const int& yPos)
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

	// Sécurisation des dimensions géométriques fondamentales
	int iconeWidth = std::max(1, themeThumbnail.themeIcone.GetWidth());
	int iconeHeight = std::max(1, themeThumbnail.themeIcone.GetHeight());
	int windowWidth = GetWindowWidth();
	int windowHeight = GetWindowHeight();

	int nbElementByRow = windowWidth / iconeWidth;
	if ((nbElementByRow * iconeWidth) < windowWidth)
		nbElementByRow++;

	if (nbElementByRow <= 0)
		return;

	int firstVisibleRow = posHauteur / iconeHeight;
	int lastVisibleRow = (posHauteur + windowHeight) / iconeHeight;

	int firstVisibleIdx = firstVisibleRow * nbElementByRow;
	int lastVisibleIdx = ((lastVisibleRow + 1) * nbElementByRow) - 1;

	if (firstVisibleIdx < 0) firstVisibleIdx = 0;
	if (lastVisibleIdx >= nbElementInIconeList) lastVisibleIdx = nbElementInIconeList - 1;

	int realWidth = themeThumbnail.themeIcone.GetRealWidth();
	int realHeight = themeThumbnail.themeIcone.GetRealHeight();

	for (int i = firstVisibleIdx; i <= lastVisibleIdx; i++)
	{
		CIcone* pBitmapIcone = iconeList->GetElement(i);
		if (pBitmapIcone != nullptr)
		{
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

	int iconeWidth = std::max(1, themeThumbnail.themeIcone.GetWidth());
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
	if (noVscroll)
		RenderIconeWithoutVScroll(deviceContext);
	else
		RenderIconeWithVScroll(deviceContext);
}

void CThumbnailVertical::UpdateScrollWithVScroll()
{
	int oldthumbnailSizeX = thumbnailSizeX;
	int oldthumbnailSizeY = thumbnailSizeY;

	thumbnailSizeX = 0;
	thumbnailSizeY = 0;

	int nbElement = nbElementInIconeList;
	int iconeWidth = std::max(1, themeThumbnail.themeIcone.GetWidth());
	int iconeHeight = std::max(1, themeThumbnail.themeIcone.GetHeight());

	if (nbElement > 0)
	{
		int nbElementByRow = (GetWindowWidth()) / iconeWidth;
		if ((nbElementByRow * iconeWidth) < (GetWindowWidth()))
			nbElementByRow++;

		if (nbElementByRow > 0)
		{
			int nbElementEnY = nbElementInIconeList / nbElementByRow;
			if (nbElementEnY * nbElementByRow < nbElementInIconeList)
				nbElementEnY++;

			if (nbElement < nbElementByRow)
				nbElementByRow = nbElement;

			thumbnailSizeX = nbElementByRow * iconeWidth;
			thumbnailSizeY = nbElementEnY * iconeHeight;
		}
	}

	if (nbElementInIconeList >= 0)
	{
		float xRatio = 1.0f;
		float yRatio = 1.0f;

		if (oldthumbnailSizeX != 0)
			xRatio = static_cast<float>(thumbnailSizeX) / static_cast<float>(oldthumbnailSizeX);

		if (oldthumbnailSizeY != 0)
			yRatio = static_cast<float>(thumbnailSizeY) / static_cast<float>(oldthumbnailSizeY);

		float posX = static_cast<float>(posLargeur) * xRatio;
		float posY = static_cast<float>(posHauteur) * yRatio;

		wxWindow* parent = this->GetParent();
		if (parent != nullptr)
		{
			// Utilisation de std::unique_ptr en garde locale pour éviter la fuite en cas d'interruption
			auto controlSize = std::make_unique<CControlSize>();
			controlSize->controlWidth = thumbnailSizeX;
			controlSize->controlHeight = thumbnailSizeY;

			wxCommandEvent evt(wxEVENT_SETCONTROLSIZE);
			evt.SetClientData(controlSize.release()); // Le destinataire prend la responsabilité mémoire
			parent->GetEventHandler()->AddPendingEvent(evt);

			auto size = std::make_unique<wxSize>(static_cast<int>(posX), static_cast<int>(posY));
			wxCommandEvent evtPos(wxEVENT_SETPOSITION);
			evtPos.SetClientData(size.release());
			parent->GetEventHandler()->AddPendingEvent(evtPos);
		}

		posLargeur = posX;
		posHauteur = posY;
	}
}

void CThumbnailVertical::UpdateScrollWithoutVScroll()
{
	int nbElement = nbElementInIconeList;
	int iconeWidth = std::max(1, themeThumbnail.themeIcone.GetWidth());
	int iconeHeight = std::max(1, themeThumbnail.themeIcone.GetHeight());

	if (nbElement > 0)
	{
		nbLigneY = 1;
		nbLigneX = nbElement;
		thumbnailSizeX = nbLigneX * iconeWidth;
		thumbnailSizeY = iconeHeight;

		wxWindow* parent = this->GetParent();
		if (parent != nullptr)
		{
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
}

void CThumbnailVertical::UpdateScroll()
{
	if (GetWindowWidth() <= 0)
		return;

	if (noVscroll)
		UpdateScrollWithoutVScroll();
	else
		UpdateScrollWithVScroll();
}
