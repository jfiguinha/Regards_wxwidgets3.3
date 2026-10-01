#pragma once
#include "Draw.h"
#include <opencv2/core.hpp>

namespace Regards::FiltreEffet {

    class CPaintBucketDraw : public CDraw {
    public:
        CPaintBucketDraw() : m_ptClick(-1, -1), m_hasClicked(false) {}
        ~CPaintBucketDraw() override = default;

        void MouseDown(CEffectParameter* effect) override {
            m_hasClicked = false;
        }

        void MouseUp() override {}

        void InitPoint(CEffectParameter* effect, const long& m_lx, const long& m_ly,
            const long& m_lHScroll, const long& m_lVScroll, const float& ratio) override
        {
            float realX = XRealPosition(static_cast<float>(m_lx), m_lHScroll, ratio);
            float realY = YRealPosition(static_cast<float>(m_ly), m_lVScroll, ratio);

            m_ptClick = wxPoint(static_cast<int>(realX), static_cast<int>(realY));
            m_hasClicked = true;
        }

        void MouseMove(const long& xNewSize, const long& yNewSize,
            const long& m_lHScroll, const long& m_lVScroll, const float& ratio) override {}

        void GetPoint(wxPoint& pt) override { pt = m_ptClick; }
        void DessinerSurMat(cv::Mat& matrix, const long& hScroll, const long& vScroll, const float& ratio) override {}

        wxPoint GetClickPoint() const { return m_ptClick; }
        bool HasClicked() const { return m_hasClicked; }
        void ResetClick() { m_hasClicked = false; }

    private:
        wxPoint m_ptClick;
        bool m_hasClicked;
    };
}
