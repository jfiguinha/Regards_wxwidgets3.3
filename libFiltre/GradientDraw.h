#pragma once
#include "Draw.h"
#include <opencv2/core.hpp>

namespace Regards::FiltreEffet {

    class CGradientDraw : public CDraw {
    public:
        CGradientDraw() : m_isDrawing(false), m_ptStart(0, 0), m_ptEnd(0, 0) {}
        ~CGradientDraw() override = default;

        void MouseDown(CEffectParameter* effect) override {
            m_isDrawing = true;
            // On réinitialise les points locaux
            m_ptStart = wxPoint(0, 0);
            m_ptEnd = wxPoint(0, 0);
            isInit = false;
        }

        void MouseUp() override {
            m_isDrawing = false;
            isInit = true;
        }

        void InitPoint(CEffectParameter* effect, const long& m_lx, const long& m_ly,
            const long& m_lHScroll, const long& m_lVScroll, const float& ratio) override
        {
            m_isDrawing = true;
            float realX = XRealPosition(static_cast<float>(m_lx), m_lHScroll, ratio);
            float realY = YRealPosition(static_cast<float>(m_ly), m_lVScroll, ratio);

            m_ptStart = wxPoint(static_cast<int>(realX), static_cast<int>(realY));
            m_ptEnd = m_ptStart; // Initialement confondu
        }

        void MouseMove(const long& xNewSize, const long& yNewSize,
            const long& m_lHScroll, const long& m_lVScroll, const float& ratio) override
        {
            if (!m_isDrawing) return;

            float realX = XRealPosition(static_cast<float>(xNewSize), m_lHScroll, ratio);
            float realY = YRealPosition(static_cast<float>(yNewSize), m_lVScroll, ratio);

            m_ptEnd = wxPoint(static_cast<int>(realX), static_cast<int>(realY));
        }

        void GetPoint(wxPoint& pt) override {
            pt = m_ptEnd;
        }

        // Trace la ligne de repère dynamique entre le point de départ et la position de la souris
        void DessinerSurMat(cv::Mat& matrix, const long& hScroll, const long& vScroll, const float& ratio) override {
            if (matrix.empty() || !m_isDrawing) return;

            if (matrix.channels() == 3)
                cv::cvtColor(matrix, matrix, cv::COLOR_BGR2BGRA);

            // Conversion des positions logiques image en positions d'affichage écran
            int screenXStart = static_cast<int>(XDrawingPosition(static_cast<float>(m_ptStart.x), hScroll, ratio));
            int screenYStart = static_cast<int>(YDrawingPosition(static_cast<float>(m_ptStart.y), vScroll, ratio));
            int screenXEnd = static_cast<int>(XDrawingPosition(static_cast<float>(m_ptEnd.x), hScroll, ratio));
            int screenYEnd = static_cast<int>(YDrawingPosition(static_cast<float>(m_ptEnd.y), vScroll, ratio));

            // Tracé d'une ligne élastique blanche doublée d'une ligne noire pour le contraste
            cv::line(matrix, cv::Point(screenXStart, screenYStart), cv::Point(screenXEnd, screenYEnd), cv::Scalar(0, 0, 0, 255), 3, cv::LINE_AA);
            cv::line(matrix, cv::Point(screenXStart, screenYStart), cv::Point(screenXEnd, screenYEnd), cv::Scalar(255, 255, 255, 255), 1, cv::LINE_AA);
        }

        wxPoint GetStartPoint() const { return m_ptStart; }
        wxPoint GetEndPoint() const { return m_ptEnd; }

		bool GetIsDrawing() const { return m_isDrawing; }
		bool GetIsInit() const { return isInit; }
    private:

        bool isInit = false;
        bool m_isDrawing;
        wxPoint m_ptStart;
        wxPoint m_ptEnd;
    };
}
