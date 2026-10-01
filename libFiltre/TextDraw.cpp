#include <header.h>
#include "TextDraw.h"
#include "TextEffectParameter.h"
#include <opencv2/imgproc.hpp>
#include <algorithm>

using namespace Regards::FiltreEffet;
using namespace Regards::Filter;

CTextDraw::CTextDraw() : m_isDrawing(false) {
    m_tousLesTextes.clear();
}

void CTextDraw::Reset() {
    m_tousLesTextes.clear();
    m_isDrawing = false;
}

void CTextDraw::MouseDown(CEffectParameter* effect) {
    m_isDrawing = true;

    // Création d'une nouvelle instance vide au moment du clic
    STextStyleDraw nouveauTexte;
    nouveauTexte.color = m_currentColor;
    nouveauTexte.fontSize = m_currentFontSize;
    nouveauTexte.fontName = m_currentFontName;
    nouveauTexte.fontIndex = m_currentFontIndex;
    nouveauTexte.isBold = m_currentIsBold;
    nouveauTexte.isItalic = m_currentIsItalic;
    nouveauTexte.opacity = m_currentOpacity;
    nouveauTexte.text = ""; // Initialisé vide : l'utilisateur va taper son texte
    nouveauTexte.point = wxPoint(0, 0);

    m_tousLesTextes.push_back(nouveauTexte);
}

void CTextDraw::MouseUp() {
    // Note : On ne passe pas m_isDrawing à false ici pour permettre de 
    // continuer à taper au clavier après avoir relâché le clic de souris.
}

void CTextDraw::GetPoint(wxPoint& pt) {
    if (!m_tousLesTextes.empty()) {
        pt = m_tousLesTextes.back().point;
    }
    else {
        pt = wxPoint(0, 0);
    }
}

void CTextDraw::InitPoint(CEffectParameter* effect, const long& m_lx, const long& m_ly, const long& m_lHScroll, const long& m_lVScroll, const float& ratio) {
    const wxPoint point(static_cast<int>(m_lx), static_cast<int>(m_ly));
    if (!VerifierValiditerPoint(point)) return;

    MouseDown(effect);

    float realX = XRealPosition(static_cast<float>(m_lx), m_lHScroll, ratio);
    float realY = YRealPosition(static_cast<float>(m_ly), m_lVScroll, ratio);

    m_tousLesTextes.back().point = wxPoint(static_cast<int>(realX), static_cast<int>(realY));
}

void CTextDraw::MouseMove(const long& xNewSize, const long& yNewSize, const long& m_lHScroll, const long& m_lVScroll, const float& ratio) {
    // Non utilisé pour la saisie clavier pure
}

// Configuration dynamique de la boîte à outils active
void CTextDraw::SetCurrentTextParams(int size, const wxString& fontName, int fontIndex, bool bold, bool italic, int opacity, const wxColour& color) {
    m_currentFontSize = size;
    m_currentFontName = fontName;
    m_currentFontIndex = fontIndex;
    m_currentIsBold = bold;
    m_currentIsItalic = italic;
    m_currentOpacity = opacity;
    m_currentColor = color;

    // Sécurité : Vérifie si un élément de texte est en cours d'édition
    if (m_tousLesTextes.empty() || !m_isDrawing) return;

    auto& texteActif = m_tousLesTextes.back();
	texteActif.color = m_currentColor;
	texteActif.fontIndex = m_currentFontIndex;
    texteActif.fontSize = m_currentFontSize;
    texteActif.fontName = m_currentFontName;
    texteActif.isBold = m_currentIsBold;
    texteActif.isItalic = m_currentIsItalic;
    texteActif.opacity = m_currentOpacity;
    texteActif.color = m_currentColor;

}

void CTextDraw::KeyDown(const int32_t& keyCode)
{
    // Sécurité : Vérifie si un élément de texte est en cours d'édition
    if (m_tousLesTextes.empty() || !m_isDrawing) return;

    auto& texteActif = m_tousLesTextes.back();

    // 1. Gestion de la touche "Retour arrière" (Effacer le dernier caractère)
    if (keyCode == WXK_BACK)
    {
        if (!texteActif.text.IsEmpty()) {
            texteActif.text.RemoveLast();
        }
    }
    // 2. Terminer la saisie sur la touche "Entrée"
    else if (keyCode == WXK_RETURN || keyCode == WXK_NUMPAD_ENTER)
    {
        m_isDrawing = false; // Désactive le focus clavier sur ce bloc de texte
    }
    // 3. Capture des caractères standards imprimables (code ASCII/Unicode valide)
    else if (keyCode >= 32 && keyCode < 127)
    {
        texteActif.text.Append(static_cast<wxChar>(keyCode));
    }
}

void CTextDraw::Dessiner(wxDC* deviceContext, const long& hScroll, const long& vScroll, const float& ratio, const wxColour& rgb, const wxColour& rgbFirst, const wxColour& rgbSecond, const int32_t& style) {
    if (deviceContext == nullptr || m_tousLesTextes.empty()) return;

    for (const auto& txt : m_tousLesTextes) {
        if (txt.opacity <= 0 || txt.text.IsEmpty()) continue;

        int screenX = static_cast<int>(XDrawingPosition(static_cast<float>(txt.point.x), hScroll, ratio));
        int screenY = static_cast<int>(YDrawingPosition(static_cast<float>(txt.point.y), vScroll, ratio));

        int scaledFontSize = std::max(4, static_cast<int>(txt.fontSize * ratio));

        wxFontStyle fontStyle = txt.isItalic ? wxFONTSTYLE_ITALIC : wxFONTSTYLE_NORMAL;
        wxFontWeight fontWeight = txt.isBold ? wxFONTWEIGHT_BOLD : wxFONTWEIGHT_NORMAL;

        wxString chosenFontName = "Arial";
        if (!txt.fontName.IsEmpty()) {
            chosenFontName = txt.fontName;
        }

        wxFont font(wxFontInfo(scaledFontSize)
            .FaceName(chosenFontName)
            .Style(fontStyle)
            .Weight(fontWeight));

        deviceContext->SetFont(font);

        wxColour textColourWithAlpha(
            txt.color.Red(),
            txt.color.Green(),
            txt.color.Blue(),
            static_cast<unsigned char>(txt.opacity)
        );

        deviceContext->SetTextForeground(textColourWithAlpha);
        deviceContext->SetBackgroundMode(wxTRANSPARENT);

        deviceContext->DrawText(txt.text, screenX, screenY);
    }

    deviceContext->SetFont(wxNullFont);
}

void CTextDraw::DessinerSurMat(cv::Mat& matrix, const long& hScroll, const long& vScroll, const float& ratio) {
    if (matrix.empty()) return;
    if (matrix.channels() == 3) cv::cvtColor(matrix, matrix, cv::COLOR_BGR2BGRA);

    for (const auto& txt : m_tousLesTextes) {
        if (txt.opacity <= 0 || txt.text.IsEmpty()) continue;

        int screenX = static_cast<int>(XDrawingPosition(static_cast<float>(txt.point.x), hScroll, ratio));
        int screenY = static_cast<int>(YDrawingPosition(static_cast<float>(txt.point.y), vScroll, ratio));

        cv::Mat overlay = matrix.clone();

        int fontFace = txt.fontIndex;
        if (txt.isItalic) fontFace |= cv::FONT_ITALIC;

        double fontScale = (txt.fontSize * 0.05) * ratio;
        int epaisseur = std::max(1, static_cast<int>((txt.isBold ? 3 : 1) * ratio));

        cv::Scalar colorOpenCV(txt.color.Blue(), txt.color.Green(), txt.color.Red(), static_cast<double>(txt.opacity));

        cv::putText(overlay, txt.text.ToStdString(), cv::Point(screenX, screenY), fontFace, fontScale, colorOpenCV, epaisseur, cv::LINE_AA);

        if (txt.opacity >= 255) {
            overlay.copyTo(matrix);
        }
        else {
            double alpha = txt.opacity / 255.0;
            cv::addWeighted(overlay, alpha, matrix, 1.0 - alpha, 0, matrix);
        }
    }
}
