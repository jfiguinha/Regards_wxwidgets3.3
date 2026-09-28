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

    SLineStyleDraw nouvelleLigne;
    nouvelleLigne.color = m_currentColor;
    nouvelleLigne.penSize = m_currentSize;
    nouvelleLigne.typeBrush = m_currentBrush;
    nouvelleLigne.opacity = m_currentOpacity; // <-- On fige l'opacité pour ce trait
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

    // Enregistrement continu des valeurs en provenance de l'interface
    m_currentColor = rgbFirst;
    m_currentSize = style;

    // Le paramètre rgbSecond n'étant pas utilisé, ou via une autre variable, 
    // le plus propre est de détourner temporairement 'rgbSecond' pour faire passer le type de brush, 
    // ou de laisser le CPenFilter::Drawing injecter la valeur directement.
    // Pour l'instant, m_currentBrush est mis à jour par le filtre juste avant (voir étape 2).

    if (m_tousLesTraces.empty()) return;

    // Parcours de l'ensemble des tracés enregistrés de manière indépendante
    for (const auto& ligne : m_tousLesTraces) {
        if (ligne.points.empty()) continue;



        // Calcul du diamètre d'affichage à l'écran (proportions conservées selon le zoom/ratio)
        int epaisseurAffichage = std::max(1, static_cast<int>(ligne.penSize * 2 * ratio));

        wxPenStyle penStylewx = wxPENSTYLE_SOLID;
        if (ligne.typeBrush == 2) {
            penStylewx = wxPENSTYLE_SHORT_DASH; // Style de trait en pointillés
        }

        // Instanciation du crayon avec sa couleur et son épaisseur historique
        wxPen pen(ligne.color, epaisseurAffichage, penStylewx);

        // 2. Gestion de la géométrie des extrémités et des jointures selon le type de Brush
        if (ligne.typeBrush == 1) {
            // Pinceau de forme Carrée
            pen.SetCap(wxCAP_PROJECTING);
            pen.SetJoin(wxJOIN_MITER);
        }
        else {
            // Pinceau Standard de forme Ronde
            pen.SetCap(wxCAP_ROUND);
            pen.SetJoin(wxJOIN_ROUND);
        }

        // Application du crayon sur le contexte graphique de périphérique
        deviceContext->SetPen(pen);
        deviceContext->SetBrush(*wxTRANSPARENT_BRUSH); // Remplissage transparent pour le tracé de lignes

        // 3. Rendu vectoriel du tracé à l'écran
        if (ligne.points.size() == 1) {
            // S'il n'y a qu'un seul point isolé (clic fixe sans mouvement)
            int screenX = static_cast<int>(XDrawingPosition(static_cast<float>(ligne.points[0].x), hScroll, ratio));
            int screenY = static_cast<int>(YDrawingPosition(static_cast<float>(ligne.points[0].y), vScroll, ratio));

            deviceContext->DrawLine(screenX, screenY, screenX, screenY);
        }
        else {
            // S'il y a plusieurs coordonnées successives, on trace des segments continus
            for (size_t i = 1; i < ligne.points.size(); ++i) {
                int xPrecedent = static_cast<int>(XDrawingPosition(static_cast<float>(ligne.points[i - 1].x), hScroll, ratio));
                int yPrecedent = static_cast<int>(YDrawingPosition(static_cast<float>(ligne.points[i - 1].y), vScroll, ratio));

                int xActuel = static_cast<int>(XDrawingPosition(static_cast<float>(ligne.points[i].x), hScroll, ratio));
                int yActuel = static_cast<int>(YDrawingPosition(static_cast<float>(ligne.points[i].y), vScroll, ratio));

                deviceContext->DrawLine(xPrecedent, yPrecedent, xActuel, yActuel);
            }
        }
    }

    // 4. Restauration et nettoyage standard des outils graphiques GDI de wxWidgets
    deviceContext->SetBrush(wxNullBrush);
    deviceContext->SetPen(wxNullPen);
}
