#pragma once
#include <vector>
#include "Draw.h"

namespace Regards::FiltreEffet {

    struct SShapeStyleDraw {
        wxPoint startPoint;
        wxPoint endPoint;
        int shapeType;
        wxColour color;
        int penSize;
        int typeBrush;
        int opacity;
        bool isFilled;
    };

    class CRectangleDraw : public CDraw {
    public:
        CRectangleDraw();
        ~CRectangleDraw() override = default;

        void InitPoint(CEffectParameter* effect, const long& m_lx, const long& m_ly, const long& m_lHScroll, const long& m_lVScroll, const float& ratio) override;
        void MouseMove(const long& xNewSize, const long& yNewSize, const long& m_lHScroll, const long& m_lVScroll, const float& ratio) override;
        void Dessiner(wxDC* deviceContext, const long& hScroll, const long& vScroll, const float& ratio, const wxColour& rgb, const wxColour& rgbFirst, const wxColour& rgbSecond, const int32_t& style) override;
        void DessinerSurMat(cv::Mat& matrix, const long& hScroll, const long& vScroll, const float& ratio);
        void MouseDown(CEffectParameter* effect) override;
        void MouseUp() override;
        void Reset();
        void GetPoint(wxPoint& pt) override;

        const std::vector<SShapeStyleDraw>& GetTousLesTraces() const { return m_tousLesTraces; }
        void SetCurrentShapeParams(int shapeType, int brushType, int opacity, bool isFilled) {
            m_currentShapeType = shapeType;
            m_currentBrush = brushType;
            m_currentOpacity = opacity;
            m_currentIsFilled = isFilled;
        }

    private:

        void DrawShapeOnMat(cv::Mat& matrix, SShapeStyleDraw& shape, const long& hScroll, const long& vScroll, const float& ratio);
        std::vector<SShapeStyleDraw> m_tousLesTraces;
        bool m_isDrawing = false;
        int m_currentShapeType = 0;
        int m_currentBrush = 0;
        int m_currentOpacity = 255;
        bool m_currentIsFilled = false;
    };
}
