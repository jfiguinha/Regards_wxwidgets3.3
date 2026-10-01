#pragma once
#include "Draw.h"
#include <vector>
#include <opencv2/core.hpp>

class CMagicHandParameter;

namespace Regards::FiltreEffet {

    class CMagicHandDraw : public CDraw {
    public:
        CMagicHandDraw();
        ~CMagicHandDraw() override = default;

        void InitPoint(CEffectParameter* effect, const long& m_lx, const long& m_ly,
            const long& m_lHScroll, const long& m_lVScroll, const float& ratio) override;

        void MouseMove(const long& xNewSize, const long& yNewSize,
            const long& m_lHScroll, const long& m_lVScroll, const float& ratio) override {}

        void DessinerSurMat(cv::Mat& matrix, const long& hScroll, const long& vScroll, const float& ratio, CMagicHandParameter* param);

        void MouseDown(CEffectParameter* effect) override;
        void MouseUp() override;
        void Reset();
        void GetPoint(wxPoint& pt) override;

    private:
        bool m_isDrawing = false;
        wxPoint m_ptReelClic = wxPoint(-1, -1);
        wxPoint m_ptLocalClic = wxPoint(-1, -1);
    };
}
