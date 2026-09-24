#include <header.h>
#include "ThumbnailHorizontal.h"
#include "ScrollbarWnd.h"
using namespace Regards::Control;


CThumbnailHorizontal::CThumbnailHorizontal(wxWindow* parent, wxWindowID id, const CThemeThumbnail& themeThumbnail,
                                           const bool& testValidity)
	: CThumbnail(parent, id, themeThumbnail, testValidity)
{
}

CThumbnailHorizontal::~CThumbnailHorizontal(void)
{
}

void CThumbnailHorizontal::InitPosition()
{
	wxWindow* parent = this->GetParent();
	if (parent != nullptr)
	{
		auto size = new wxSize();
		wxCommandEvent evt(wxEVENT_SETPOSITION);
		size->x = posLargeur;
		size->y = posHauteur;
		evt.SetClientData(size);
		parent->GetEventHandler()->AddPendingEvent(evt);
	}

	posHauteur = 0;
	posLargeur = 0;
}

void CThumbnailHorizontal::RenderIcone(wxDC* deviceContext)
{
	if (nbElementInIconeList == 0)
		return;

	int iconeWidth = themeThumbnail.themeIcone.GetWidth();
	int windowWidth = GetWindowWidth();

	// 1. Calcul mathématique O(1) de l'intervalle des icônes visibles à l'écran
	int firstVisibleIdx = posLargeur / iconeWidth;
	int lastVisibleIdx = (posLargeur + windowWidth) / iconeWidth;

	// Sécurisation des index pour ne pas déborder du tableau
	if (firstVisibleIdx < 0) firstVisibleIdx = 0;
	if (lastVisibleIdx >= nbElementInIconeList) lastVisibleIdx = nbElementInIconeList - 1;

	// 2. La boucle s'exécute UNIQUEMENT sur les éléments visibles
	for (int i = firstVisibleIdx; i <= lastVisibleIdx; i++)
	{
		CIcone* pBitmapIcone = iconeList->GetElement(i);
		if (pBitmapIcone != nullptr)
		{
			// Position absolue sur la toile virtuelle
			int absX = i * iconeWidth;
			int absY = 0;

			// Coordonnées relatives à l'écran (pour RenderIcone interne)
			pBitmapIcone->SetWindowPos(-posLargeur, 0);
			pBitmapIcone->SetPos(absX, absY);
			pBitmapIcone->SetVisibility(true);
			pBitmapIcone->SetTheme(themeThumbnail.themeIcone);

			RenderBitmap(deviceContext, pBitmapIcone, 0, 0);
		}
	}
}



void CThumbnailHorizontal::UpdateScroll()
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
	}
}


CIcone * CThumbnailHorizontal::FindElement(const int& xPos, const int& yPos)
{
	int x = posLargeur + xPos;
	if (x > thumbnailSizeX)
		return nullptr;

	int numElement = x / themeThumbnail.themeIcone.GetWidth();

	if (numElement >= nbElementInIconeList)
		return nullptr;

	return iconeList->GetElement(numElement);
}
