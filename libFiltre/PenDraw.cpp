#include <header.h>
#include "PenDraw.h"
#include <algorithm>
#include "PenEffectParameter.h"

using namespace Regards::FiltreEffet;

CPenDraw::CPenDraw() : m_isDrawing(false) {
    m_tousLesTraces.clear();
}

void CPenDraw::Reset() {
    m_tousLesTraces.clear();
    m_isDrawing = false;
}


void CPenDraw::MouseDown(CEffectParameter* effect) {
    m_isDrawing = true;


    CPenFilterParameter* penEffect = (CPenFilterParameter*)effect;

    SLineStyleDraw nouvelleLigne;
    nouvelleLigne.color = penEffect->ConvertScalarToWxColour();
    nouvelleLigne.penSize = penEffect->penSize;
    nouvelleLigne.typeBrush = penEffect->typeBrush;
    nouvelleLigne.opacity = penEffect->opacity; // <-- On fige l'opacité pour ce trait
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
    int hpos = hScroll;
    int vpos = vScroll;

    // 2. Sécurité : S'assurer que la matrice de rendu est au format 4 canaux (BGRA)
    if (matrix.channels() == 3)
        cv::cvtColor(matrix, matrix, cv::COLOR_BGR2BGRA);


    try
    {
        // 4. Parcours de l'ensemble des tracés (issus de l'image réelle)
        for (const auto& ligne : m_tousLesTraces)
        {
            if (ligne.points.empty()) continue;

            // Calcul de l'épaisseur du trait corrigée avec le ratio d'affichage (Similaire à CPenDraw)
            int epaisseurAffichage = std::max<int>(1, static_cast<int>(ligne.penSize * 2 * ratio));
            int lineStyle = (ligne.typeBrush == 1) ? cv::LINE_8 : cv::LINE_AA;

            // Si le tracé est visible
            if (ligne.opacity > 0)
            {
                // Pour gérer l'alpha blending (transparence), on utilise un overlay de la taille de la matrice d'affichage
                cv::Mat overlay = matrix.clone();

                if (ligne.points.size() == 1)
                {
                    // Conversion des coordonnées du point unique avec les fonctions de m_cDessin
                    int screenX = static_cast<int>(XDrawingPosition(static_cast<float>(ligne.points[0].x), hpos, ratio));
                    int screenY = static_cast<int>(YDrawingPosition(static_cast<float>(ligne.points[0].y), vpos, ratio));

                    int rayon = std::max(1, epaisseurAffichage / 2);
                    cv::circle(overlay, cv::Point(screenX, screenY), rayon, GetColorWithTransparancy(ligne.color, ligne.opacity), -1, lineStyle);
                }
                else
                {
                    for (size_t i = 1; i < ligne.points.size(); ++i)
                    {
                        if (ligne.typeBrush == 2 && i % 2 != 0) continue; // Pointillés

                        // Conversion des coordonnées des points (i-1) et (i)
                        int xPrecedent = static_cast<int>(XDrawingPosition(static_cast<float>(ligne.points[i - 1].x), hpos, ratio));
                        int yPrecedent = static_cast<int>(YDrawingPosition(static_cast<float>(ligne.points[i - 1].y), vpos, ratio));

                        int xActuel = static_cast<int>(XDrawingPosition(static_cast<float>(ligne.points[i].x), hpos, ratio));
                        int yActuel = static_cast<int>(YDrawingPosition(static_cast<float>(ligne.points[i].y), vpos, ratio));

                        cv::line(overlay, cv::Point(xPrecedent, yPrecedent), cv::Point(xActuel, yActuel), GetColorWithTransparancy(ligne.color, ligne.opacity), epaisseurAffichage, lineStyle);
                    }
                }

                // Application de la transparence (Alpha Blending)
                if (ligne.opacity >= 255)
                {
                    // Si opaque à 100%, on copie directement l'overlay modifié sur la matrice
                    overlay.copyTo(matrix);
                }
                else
                {
                    // Sinon, fusion mathématique de l'overlay transparent par-dessus la matrice d'affichage d'origine
                    double alpha = ligne.opacity / 255.0;
                    double beta = 1.0 - alpha;
                    cv::addWeighted(overlay, alpha, matrix, beta, 0, matrix);
                }
            }
        }
    }
    catch (cv::Exception& e)
    {
        const char* err_msg = e.what();
        std::cout << "CPenFilter::Drawing - exception caught: " << err_msg << std::endl;
    }

}
