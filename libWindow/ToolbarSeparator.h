#pragma once
#include <ToolbarElement.h>

namespace Regards::Window
{
    class CToolbarSeparator : public CToolbarElement
    {
    public:
        CToolbarSeparator()
        {
            this->SetVisible(true);
        }
        ~CToolbarSeparator() = default;

        int GetWidth() override { return width; }  // Épaisseur de la ligne sur l'image
        int GetHeight() override { return height; } // Hauteur de la ligne verticale

        bool FindElement(const int& x, const int& y)  { return false; } // Pas d'interaction au clic
        bool MouseOver(wxDC* deviceContext, const int& x, const int& y) override { return false; }

        void DrawButton(wxDC* context, const int& x, const int& y) override
        {
            // Dessine la ligne fine grise (couleur subtile de séparation sur fond sombre)
            context->SetPen(wxPen(wxColour(60, 68, 85), 1, wxPENSTYLE_SOLID));
            context->DrawLine(x, y + 4, x, y + GetHeight() - 4);
            context->SetPen(wxNullPen);
        }

        void Resize(const int& tailleX, const int& tailleY) override
        {
            width = tailleX;
			height = tailleY;
        }
    private:
		int width = 1;  // Épaisseur de la ligne sur l'image
		int height = 24; // Hauteur de la ligne verticale
    };
}
