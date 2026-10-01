#pragma once

#include <vector>
#include "Draw.h"
#include <wx/colour.h>

namespace Regards::FiltreEffet {

    struct STextStyleDraw {
        wxPoint point;
        wxString text;
        wxColour color;
        wxString fontName;
        int fontSize;
        int fontIndex;
        bool isBold;
        bool isItalic;
        int opacity;
    };

    class CTextDraw : public CDraw {
    public:
        CTextDraw();
        ~CTextDraw() override = default;

        bool RefreshAfterKeyDown() override
        {
            return true;
        }

        void InitPoint(CEffectParameter* effect, const long& m_lx, const long& m_ly, const long& m_lHScroll, const long& m_lVScroll, const float& ratio) override;
        void MouseMove(const long& xNewSize, const long& yNewSize, const long& m_lHScroll, const long& m_lVScroll, const float& ratio) override;
        void DessinerSurMat(cv::Mat& matrix, const long& hScroll, const long& vScroll, const float& ratio) override;
        void MouseDown(CEffectParameter* effect) override;
        void MouseUp() override;
        void GetPoint(wxPoint& pt) override;
        void Reset();
        void Dessiner(wxDC* deviceContext, const long& hScroll, const long& vScroll, const float& ratio, const wxColour& rgb, const wxColour& rgbFirst, const wxColour& rgbSecond, const int32_t& style);
        const std::vector<STextStyleDraw>& GetTousLesTextes() const { return m_tousLesTextes; }
        // Interception des touches clavier
        void KeyDown(const int32_t& keyCode) override;

        // Configuration dynamique de la boîte à outils active
        void SetCurrentTextParams(int size, const wxString& fontName, int fontIndex, bool bold, bool italic, int opacity, const wxColour& color) {
            m_currentFontSize = size;
            m_currentFontName = fontName;
            m_currentFontIndex = fontIndex;
            m_currentIsBold = bold;
            m_currentIsItalic = italic;
            m_currentOpacity = opacity;
            m_currentColor = color;
        }
    private:
        std::vector<STextStyleDraw> m_tousLesTextes;

        bool m_isDrawing = false;

        // Variables pour stocker la configuration de l'outil courant
        int m_currentFontSize = 24;
        wxString m_currentFontName = "Arial";
        int m_currentFontIndex = 0;
        bool m_currentIsBold = false;
        bool m_currentIsItalic = false;
        int m_currentOpacity = 255;
        wxColour m_currentColor = *wxBLACK;


    };
}
