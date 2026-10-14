#include <header.h>
#include "SelectDraw.h"
#include <algorithm>
#include <cmath>
#include "SelectFilterParameter.h"
#include <opencv2/imgproc.hpp>

using namespace Regards::FiltreEffet;

CSelectDraw::CSelectDraw() : m_isDrawing(false) { m_tousLesTraces.clear(); }
void CSelectDraw::Reset() { m_tousLesTraces.clear(); m_isDrawing = false; }

void CSelectDraw::MouseUp() {
    m_isDrawing = false;
    if (!m_tousLesTraces.empty() && m_tousLesTraces.back().selectType == SELECT_LASSO) {
        if (m_tousLesTraces.back().points.size() > 2) {
            m_tousLesTraces.back().endPoint = m_tousLesTraces.back().startPoint;
            m_tousLesTraces.back().points.push_back(m_tousLesTraces.back().startPoint);
        }
    }
}

void CSelectDraw::GetPoint(wxPoint& pt) {
    if (!m_tousLesTraces.empty()) pt = m_tousLesTraces.back().endPoint;
    else pt = wxPoint(0, 0);
}

void CSelectDraw::MouseDown(CEffectParameter* effect) {
    m_isDrawing = true;
    CSelectFilterParameter* param = (CSelectFilterParameter*)effect;

    m_tousLesTraces.clear();

    SSelectionStyleDraw nouvelleSelection;
    nouvelleSelection.color = wxColour(0, 0, 0);
    nouvelleSelection.penSize = 1;
    nouvelleSelection.opacity = 255;
    nouvelleSelection.isFilled = false;
    nouvelleSelection.selectType = param->selectType;
    nouvelleSelection.points.clear();

    m_tousLesTraces.push_back(nouvelleSelection);
}

void CSelectDraw::InitPoint(CEffectParameter* effect, const long& m_lx, const long& m_ly, const long& m_lHScroll, const long& m_lVScroll, const float& ratio) {
    const wxPoint point(static_cast<int>(m_lx), static_cast<int>(m_ly));
    if (!VerifierValiditerPoint(point)) return;

    MouseDown(effect);

    float realX = XRealPosition(static_cast<float>(m_lx), m_lHScroll, ratio);
    float realY = YRealPosition(static_cast<float>(m_ly), m_lVScroll, ratio);
    wxPoint ptReel(static_cast<int>(realX), static_cast<int>(realY));

    m_tousLesTraces.back().startPoint = ptReel;
    m_tousLesTraces.back().endPoint = ptReel;
    m_tousLesTraces.back().points.push_back(ptReel);
}

void CSelectDraw::MouseMove(const long& xNewSize, const long& yNewSize, const long& m_lHScroll, const long& m_lVScroll, const float& ratio) {
    if (!m_isDrawing || m_tousLesTraces.empty()) return;

    const wxPoint point(static_cast<int>(xNewSize), static_cast<int>(yNewSize));
    if (!VerifierValiditerPoint(point)) return;

    float realX = XRealPosition(static_cast<float>(xNewSize), m_lHScroll, ratio);
    float realY = YRealPosition(static_cast<float>(yNewSize), m_lVScroll, ratio);
    wxPoint ptReel(static_cast<int>(realX), static_cast<int>(realY));

    m_tousLesTraces.back().endPoint = ptReel;

    if (m_tousLesTraces.back().selectType == SELECT_LASSO) {
        if (m_tousLesTraces.back().points.empty() || m_tousLesTraces.back().points.back() != ptReel) {
            m_tousLesTraces.back().points.push_back(ptReel);
        }
    }
}

// Méthode utilitaire générique pour tracer un segment pointillé (Dotted/Dashed Line) sur un cv::Mat
void DrawDottedLine(cv::Mat& matrix, const cv::Point& pt1, const cv::Point& pt2, const cv::Scalar& color, int thickness) {
    cv::LineIterator it(matrix, pt1, pt2, 8); // Utilisation du LineIterator natif d'OpenCV
    for (int i = 0; i < it.count; i++, ++it) {
        // Condition de pointillés : dessine sur 4 pixels, saute les 4 suivants (ajustable)
        if (i % 8 < 4) {
            if (matrix.channels() == 4) {
                matrix.at<cv::Vec4b>(it.pos()) = cv::Vec4b(color[0], color[1], color[2], color[3]);
            }
            else {
                matrix.at<cv::Vec3b>(it.pos()) = cv::Vec3b(color[0], color[1], color[2]);
            }
        }
    }
}

// MODIFICATION : Rendu sous forme de DOTS pour cv::Mat
void CSelectDraw::DrawSelectOnMat(cv::Mat& matrix, SSelectionStyleDraw& select, const long& hScroll, const long& vScroll, const float& ratio)
{
    float thickness = std::max(1.0f, static_cast<float>(select.penSize * ratio));
    cv::Scalar color(select.color.Blue(), select.color.Green(), select.color.Red(), select.opacity);

    if (select.selectType == SELECT_RECTANGLE || select.selectType == SELECT_ELLIPSE) {
        cv::Point p1, p2;
        p1.x = static_cast<int>(XDrawingPosition(static_cast<float>(select.startPoint.x), hScroll, ratio));
        p1.y = static_cast<int>(YDrawingPosition(static_cast<float>(select.startPoint.y), vScroll, ratio));
        p2.x = static_cast<int>(XDrawingPosition(static_cast<float>(select.endPoint.x), hScroll, ratio));
        p2.y = static_cast<int>(YDrawingPosition(static_cast<float>(select.endPoint.y), vScroll, ratio));

        if (select.selectType == SELECT_RECTANGLE) {
            // Un rectangle se compose de 4 segments distincts
            cv::Point topLeft(p1.x, p1.y);
            cv::Point topRight(p2.x, p1.y);
            cv::Point bottomRight(p2.x, p2.y);
            cv::Point bottomLeft(p1.x, p2.y);

            DrawDottedLine(matrix, topLeft, topRight, color, static_cast<int>(thickness));
            DrawDottedLine(matrix, topRight, bottomRight, color, static_cast<int>(thickness));
            DrawDottedLine(matrix, bottomRight, bottomLeft, color, static_cast<int>(thickness));
            DrawDottedLine(matrix, bottomLeft, topLeft, color, static_cast<int>(thickness));
        }
        else if (select.selectType == SELECT_ELLIPSE) {
            // Modélisation d'une ellipse pointillée en reliant des segments polygonaux successifs
            cv::Point center((p1.x + p2.x) / 2, (p1.y + p2.y) / 2);
            cv::Size axes(std::abs(p1.x - p2.x) / 2, std::abs(p1.y - p2.y) / 2);

            const int steps = 40; // Nombre de segments définissant la granularité
            cv::Point prevPt;
            for (int i = 0; i <= steps; ++i) {
                double angle = (i * 2 * M_PI) / steps;
                cv::Point currPt(
                    center.x + static_cast<int>(axes.width * std::cos(angle)),
                    center.y + static_cast<int>(axes.height * std::sin(angle))
                );
                if (i > 0 && i % 2 == 1) { // Saute un segment sur deux pour créer l'effet pointillé
                    cv::line(matrix, prevPt, currPt, color, static_cast<int>(thickness), cv::LINE_8);
                }
                prevPt = currPt;
            }
        }
    }
    else if (select.selectType == SELECT_LASSO && select.points.size() > 1) {
        for (size_t i = 0; i < select.points.size() - 1; ++i) {
            cv::Point pt1, pt2;
            pt1.x = static_cast<int>(XDrawingPosition(static_cast<float>(select.points[i].x), hScroll, ratio));
            pt1.y = static_cast<int>(YDrawingPosition(static_cast<float>(select.points[i].y), vScroll, ratio));
            pt2.x = static_cast<int>(XDrawingPosition(static_cast<float>(select.points[i + 1].x), hScroll, ratio));
            pt2.y = static_cast<int>(YDrawingPosition(static_cast<float>(select.points[i + 1].y), vScroll, ratio));

            DrawDottedLine(matrix, pt1, pt2, color, static_cast<int>(thickness));
        }
    }
}

// MODIFICATION : Rendu sous forme de DOTS pour cv::UMat (Accélération matérielle OpenCL via copie locale)
void CSelectDraw::DrawSelectOnMat(cv::UMat& matrix, SSelectionStyleDraw& select, const long& hScroll, const long& vScroll, const float& ratio)
{
    // Clone la structure GPU vers une Matrice CPU locale car cv::LineIterator requiert un accès direct à la mémoire
    cv::Mat localMat;
    matrix.copyTo(localMat);

    // On applique le traitement pointillé sur la structure locale
    DrawSelectOnMat(localMat, select, hScroll, vScroll, ratio);

    // Réinjecte le résultat traité vers le flux UMat original
    localMat.copyTo(matrix);
}

void CSelectDraw::DessinerSurMat(cv::UMat& matrix, const long& hScroll, const long& vScroll, const float& ratio) {
    if (matrix.empty() || m_tousLesTraces.empty()) return;
    if (matrix.channels() == 3) cv::cvtColor(matrix, matrix, cv::COLOR_BGR2BGRA);

    try {
        for (auto& select : m_tousLesTraces) {
            DrawSelectOnMat(matrix, select, hScroll, vScroll, ratio);
        }
    }
    catch (cv::Exception& e) {
        std::cout << "Exception SelectDraw (UMat): " << e.what() << std::endl;
    }
}

void CSelectDraw::DessinerSurMat(cv::Mat& matrix, const long& hScroll, const long& vScroll, const float& ratio) {
    if (matrix.empty() || m_tousLesTraces.empty()) return;
    if (matrix.channels() == 3) cv::cvtColor(matrix, matrix, cv::COLOR_BGR2BGRA);

    try {
        for (auto& select : m_tousLesTraces) {
            DrawSelectOnMat(matrix, select, hScroll, vScroll, ratio);
        }
    }
    catch (cv::Exception& e) {
        std::cout << "Exception SelectDraw (Mat): " << e.what() << std::endl;
    }
}
