#pragma once
#include "Icone.h"

namespace Regards::Window
{
    class CLayerIcone : public CIcone
    {
    public:
        CLayerIcone(CThumbnailData* data, bool deleteData = true);
        virtual ~CLayerIcone(void);

        void SetLibelle(const wxString& libelle)
        {
            this->m_libelle = libelle;
        }

        wxString GetLibelle() const
        {
            return m_libelle;
        }

        int OnClick(int x, int y, int posLargeur, int posHauteur) override;

    private:
        // Surcharge de la méthode de rendu de la classe mère
        void RenderPictureBitmap(wxDC* memDC, wxImage& bitmapScale, const int& type);

        // Méthode de calcul de position adaptée à l'alignement horizontal et vertical
        void CalculPositionHorizontale(const wxImage& render, int& xThumbnail, int& yThumbnail);

        wxString m_libelle = "";

        // Bitmaps locales pour éviter de toucher au scope private de CIcone
        wxImage m_bitmapCheckOn;
        wxImage m_bitmapCheckOff;
    };
}
