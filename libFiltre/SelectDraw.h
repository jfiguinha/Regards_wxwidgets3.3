#pragma once
#include <vector>
#include "Draw.h"

namespace Regards::FiltreEffet {

#define SELECT_RECTANGLE 0
#define SELECT_ELLIPSE   1
#define SELECT_LASSO     2

    struct SSelectionStyleDraw {
        wxPoint startPoint;
        wxPoint endPoint;
        std::vector<wxPoint> points;
        int selectType;
        wxColour color;
        int penSize;
        int opacity;
        bool isFilled;
    };

    class CSelectDraw : public CDraw {
    public:
        CSelectDraw();
        ~CSelectDraw() override = default;

        void InitPoint(CEffectParameter* effect, const long& m_lx, const long& m_ly, const long& m_lHScroll, const long& m_lVScroll, const float& ratio) override;
        void MouseMove(const long& xNewSize, const long& yNewSize, const long& m_lHScroll, const long& m_lVScroll, const float& ratio) override;
        void Dessiner(wxDC* deviceContext, const long& hScroll, const long& vScroll, const float& ratio, const wxColour& rgb, const wxColour& rgbFirst, const wxColour& rgbSecond, const int32_t& style) override;
        void DessinerSurMat(cv::Mat& matrix, const long& hScroll, const long& vScroll, const float& ratio);
        void MouseDown(CEffectParameter* effect) override;
        void MouseUp() override;
        void Reset();
        void GetPoint(wxPoint& pt) override;

        const std::vector<SSelectionStyleDraw>& GetTousLesTraces() const { return m_tousLesTraces; }

        // Gère uniquement le type, le reste est fixé en dur
        void SetCurrentSelectParams(int selectType) {
            m_currentSelectType = selectType;
        }

    private:
        void DrawSelectionOnMat(cv::Mat& matrix, SSelectionStyleDraw& selection, const long& hScroll, const long& vScroll, const float& ratio);
        std::vector<SSelectionStyleDraw> m_tousLesTraces;
        bool m_isDrawing = false;
        int m_currentSelectType = 0;
        wxColour wx_color = wxColour(0, 0, 0);
        cv::Scalar cvColor = cv::Scalar(0, 0, 0, 0);
    };
}
