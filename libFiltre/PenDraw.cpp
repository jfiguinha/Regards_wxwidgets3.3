#include <header.h>
#include "PenDraw.h"
#include <algorithm>


using namespace Regards::FiltreEffet;

CPenDraw::CPenDraw() : m_isDrawing(false) {
    m_tousLesTraces.clear();
}

void CPenDraw::Reset() {
    m_tousLesTraces.clear();
    m_isDrawing = false;
}



void CPenDraw::MouseDown() {
    m_isDrawing = true;

    // On instancie une nouvelle ligne qui va capturer le style courant capté dans Dessiner()
    SLineStyleDraw nouvelleLigne;
    nouvelleLigne.color = m_currentColor;
    nouvelleLigne.penSize = m_currentSize;
    nouvelleLigne.points.clear();

    m_tousLesTraces.push_back(nouvelleLigne);
}

void CPenDraw::MouseUp() {
    m_isDrawing = false;
}

void CPenDraw::GetPoint(wxPoint& pt) {
    if (!m_tousLesTraces.empty() && !m_tousLesTraces.back().points.empty()) {
        pt = m_tousLesTraces.back().points.back();
    }
    else {
        pt = wxPoint(0, 0);
    }
}

void CPenDraw::InitPoint(const long& m_lx, const long& m_ly,
    const long& m_lHScroll, const long& m_lVScroll, const float& ratio) {

    const wxPoint point(static_cast<int>(m_lx), static_cast<int>(m_ly));
    if (!VerifierValiditerPoint(point)) return;

    if (m_tousLesTraces.empty() || !m_isDrawing) {
        MouseDown();
    }

    float realX = XRealPosition(static_cast<float>(m_lx), m_lHScroll, ratio);
    float realY = YRealPosition(static_cast<float>(m_ly), m_lVScroll, ratio);

    m_tousLesTraces.back().points.push_back(wxPoint(static_cast<int>(realX), static_cast<int>(realY)));
}

void CPenDraw::MouseMove(const long& xNewSize, const long& yNewSize,
    const long& m_lHScroll, const long& m_lVScroll, const float& ratio) {

    if (!m_isDrawing || m_tousLesTraces.empty()) return;

    const wxPoint point(static_cast<int>(xNewSize), static_cast<int>(yNewSize));
    if (!VerifierValiditerPoint(point)) return;

    float realX = XRealPosition(static_cast<float>(xNewSize), m_lHScroll, ratio);
    float realY = YRealPosition(static_cast<float>(yNewSize), m_lVScroll, ratio);

    m_tousLesTraces.back().points.push_back(wxPoint(static_cast<int>(realX), static_cast<int>(realY)));
}

void CPenDraw::Dessiner(wxDC* deviceContext, const long& hScroll,
    const long& vScroll, const float& ratio,
    const wxColour& rgb, const wxColour& rgbFirst,
    const wxColour& rgbSecond, const int32_t& style) {

    if (deviceContext == nullptr) return;

    // On mémorise en continu les valeurs en provenance de l'interface graphique (Filtre -> Clavier/Souris)
    // Ainsi, au prochain MouseDown(), le bon style sera déjà enregistré.
    m_currentColor = rgbFirst;
    m_currentSize = style;

    if (m_tousLesTraces.empty()) return;

    // Rendu à l'écran de chaque ligne avec son propre style historique sauvegardé
    for (const auto& ligne : m_tousLesTraces) {
        if (ligne.points.empty()) continue;

        int epaisseurAffichage = std::max(1, static_cast<int>(ligne.penSize * 2 * ratio));
        wxPen pen(ligne.color, epaisseurAffichage, wxPENSTYLE_SOLID);
        deviceContext->SetPen(pen);
        deviceContext->SetBrush(*wxTRANSPARENT_BRUSH);

        if (ligne.points.size() == 1) {
            int screenX = static_cast<int>(XDrawingPosition(static_cast<float>(ligne.points[0].x), hScroll, ratio));
            int screenY = static_cast<int>(YDrawingPosition(static_cast<float>(ligne.points[0].y), vScroll, ratio));
            deviceContext->DrawLine(screenX, screenY, screenX, screenY);
        }
        else {
            for (size_t i = 1; i < ligne.points.size(); ++i) {
                int xPrecedent = static_cast<int>(XDrawingPosition(static_cast<float>(ligne.points[i - 1].x), hScroll, ratio));
                int yPrecedent = static_cast<int>(YDrawingPosition(static_cast<float>(ligne.points[i - 1].y), vScroll, ratio));

                int xActuel = static_cast<int>(XDrawingPosition(static_cast<float>(ligne.points[i].x), hScroll, ratio));
                int yActuel = static_cast<int>(YDrawingPosition(static_cast<float>(ligne.points[i].y), vScroll, ratio));

                deviceContext->DrawLine(xPrecedent, yPrecedent, xActuel, yActuel);
            }
        }
    }

    deviceContext->SetBrush(wxNullBrush);
    deviceContext->SetPen(wxNullPen);
}
