// ReSharper disable All
#include <header.h>
#include "InfosSeparationBarFace.h"
#include <WindowMain.h>
#include <ChangeLabel.h>
#include <ConvertUtility.h>
#include <LibResource.h>
#include <SqlFaceLabel.h>
using namespace Regards::Window;
using namespace Regards::Sqlite;

CInfosSeparationBarFace::~CInfosSeparationBarFace(void)
{
}

bool CInfosSeparationBarFace::GetSelected()
{
	return isSelectIcone;
}

/**
 * \brief 
 * \param theme 
 */
CInfosSeparationBarFace::CInfosSeparationBarFace(const CThemeInfosSeparationBar& theme)
	: CInfosSeparationBar(theme), theme_(theme)
{
	parentWindow = nullptr;
	isSelected = false;
	isSelectIcone = false;
	libelleSelectAll = CLibResource::LoadStringFromResource(L"LBLSELECTABLE", 1);
	libelleSelectIcone = CLibResource::LoadStringFromResource(L"LBLSelectAll", 1);
	libelleDelete = "Delete Face";
}

bool CInfosSeparationBarFace::TestIfEnable()
{
	if (!enableModification)
	{
		wxMessageBox("Face detection is working. Please wait", "Informations");
		return false;
	}
	return true;
}


int CInfosSeparationBarFace::GetNumFace()
{
	return this->numFace;
}

void CInfosSeparationBarFace::SetParentWindow(wxWindow* parentWindow)
{
	this->parentWindow = parentWindow;
}

void CInfosSeparationBarFace::SetNumFace(const CFaceName& faceName)
{
	this->numFace = faceName.numFace;
	this->isSelected = faceName.isSelectable;
}

void CInfosSeparationBarFace::EnableModification(const bool& enable)
{
	enableModification = enable;
}

void CInfosSeparationBarFace::OnClick(const int& x, const int& y)
{
	bool updateInfos = false;
	if (!TestIfEnable())
		return;

	if ((rcSelect.x < x && x < (rcSelect.x + rcSelect.width)) && ((rcSelect.y) < y && y < (rcSelect.y + rcSelect.
		height)))
	{
		isSelected = !isSelected;

		//Mise à jour de la base de données
		CSqlFaceLabel sqlFaceLabel;
		sqlFaceLabel.UpdateFaceLabel(numFace, isSelected);

		updateInfos = true;
	}
	else if ((rcSelectIcone.x < x && x < (rcSelectIcone.x + rcSelectIcone.width)) && ((rcSelectIcone.y) < y && y < (
		rcSelectIcone.y + rcSelectIcone.height)))
	{
		isSelectIcone = !isSelectIcone;
	}
	else if ((rcDeleteIcone.x < x && x < (rcDeleteIcone.x + rcDeleteIcone.width)) && ((rcDeleteIcone.y) < y && y < (
		rcDeleteIcone.y + rcDeleteIcone.height)))
	{
		auto windowMain = static_cast<CWindowMain*>(parentWindow->FindWindowById(MAINVIEWERWINDOWID));
		if (windowMain != nullptr)
		{
			wxCommandEvent evt(wxEVENT_DELETEFACE);
			evt.SetInt(numFace);
			windowMain->GetEventHandler()->AddPendingEvent(evt);
		}
	}
	else if (bitmapEdit.IsOk() && ((xPosEdit < x && x < (xPosEdit + bitmapEdit.GetWidth())) && (yPosEdit < y && y < (yPosEdit + bitmapEdit.
		GetHeight()))))
	{
		//Modification du titre
		ChangeLabel changeLabel(parentWindow);
		changeLabel.SetLabel(title);
		changeLabel.ShowModal();
		if (changeLabel.IsOk())
		{
			wxString newLabel = changeLabel.GetNewLabel();
			CSqlFaceLabel sqlFaceLabel;
			sqlFaceLabel.UpdateFaceLabel(numFace, newLabel);
			title = newLabel;
			updateInfos = true;
		}
	}


	if (updateInfos)
	{
		auto windowMain = static_cast<CWindowMain*>(parentWindow->FindWindowById(MAINVIEWERWINDOWID));
		if (windowMain != nullptr)
		{
			wxCommandEvent evt(wxEVENT_FACEINFOSUPDATE);
			windowMain->GetEventHandler()->AddPendingEvent(evt);
		}
	}
}

void CInfosSeparationBarFace::RenderTitle(wxDC* dc)
{
	wxRect rc;
	rc.x = 0;
	rc.y = 0;
	rc.width = width;
	rc.height = GetHeight();
	CWindowMain::FillRect(dc, rc, theme.colorBack);

	int posY = 0;
	vector<wxString> listOfTexte = CConvertUtility::split(title, '@');

	if (!listOfTexte.empty())
	{
		// Mettre en cache la hauteur calculée pour éviter d'appeler GetHeight() en boucle
		int sizeOfY = rc.height / listOfTexte.size();

		for (const wxString& currentTitle : listOfTexte)
		{
			if (!currentTitle.IsEmpty())
			{
				wxSize size = CWindowMain::GetSizeTexte(dc, currentTitle, theme.themeFont);
				int localy = posY; // Utilisation de posY au lieu de forcer 0
				int localx = (width - size.x) / 2;

				CWindowMain::DrawTexte(dc, currentTitle, localx, localy, theme.themeFont);
				posY += sizeOfY;

				titleRectPos.x = localx;
				titleRectPos.y = localy;
				titleRectPos.width = size.x;
				titleRectPos.height = size.y;
			}
		}
	}
}

void CInfosSeparationBarFace::RenderIcone(wxDC* deviceContext, const int& posLargeur, const int& posHauteur)
{
	RenderTitle(deviceContext);
	int x = 0;
	int y = 0;
	int marge_text = 5;
	int marge_sepa = 20;

	// OPTIMISATION CRITIQUE : Chargement paresseux (Lazy Loading) des ressources vectorielles SVG.
	// On n'exécute le décodage et la désactivation de l'image qu'UNE seule fois par session de barre.
	if (!bitmapCheckOn.IsOk())
	{
		bitmapCheckOn = CLibResource::CreatePictureFromSVG("IDB_CHECKBOX_ON", theme.GetCheckboxWidth(), theme.GetCheckboxHeight());
		bitmapCheckOn = bitmapCheckOn.ConvertToDisabled();
	}

	if (!bitmapCheckOff.IsOk())
	{
		bitmapCheckOff = CLibResource::CreatePictureFromSVG("IDB_CHECKBOX_OFF", theme.GetCheckboxWidth(), theme.GetCheckboxHeight());
		bitmapCheckOff = bitmapCheckOff.ConvertToDisabled();
	}

	if (!bitmapDelete.IsOk())
	{
		bitmapDelete = CLibResource::CreatePictureFromSVG("IDB_DELETE", theme.GetCheckboxWidth(), theme.GetCheckboxHeight());
		bitmapDelete = bitmapDelete.ConvertToDisabled();
	}

	if (!bitmapEdit.IsOk())
	{
		bitmapEdit = CLibResource::CreatePictureFromSVG("IDB_EDIT_LABEL", theme.GetCheckboxWidth(), theme.GetCheckboxHeight());
		bitmapEdit = bitmapEdit.ConvertToDisabled();
	}

	if (bitmapEdit.IsOk())
		deviceContext->DrawBitmap(bitmapEdit, titleRectPos.x + titleRectPos.width + 5, titleRectPos.y);

	xPosEdit = _xPos + posLargeur + titleRectPos.x + titleRectPos.width + marge_text;
	yPosEdit = _yPos + posHauteur + titleRectPos.y;

	int xPos = x;
	int yPos = y + (theme.GetHeight() - bitmapCheckOn.GetHeight());

	rcSelect.x = _xPos + xPos + posLargeur;
	rcSelect.y = _yPos + yPos + posHauteur;
	rcSelect.width = bitmapCheckOn.GetWidth();
	rcSelect.height = bitmapCheckOn.GetHeight();

	if (isSelected && bitmapCheckOn.IsOk())
		deviceContext->DrawBitmap(bitmapCheckOn, xPos, yPos);
	else if (bitmapCheckOff.IsOk())
		deviceContext->DrawBitmap(bitmapCheckOff, xPos, yPos);

	// Calculer la taille du texte une première fois
	wxSize size = CWindowMain::GetSizeTexte(deviceContext, libelleSelectAll, theme.themeFont);

	xPos = xPos + marge_text + bitmapCheckOn.GetWidth();
	yPos = y + (theme.GetHeight() - size.y) - (bitmapCheckOn.GetHeight() - size.y) / 2;

	CWindowMain::DrawTexte(deviceContext, libelleSelectAll, xPos, yPos, theme.themeFont);

	xPos += size.x + marge_sepa;
	yPos = y + (theme.GetHeight() - bitmapCheckOn.GetHeight());

	if (isSelectIcone && bitmapCheckOn.IsOk())
		deviceContext->DrawBitmap(bitmapCheckOn, xPos, yPos);
	else if (bitmapCheckOff.IsOk())
		deviceContext->DrawBitmap(bitmapCheckOff, xPos, yPos);

	rcSelectIcone.x = _xPos + xPos + posLargeur;
	rcSelectIcone.y = _yPos + yPos + posHauteur;
	rcSelectIcone.width = bitmapCheckOn.GetWidth();
	rcSelectIcone.height = bitmapCheckOn.GetHeight();

	// OPTIMISATION : Suppression du second GetSizeTexte redondant pour libelleSelectAll car 'size' possède déjà les bonnes valeurs
	xPos = xPos + marge_text + bitmapCheckOn.GetWidth();
	yPos = y + (theme.GetHeight() - size.y) - (bitmapCheckOn.GetHeight() - size.y) / 2;

	CWindowMain::DrawTexte(deviceContext, libelleSelectIcone, xPos, yPos, theme.themeFont);

	// Mesurer la taille pour libelleSelectIcone qui est différent afin de décaler correctement le bouton delete
	wxSize sizeIcone = CWindowMain::GetSizeTexte(deviceContext, libelleSelectIcone, theme.themeFont);
	xPos += sizeIcone.x + marge_sepa;
	yPos = y + (theme.GetHeight() - bitmapDelete.GetHeight());

	if (bitmapDelete.IsOk())
		deviceContext->DrawBitmap(bitmapDelete, xPos, yPos);

	rcDeleteIcone.x = _xPos + xPos + posLargeur;
	rcDeleteIcone.y = _yPos + yPos + posHauteur;
	rcDeleteIcone.width = bitmapDelete.GetWidth();
	rcDeleteIcone.height = bitmapDelete.GetHeight();

	sizeDelete = CWindowMain::GetSizeTexte(deviceContext, libelleDelete, theme.themeFont);

	xPos = xPos + marge_text + bitmapDelete.GetWidth();
	yPos = y + (theme.GetHeight() - sizeDelete.y) - (bitmapDelete.GetHeight() - sizeDelete.y) / 2;

	CWindowMain::DrawTexte(deviceContext, libelleDelete, xPos, yPos, theme.themeFont);
}
