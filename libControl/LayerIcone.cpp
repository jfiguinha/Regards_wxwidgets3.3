#include "header.h"
#include "LayerIcone.h"
#include "ThumbnailData.h"
#include "WindowMain.h"
#include "LibResource.h"
#include "FileUtility.h"
#include "theme.h"

using namespace Regards::Window;

CLayerIcone::CLayerIcone(CThumbnailData* data, bool deleteData)
    : CIcone(data, deleteData)
{
    // Forcer l'affichage de la checkbox par défaut pour la gestion des calques (visibilité)
    ShowSelectButton(true);
}

CLayerIcone::~CLayerIcone(void)
{}

void CLayerIcone::CalculPositionHorizontale(const wxImage& render, int& xThumbnail, int& yThumbnail)
{
    // Positionné horizontalement juste après la checkbox et la marge
    xThumbnail = themeIcone.GetMarge() + themeIcone.GetCheckboxWidth() + themeIcone.GetMarge();

    // Centrage vertical exact au milieu de la hauteur disponible
    if (render.IsOk() && themeIcone.GetHeight() > render.GetHeight())
    {
        yThumbnail = (themeIcone.GetHeight() - render.GetHeight()) / 2;
    }
    else
    {
        yThumbnail = themeIcone.GetMarge();
    }
}

void CLayerIcone::RenderPictureBitmap(wxDC* memDC, wxImage& bitmapScale, const int& type)
{
    wxRect rc(0, 0, themeIcone.GetWidth(), themeIcone.GetHeight());

    // 1. Dessin du fond selon l'état (Sélectionné / Inactif)
    switch (type)
    {
    case INACTIFICONE:
        if (IsChecked())
            memDC->GradientFillLinear(rc, themeIcone.colorSelectTop, themeIcone.colorSelectBottom);
        else
            memDC->GradientFillLinear(rc, themeIcone.colorBack, themeIcone.colorBack);
        break;
    case SELECTEDICONE:
    case ACTIFICONE:
        if (IsChecked())
            memDC->GradientFillLinear(rc, themeIcone.colorSelectTop, themeIcone.colorSelectBottom);
        else
            memDC->GradientFillLinear(rc, themeIcone.colorTop, themeIcone.colorBottom);
        break;
    default:
        memDC->GradientFillLinear(rc, themeIcone.colorBack, themeIcone.colorBack);
        break;
    }

    CThumbnailData* pThumbnailData = GetPtData();
    if (pThumbnailData != nullptr)
    {
        // 2. Rendu du CHECKBOX (à gauche, au milieu en hauteur)
        int yCheckbox = (themeIcone.GetHeight() - themeIcone.GetCheckboxHeight()) / 2;

        // Initialisation des bitmaps locales de statut si non présentes
        if (!m_bitmapCheckOn.IsOk())
            m_bitmapCheckOn = CLibResource::CreatePictureFromSVG("IDB_CHECKBOX_ON", themeIcone.GetCheckboxWidth(), themeIcone.GetCheckboxHeight());
        if (!m_bitmapCheckOff.IsOk())
            m_bitmapCheckOff = CLibResource::CreatePictureFromSVG("IDB_CHECKBOX_OFF", themeIcone.GetCheckboxWidth(), themeIcone.GetCheckboxHeight());

        if (IsChecked() && m_bitmapCheckOn.IsOk())
            memDC->DrawBitmap(type == INACTIFICONE ? m_bitmapCheckOn.ConvertToDisabled() : m_bitmapCheckOn, themeIcone.GetMarge(), yCheckbox);
        else if (m_bitmapCheckOff.IsOk())
            memDC->DrawBitmap(type == INACTIFICONE ? m_bitmapCheckOff.ConvertToDisabled() : m_bitmapCheckOff, themeIcone.GetMarge(), yCheckbox);

        // 3. Rendu de la MINIATURE (au milieu de la hauteur, à côté de la checkbox)
        int xThumbnail = 0, yThumbnail = 0;
        CalculPositionHorizontale(bitmapScale, xThumbnail, yThumbnail);

        if (bitmapScale.IsOk())
            memDC->DrawBitmap(bitmapScale, xThumbnail, yThumbnail);

        // 4. Détermination du libellé à afficher
        wxString txtAffichage = m_libelle;
        if (txtAffichage.IsEmpty())
        {
            // Si aucun libellé personnalisé n'a été fourni via SetLibelle, on récupère le nom du fichier par défaut
            txtAffichage = (pThumbnailData->GetTypeElement() == TYPEPHOTO)
                ? CFileUtility::GetFileName(pThumbnailData->GetFilename())
                : pThumbnailData->GetLibelle();
        }

        // 5. Rendu du LIBELLÉ (à côté de la miniature, à mi-hauteur)
        if (!txtAffichage.IsEmpty())
        {
            CThemeFont themeFont = themeIcone.font;
            wxSize sizeTexte;

            // Décalage horizontal : Checkbox + Miniature + Marges
            int xTexteDebut = xThumbnail + bitmapScale.GetWidth() + themeIcone.GetMarge();
            int espaceDisponible = themeIcone.GetWidth() - xTexteDebut - themeIcone.GetMarge();

            // Ajustement dynamique de la taille de la police si le texte est trop long
            do
            {
                sizeTexte = Regards::Window::CWindowMain::GetSizeTexte(memDC, txtAffichage, themeFont);
                if (sizeTexte.x > espaceDisponible)
                    themeFont.SetFontSize(themeFont.GetFontRealSize() - 1);
            } while (sizeTexte.x > espaceDisponible && themeFont.GetFontRealSize() > 6);

            // Centrage vertical exact du texte à mi-hauteur
            int yTexte = (themeIcone.GetHeight() - sizeTexte.y) / 2;

            Regards::Window::CWindowMain::DrawTexte(memDC, txtAffichage, xTexteDebut, yTexte, themeFont);
        }
    }
}
