#include <header.h>
#include "PenDraw.h"
#include <algorithm>
#include "PenEffectParameter.h"
#include "PenStrokeRenderer.h"

using namespace Regards::FiltreEffet;

CPenDraw::CPenDraw() : m_isDrawing(false) {
    m_tousLesTraces.clear();
    applyEffectOnMouseRelease = true;
}

void CPenDraw::Reset() {
    m_tousLesTraces.clear();
    m_isDrawing = false;
}


void CPenDraw::MouseDown(CEffectParameter* effect) {
    m_isDrawing = true;

    m_tousLesTraces.clear();

    CPenFilterParameter* penEffect = (CPenFilterParameter*)effect;

    SLineStyleDraw nouvelleLigne;
    nouvelleLigne.color = penEffect->GetColor1();
    nouvelleLigne.penSize = penEffect->penSize;
    nouvelleLigne.typeBrush = penEffect->typeBrush;
    nouvelleLigne.opacity = penEffect->opacity; // <-- On fige l'opacité pour ce trait
    nouvelleLigne.points.clear();

    m_tousLesTraces.push_back(nouvelleLigne);
}


void CPenDraw::MouseUp() {
    m_isDrawing = false;
    //Active the modification
}

void CPenDraw::GetPoint(wxPoint& pt) {
    if (!m_tousLesTraces.empty() && !m_tousLesTraces.back().points.empty()) {
        pt = m_tousLesTraces.back().points.back();
    }
    else {
        pt = wxPoint(0, 0);
    }
}

void CPenDraw::InitPoint(CEffectParameter* effect, const long& m_lx, const long& m_ly,
    const long& m_lHScroll, const long& m_lVScroll, const float& ratio) {

    const wxPoint point(static_cast<int>(m_lx), static_cast<int>(m_ly));
    if (!VerifierValiditerPoint(point)) return;

    if (m_tousLesTraces.empty() || !m_isDrawing) {
        MouseDown(effect);
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


void CPenDraw::SetTransparenceValue(wxImage& drawingImage)
{

    // 5. Fusion par balayage de pixels (ignorer le Magenta)
    unsigned char* baseData = drawingImage.GetData();

    unsigned char* alphaData = drawingImage.GetAlpha();

    int width = drawingImage.GetWidth();
    int height = drawingImage.GetHeight();

    if (baseData && alphaData && m_currentOpacity != 0)
    {
        for (int i = 0; i < width * height; ++i)
        {
            int idx = i * 3;

            unsigned char r = baseData[idx];
            unsigned char g = baseData[idx + 1];
            unsigned char b = baseData[idx + 2];

            // Détection du fond : Si le pixel N'EST PAS le magenta pur (255, 0, 255)
            if (!(r == 255 && g == 0 && b == 255) && alphaData[i] == 0)
            {
                alphaData[i] = m_currentOpacity;
            }
        }
    }
}





void CPenDraw::DessinerSurMat(cv::Mat& matrix, const long& hScroll, const long& vScroll, const float& ratio)
{
    if (matrix.empty()) return;

    // 1. Récupération des paramètres de défilement (Scroll) et de zoom (Ratio) depuis le viewer
    const int hpos = hScroll;
    const int vpos = vScroll;

    // 2. Sécurité : S'assurer que la matrice de rendu est au format 4 canaux (BGRA)
    if (matrix.channels() == 3)
        cv::cvtColor(matrix, matrix, cv::COLOR_BGR2BGRA);

    // 3. Conversion image réelle -> coordonnées d'affichage (scroll + zoom)
    const auto toScreen = [&](const wxPoint& point)
        {
            return cv::Point(
                static_cast<int>(XDrawingPosition(static_cast<float>(point.x), hpos, ratio)),
                static_cast<int>(YDrawingPosition(static_cast<float>(point.y), vpos, ratio)));
        };

    // 4. Parcours de l'ensemble des tracés (issus de l'image réelle).
    //    Le rendu est le même que sur l'image source (RenderPenStroke), à l'échelle 'ratio'.
    for (const auto& ligne : m_tousLesTraces)
    {
        RenderPenStroke(matrix, ligne.points, ligne.penSize, ligne.typeBrush, ligne.opacity,
            GetColorWithTransparancy(ligne.color, ligne.opacity), ratio, toScreen);
    }
}