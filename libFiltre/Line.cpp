#include <header.h>
#include "Line.h"
#include <RGBAQuad.h>
#include <algorithm>
#include <cmath>


CLine::CLine(const int& heightMax, const int& widthMax)
    : heightMax(heightMax), widthMax(widthMax) {}

CLine::~CLine() = default;

void CLine::MidpointLine(cv::Mat* bitmap, const int& xFrom, const int& yFrom,
    const int& xTo, const int& yTo, const CRgbaquad& color,
    const float& alpha, const bool& antialiasing) {

    if (bitmap == nullptr || bitmap->empty() || widthMax <= 0 || heightMax <= 0) {
        return;
    }

    const float clampedAlpha = std::clamp(alpha, 0.0f, 1.0f);
    if (clampedAlpha <= 0.0f) return;

    // 1. Pré-calcul unique des limites géométriques (Invariants sortis de la boucle)
    const int bitmapWidth = bitmap->cols;
    const int bitmapHeight = bitmap->rows;
    const int minX = bitmapWidth - widthMax;
    const int minY = bitmapHeight - heightMax;

    // Extraction des couleurs de surimpression
    const uint8_t rSrc = color.GetRed();
    const uint8_t gSrc = color.GetGreen();
    const uint8_t bSrc = color.GetBlue();

    // Facteurs de blending convertis en entiers sur une base 256
    const uint16_t aInt = static_cast<uint16_t>(clampedAlpha * 256.0f);
    const uint16_t invAInt = 256 - aInt;

    // Lambda interne optimisée remplaçant SetAlphaColorValue sans surcharge d'appel
    auto DrawPixelDirect = [&](const int& x, const int& y) {
        // Validation des contraintes spécifiques de votre ancien code
        if (x < minX || x >= widthMax || y < minY || y >= heightMax) return;
        if (x < 0 || x >= bitmapWidth || y < 0 || y >= bitmapHeight) return;

        // Accès mémoire direct au pixel
        CRgbaquad* currentColor = CRgbaquad::GetPtColorValue(bitmap, x, y);
        if (currentColor == nullptr) return;

        // Blending entier respectant votre formule d'origine :
        // (Couleur_Ligne * inverseAlpha) + (Couleur_Fond * clampedAlpha)
        uint8_t outR = static_cast<uint8_t>((rSrc * invAInt + currentColor->GetRed() * aInt) >> 8);
        uint8_t outG = static_cast<uint8_t>((gSrc * invAInt + currentColor->GetGreen() * aInt) >> 8);
        uint8_t outB = static_cast<uint8_t>((bSrc * invAInt + currentColor->GetBlue() * aInt) >> 8);

        currentColor->SetRed(outR);
        currentColor->SetGreen(outG);
        currentColor->SetBlue(outB);
        };

    // Cas particulier : point unique.
    if (xFrom == xTo && yFrom == yTo) {
        DrawPixelDirect(xFrom, yFrom);
        return;
    }

    // 2. Boucle Bresenham pure (aucune allocation, aucune condition lourde interne)
    int x0 = xFrom;
    int y0 = yFrom;
    const int x1 = xTo;
    const int y1 = yTo;

    const int dx = std::abs(x1 - x0);
    const int dy = std::abs(y1 - y0);
    const int sx = (x0 < x1) ? 1 : -1;
    const int sy = (y0 < y1) ? 1 : -1;

    int error = dx - dy;

    while (true) {
        DrawPixelDirect(x0, y0);

        if (x0 == x1 && y0 == y1) {
            break;
        }

        const int error2 = error * 2;

        if (error2 > -dy) {
            error -= dy;
            x0 += sx;
        }

        if (error2 < dx) {
            error += dx;
            y0 += sy;
        }
    }
}

// Conservée uniquement pour la rétrocompatibilité binaire si nécessaire, devenue inutile en interne
void CLine::SetAlphaColorValue(const int& xFrom, const int& yFrom, const int& x,
    const int& y, const float& alpha, const CRgbaquad& color, cv::Mat* bitmap) {
    // Cette méthode n'est plus appelée par MidpointLine pour maximiser les performances
}
