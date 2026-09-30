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
        else if (select.selectType == SELECT_LASSO && select.points.size() > 1) {
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
