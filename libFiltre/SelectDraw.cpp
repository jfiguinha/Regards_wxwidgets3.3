#include <header.h>
#include "SelectDraw.h"
#include <algorithm>
#include <cmath>
#include "SelectFilterParameter.h"

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
    // Fixé par le filtre en dur : Blanc, 1px, pas de remplissage, opaque
    nouvelleSelection.color = wx_color;
    nouvelleSelection.penSize = 1;
    nouvelleSelection.opacity = 255;
    nouvelleSelection.isFilled = false;
    nouvelleSelection.selectType = param->selectType; // Seul le type provient du paramètre
    nouvelleSelection.points.clear();

    m_tousLesTraces.push_back(nouvelleSelection);
}

void CSelectDraw::GetStartPoint(wxPoint& pt) {
    if (!m_tousLesTraces.empty()) pt = m_tousLesTraces.back().startPoint;
    else pt = wxPoint(-1, -1);
}

// MET À JOUR L'HISTORIQUE AVEC LES NOUVEAUX POINTS DE LA BAGUETTE MAGIQUE
void CSelectDraw::InjectExternalShape(int type, const std::vector<wxPoint>& pointsReels) {
    m_tousLesTraces.clear(); // Conserve une sélection unique active

    SSelectionStyleDraw shape;
    shape.selectType = type;
    shape.color = wx_color;
    shape.penSize = 1;
    shape.opacity = 255;
    shape.isFilled = false;
    shape.points = pointsReels; // Copie et intègre l'ensemble des points générés

    if (!pointsReels.empty()) {
        shape.startPoint = pointsReels.front();
        shape.endPoint = pointsReels.back();
    }
    else {
        shape.startPoint = wxPoint(0, 0);
        shape.endPoint = wxPoint(0, 0);
    }

    m_tousLesTraces.push_back(shape);
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

void CSelectDraw::Dessiner(wxDC* deviceContext, const long& hScroll, const long& vScroll, const float& ratio, const wxColour& rgb, const wxColour& rgbFirst, const wxColour& rgbSecond, const int32_t& style) {
    if (deviceContext == nullptr || m_tousLesTraces.empty()) return;

    for (const auto& select : m_tousLesTraces) {
        int epaisseur = std::max(1, static_cast<int>(1 * ratio));
        wxPen pen(select.color, epaisseur, wxPENSTYLE_SHORT_DASH); // Lignes pointillées de sélection

        deviceContext->SetPen(pen);
        deviceContext->SetBrush(*wxTRANSPARENT_BRUSH);

        int x1 = static_cast<int>(XDrawingPosition(static_cast<float>(select.startPoint.x), hScroll, ratio));
        int y1 = static_cast<int>(YDrawingPosition(static_cast<float>(select.startPoint.y), vScroll, ratio));
        int x2 = static_cast<int>(XDrawingPosition(static_cast<float>(select.endPoint.x), hScroll, ratio));
        int y2 = static_cast<int>(YDrawingPosition(static_cast<float>(select.endPoint.y), vScroll, ratio));

        if (select.selectType == SELECT_RECTANGLE) {
            deviceContext->DrawRectangle(wxRect(wxPoint(x1, y1), wxPoint(x2, y2)));
        }
        else if (select.selectType == SELECT_ELLIPSE) {
            deviceContext->DrawEllipse(wxRect(wxPoint(x1, y1), wxPoint(x2, y2)));
        }
        else if ((select.selectType == SELECT_LASSO || select.selectType == SELECT_MAGICWAND) && select.points.size() > 1) {
            std::vector<wxPoint> screenPoints;
            for (const auto& ptReel : select.points) {
                int sx = static_cast<int>(XDrawingPosition(static_cast<float>(ptReel.x), hScroll, ratio));
                int sy = static_cast<int>(YDrawingPosition(static_cast<float>(ptReel.y), vScroll, ratio));
                screenPoints.push_back(wxPoint(sx, sy));
            }
            deviceContext->DrawLines(static_cast<int>(screenPoints.size()), screenPoints.data());
        }
    }
    deviceContext->SetBrush(wxNullBrush);
    deviceContext->SetPen(wxNullPen);
}

// MISE À JOUR DE LA FUSION OPENCV AVEC PRISE EN CHARGE DU TYPE SELECT_MAGICWAND
void CSelectDraw::DrawSelectionOnMat(cv::Mat& matrix, SSelectionStyleDraw& selection, const long& hScroll, const long& vScroll, const float& ratio) {
    int thickness = 1;
    int lineStyle = cv::LINE_AA;

    cv::Point p1;
    cv::Point p2;
    p1.x = static_cast<int>(XDrawingPosition(static_cast<float>(selection.startPoint.x), hScroll, ratio));
    p1.y = static_cast<int>(XDrawingPosition(static_cast<float>(selection.startPoint.y), vScroll, ratio));
    p2.x = static_cast<int>(XDrawingPosition(static_cast<float>(selection.endPoint.x), hScroll, ratio));
    p2.y = static_cast<int>(YDrawingPosition(static_cast<float>(selection.endPoint.y), vScroll, ratio));

    if (selection.selectType == SELECT_RECTANGLE) {
        cv::rectangle(matrix, p1, p2, cvColor, thickness, lineStyle);
    }
    else if (selection.selectType == SELECT_ELLIPSE) {
        cv::Point center((p1.x + p2.x) / 2, (p1.y + p2.y) / 2);
        cv::Size axes(std::abs(p2.x - p1.x) / 2, std::abs(p2.y - p1.y) / 2);
        cv::ellipse(matrix, center, axes, 0, 0, 360, cvColor, thickness, lineStyle);
    }
    // GESTION DE LA BAGUETTE MAGIQUE ALIGNÉE SUR LE RENDU DU LASSO
    else if ((selection.selectType == SELECT_LASSO || selection.selectType == SELECT_MAGICWAND) && selection.points.size() > 1) {
        std::vector<cv::Point> cvPoints;
        for (const auto& ptReel : selection.points) {
            int cx = static_cast<int>(XDrawingPosition(static_cast<float>(ptReel.x), hScroll, ratio));
            int cy = static_cast<int>(YDrawingPosition(static_cast<float>(ptReel.y), vScroll, ratio));
            cvPoints.push_back(cv::Point(cx, cy));
        }

        std::vector<std::vector<cv::Point>> ppt = { cvPoints };
        cv::polylines(matrix, ppt, true, cvColor, thickness, lineStyle);
    }
}

void CSelectDraw::DessinerSurMat(cv::Mat& matrix, const long& hScroll, const long& vScroll, const float& ratio) {
    if (matrix.empty()) return;
    if (matrix.channels() == 3) cv::cvtColor(matrix, matrix, cv::COLOR_BGR2BGRA);

    for (auto& select : m_tousLesTraces) {
        DrawSelectionOnMat(matrix, select, hScroll, vScroll, ratio);
    }
}
