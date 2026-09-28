#pragma once

#include <vector>
#include "Draw.h"

namespace Regards::FiltreEffet {

    // Structure locale pour stocker le style d'affichage à l'écran
    struct SLineStyleDraw {
        std::vector<wxPoint> points;
        wxColour color;
        int penSize;
        int typeBrush;
        int opacity; // <-- Stockage à l'écran
    };


    class CPenDraw : public CDraw {
    public:
        CPenDraw();
        ~CPenDraw() override = default;

        void InitPoint(CEffectParameter* effect, const long& m_lx, const long& m_ly, const long& m_lHScroll,
            const long& m_lVScroll, const float& ratio) override;

        void MouseMove(const long& xNewSize, const long& yNewSize,
            const long& m_lHScroll, const long& m_lVScroll,
            const float& ratio) override;

        void Dessiner(wxDC* deviceContext, const long& hScroll, const long& vScroll, const float& ratio,
            const wxColour& rgb, const wxColour& rgbFirst, const wxColour& rgbSecond, const int32_t& style) override;

        void DessinerSurMat(cv::Mat& matrix, const long& hScroll, const long& vScroll, const float& ratio);

        void MouseDown(CEffectParameter* effect) override;
        void MouseUp() override;

        void Reset();
        void GetPoint(wxPoint& pt) override;

        void SetTransparenceValue(wxImage& drawingImage);

        const std::vector<SLineStyleDraw>& GetTousLesTraces() const {
            return m_tousLesTraces;
        }

        // Dans PenDraw.h, section public :
        void SetCurrentBrushType(int type) { m_currentBrush = type; }

        void SetCurrentOpacity(int type) { m_currentOpacity = type; }

    private:

        cv::Scalar ConvertScalarToWxColour(const cv::Scalar& scalar_color, int alpha);
        std::vector<SLineStyleDraw> m_tousLesTraces;
        bool m_isDrawing = false;
        // Dans PenDraw.h, section private :
        int m_currentBrush = 0;
        int m_currentOpacity = 0;
        // Variables temporaires pour stocker le style du tracé actuel
        wxColour m_currentColor;
        int m_currentSize = 4;
    };
} // namespace Regards::FiltreEffet
