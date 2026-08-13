#include <header.h>
#include "LensFlare.h"

#include "Color.h"
#include "circle.h"
#include "Line.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <random>

using namespace Regards::FiltreEffet;

namespace
{
    constexpr float DegToRad = 0.017453292519943295f;
    constexpr int HueMax = 360;

    int NormalizeHue(int hue)
    {
        hue %= HueMax;

        if (hue < 0)
            hue += HueMax;

        return hue;
    }

    int FloatToInt(float value)
    {
        return static_cast<int>(std::lround(value));
    }

    struct FlareAxis
    {
        float x;
        float y;

        int X(float distance) const
        {
            return FloatToInt(x * distance);
        }

        int Y(float distance) const
        {
            return FloatToInt(y * distance);
        }
    };
}


CLensFlare::CLensFlare()
    : iColorIntensity(100)
{}

CLensFlare::~CLensFlare()
{}


int CLensFlare::InsertwxImage(const wxImage& bitmap, int xPos, int yPos)
{
    if (pBitmap == nullptr || pBitmap->empty() || !bitmap.IsOk())
        return 0;

    const int imageWidth = bitmap.GetWidth();
    const int imageHeight = bitmap.GetHeight();

    if (imageWidth <= 0 || imageHeight <= 0)
        return 0;

    /*
     * Clip la zone source/destination.
     *
     * Le code original ne gérait que le débordement à droite
     * et en bas. Un lens flare peut cependant être partiellement
     * situé à gauche ou au-dessus de l'image.
     */
    const int startX = std::max(0, xPos);
    const int startY = std::max(0, yPos);

    const int endX = std::min(pBitmap->cols, xPos + imageWidth);
    const int endY = std::min(pBitmap->rows, yPos + imageHeight);

    if (startX >= endX || startY >= endY)
        return 0;

    const unsigned char* data = bitmap.GetData();

    if (data == nullptr)
        return 0;

    const unsigned char* alpha =
        bitmap.HasAlpha() ? bitmap.GetAlpha() : nullptr;

    /*
     * Chaque pixel destination est écrit une seule fois.
     * Le parallel_for reste donc possible.
     */
    tbb::parallel_for(
        startY,
        endY,
        1,
        [this, &bitmap, data, alpha, xPos, yPos, imageWidth, startX, endX](int y)
        {
            for (int x = startX; x < endX; ++x)
            {
                const int srcX = x - xPos;
                const int srcY = y - yPos;

                const int index =
                    srcY * imageWidth + srcX;

                CRgbaquad* colorDst =
                    CRgbaquad::GetPtColorValue(pBitmap, x, y);

                if (colorDst == nullptr)
                    continue;

                const unsigned char pixelAlpha =
                    alpha != nullptr ? alpha[index] : 255;

                if (pixelAlpha == 0)
                    continue;

                CRgbaquad color(
                    data[index * 3],
                    data[index * 3 + 1],
                    data[index * 3 + 2],
                    pixelAlpha);

                const float sourceAlpha =
                    static_cast<float>(pixelAlpha) / 255.0f;

                const float destinationAlpha =
                    1.0f - sourceAlpha;

                colorDst->Mul(destinationAlpha);
                color.Mul(sourceAlpha);
                colorDst->Add(color);
            }
        });

    return 0;
}


void CLensFlare::Halo(
    const int& x, const int& y, const int& iColor, const int& iTaille, const int& iWidth,
    const float& fAlpha2, const int& iCentre)
{
    if (iTaille <= 0)
        return;

    InsertwxImage(
        CCircle::Halo(
            iColor,
            iColorIntensity,
            iTaille * 2,
            iWidth,
            fAlpha2,
            iCentre),
        x - iTaille,
        y - iTaille);
}


void CLensFlare::HaloGradient(
    const int& x, const int& y, const int& iTaille, const int& iWidth,
    const float& fAlpha2)
{
    if (iTaille <= 0)
        return;

    InsertwxImage(
        CCircle::HaloGradient(
            iTaille * 2,
            iWidth,
            fAlpha2),
        x - iTaille,
        y - iTaille);
}


void CLensFlare::Circle(const int& x, const int& y, const CRgbaquad& color, const int& iTaille, const float& fAlpha)
{
    if (iTaille <= 0)
        return;

    const int rayon = iTaille / 2;

    if (rayon <= 0)
        return;

    InsertwxImage(
        CCircle::GenerateCircle(
            color,
            iTaille,
            fAlpha),
        x - rayon,
        y - rayon);
}


void CLensFlare::CircleGradient(
    const int& x, const int& y, const CRgbaquad& color, const int& iTaille,
    const float& fAlpha)
{
    if (iTaille <= 0)
        return;

    InsertwxImage(
        CCircle::GradientTransparent(
            color,
            iTaille * 2,
            fAlpha),
        x - iTaille,
        y - iTaille);
}


void CLensFlare::Burst(
    const int& x, const int& y, const int& iTaille, const int& iColor, const int& iIntensity,
    const int& iColorIntensity)
{
    if (iTaille <= 0)
        return;

    InsertwxImage(
        CCircle::Burst(
            iTaille * 2,
            iColor,
            iIntensity,
            iColorIntensity),
        x - iTaille,
        y - iTaille);
}


void CLensFlare::LensFlare(
    cv::Mat* pBitmap,
    const int& iPosX,
    const int& iPosY,
    const int& iPuissance,
    const int& iType,
    const int& iIntensity,
    const int& iColor,
    const int& iColorIntensity)
{
    /*
     * iType est conservé pour préserver l'API actuelle.
     * Il n'était pas utilisé dans l'implémentation originale.
     */
    (void)iType;

    if (pBitmap == nullptr || pBitmap->empty())
        return;

    this->pBitmap = pBitmap;
    this->iColorIntensity = iColorIntensity;

    const int iWidth = pBitmap->cols;
    const int iHeight = pBitmap->rows;

    if (iWidth <= 0 || iHeight <= 0)
        return;

    if (iPuissance <= 0)
        return;

    /*
     * Centre de l'image.
     */
    const float centerX = iWidth * 0.5f;
    const float centerY = iHeight * 0.5f;

    /*
     * Axe du lens flare.
     *
     * Le code original calculait :
     *
     *     a = dy / dx
     *     b = y - a*x
     *
     * ce qui provoquait une division par zéro lorsque dx == 0.
     *
     * Ici on travaille directement avec le vecteur directeur.
     */
    float axisX =
        centerX - static_cast<float>(iPosX);

    float axisY =
        centerY - static_cast<float>(iPosY);

    const float axisLength =
        std::sqrt(axisX * axisX + axisY * axisY);

    FlareAxis axis{};

    if (axisLength > 0.0001f)
    {
        axis.x = axisX / axisLength;
        axis.y = axisY / axisLength;
    }
    else
    {
        /*
         * Source exactement au centre.
         *
         * L'ancien code devenait indéfini dans cette situation.
         * On choisit un axe horizontal stable.
         */
        axis.x = 1.0f;
        axis.y = 0.0f;
    }

    /*
     * Vecteur opposé.
     */
    const FlareAxis oppositeAxis{
        -axis.x,
        -axis.y
    };

    /*
     * Rayon principal.
     */
    const float flareRadius =
        static_cast<float>(iPuissance) *
        (static_cast<float>(iIntensity) / 10.0f);

    if (flareRadius <= 0.0f)
        return;

    const int ray =
        std::max(
            1,
            FloatToInt(
                static_cast<float>(iPuissance) * 0.75f));

    /*
     * Couleurs.
     */
    const int color1 = NormalizeHue(iColor + 50);
    const int color2 = NormalizeHue(iColor + 100);
    const int color3 = NormalizeHue(iColor + 200);

    CRgbaquad rgbValue1;
    CRgbaquad rgbValue2;
    CRgbaquad rgbValue3;
    CRgbaquad rgbValue4;

    HSB value1{
        color1,
        100,
        100
    };

    HSB value2{
        color2,
        100,
        100
    };

    HSB value3{
        color3,
        100,
        100
    };

    HSB value4{
        NormalizeHue(iColor),
        iColorIntensity,
        100
    };

    CColor::HSBToRGB(value1, rgbValue1);
    CColor::HSBToRGB(value2, rgbValue2);
    CColor::HSBToRGB(value3, rgbValue3);
    CColor::HSBToRGB(value4, rgbValue4);


    /*
     * ------------------------------------------------------------
     * HALO 1
     * ------------------------------------------------------------
     */

    {
        const int x =
            iPosX +
            axis.X(axisLength * 0.875f);

        const int y =
            iPosY +
            axis.Y(axisLength * 0.875f);

        Halo(
            x,
            y,
            color1,
            ray,
            8,
            0.7f);
    }


    /*
     * ------------------------------------------------------------
     * HALO 2
     * ------------------------------------------------------------
     */

    {
        const int x =
            iPosX -
            axis.X(axisLength * 0.125f);

        const int y =
            iPosY -
            axis.Y(axisLength * 0.125f);

        Halo(
            x,
            y,
            color2,
            ray / 2,
            5,
            0.7f);
    }


    /*
     * ------------------------------------------------------------
     * SMALL BURST
     * ------------------------------------------------------------
     */

    {
        const int x =
            iPosX +
            axis.X(axisLength * 0.5f);

        const int y =
            iPosY +
            axis.Y(axisLength * 0.5f);

        Burst(
            x,
            y,
            ray / 8,
            iColor,
            25,
            100);
    }


    /*
     * ------------------------------------------------------------
     * BURSTS
     * ------------------------------------------------------------
     */

    {
        const int x =
            iPosX +
            axis.X(axisLength * 0.625f);

        const int y =
            iPosY +
            axis.Y(axisLength * 0.625f);

        Burst(
            x,
            y,
            ray / 10,
            color2);
    }

    {
        const int x =
            iPosX +
            axis.X(axisLength * 0.75f);

        const int y =
            iPosY +
            axis.Y(axisLength * 0.75f);

        Burst(
            x,
            y,
            ray / 8,
            color3,
            25,
            100);
    }


    /*
     * ------------------------------------------------------------
     * GRAND HALO
     * ------------------------------------------------------------
     */

    {
        const int x =
            iPosX +
            axis.X(axisLength);

        const int y =
            iPosY +
            axis.Y(axisLength);

        HaloGradient(
            x,
            y,
            ray * 4,
            std::max(1, iWidth / 20),
            0.7f);
    }


    /*
     * ------------------------------------------------------------
     * CERCLES SUR L'AXE
     * ------------------------------------------------------------
     */

    {
        const float distance =
            axisLength * 0.625f;

        const float offset =
            ray * 0.5f;

        const int x =
            iPosX +
            axis.X(distance) +
            FloatToInt(axis.x * offset);

        const int y =
            iPosY +
            axis.Y(distance) +
            FloatToInt(axis.y * offset);

        Circle(
            x,
            y,
            rgbValue1,
            FloatToInt(ray * 0.75f),
            0.8f);
    }


    {
        const float distance =
            axisLength * 0.625f;

        const float offset =
            ray * 0.5f;

        const int x =
            iPosX +
            axis.X(distance) +
            FloatToInt(axis.x * offset);

        const int y =
            iPosY +
            axis.Y(distance) +
            FloatToInt(axis.y * offset);

        Circle(
            x,
            y,
            rgbValue1,
            ray / 2,
            0.8f);
    }


    {
        const float distance =
            axisLength * 0.625f;

        const float offset =
            ray * 0.6f;

        const int x =
            iPosX +
            axis.X(distance) +
            FloatToInt(axis.x * offset);

        const int y =
            iPosY +
            axis.Y(distance) +
            FloatToInt(axis.y * offset);

        Circle(
            x,
            y,
            rgbValue1,
            ray / 4,
            0.8f);
    }


    {
        const float distance =
            axisLength * 0.4f;

        const float offset =
            -ray / 8.0f;

        const int x =
            iPosX +
            axis.X(distance) +
            FloatToInt(axis.x * offset);

        const int y =
            iPosY +
            axis.Y(distance) +
            FloatToInt(axis.y * offset);

        Circle(
            x,
            y,
            rgbValue1,
            ray / 4,
            0.8f);
    }


    /*
     * ------------------------------------------------------------
     * GROUPES DE CERCLES COLORÉS
     * ------------------------------------------------------------
     */

    {
        const float distance =
            axisLength * 0.2f;

        int x =
            iPosX +
            axis.X(distance);

        int y =
            iPosY +
            axis.Y(distance);

        Circle(
            x,
            y,
            rgbValue3,
            FloatToInt(ray * 0.75f),
            0.8f);

        /*
         * Petit cercle vers l'extérieur.
         */
        {
            const float offset =
                ray * 0.2f;

            x =
                iPosX +
                axis.X(distance) +
                FloatToInt(axis.x * offset);

            y =
                iPosY +
                axis.Y(distance) +
                FloatToInt(axis.y * offset);

            Circle(
                x,
                y,
                rgbValue3,
                FloatToInt(ray * 0.4f),
                0.8f);
        }

        /*
         * Petit cercle vers l'intérieur.
         */
        {
            const float offset =
                -ray * 0.2f;

            x =
                iPosX +
                axis.X(distance) +
                FloatToInt(axis.x * offset);

            y =
                iPosY +
                axis.Y(distance) +
                FloatToInt(axis.y * offset);

            Circle(
                x,
                y,
                rgbValue3,
                FloatToInt(ray * 0.2f),
                0.8f);
        }
    }


    /*
     * ------------------------------------------------------------
     * DERNIER CERCLE
     * ------------------------------------------------------------
     */

    {
        const float distance =
            axisLength * 0.75f;

        const int x =
            iPosX +
            axis.X(distance);

        const int y =
            iPosY +
            axis.Y(distance);

        Circle(
            x,
            y,
            rgbValue2,
            FloatToInt(ray * 0.2f),
            0.8f);
    }


    /*
     * ------------------------------------------------------------
     * SOURCE DU FLARE
     * ------------------------------------------------------------
     */

    {
        const int x = iPosX;
        const int y = iPosY;

        const int flareSize =
            std::max(
                1,
                FloatToInt(flareRadius * 0.5f));

        CircleGradient(
            x,
            y,
            rgbValue4,
            std::max(1, FloatToInt(flareRadius)),
            0.8f);

        Halo(
            x,
            y,
            iColor,
            flareSize,
            8,
            0.8f,
            0);

        Burst(
            x,
            y,
            std::max(1, FloatToInt(flareSize * 0.9f)),
            iColor,
            iIntensity,
            iColorIntensity);
    }


    /*
     * ------------------------------------------------------------
     * RAYONS LUMINEUX
     * ------------------------------------------------------------
     */

    {
        const int rayLength =
            std::max(
                1,
                FloatToInt(flareRadius * 0.5f));

        CLine line(iHeight, iWidth);

        /*
         * Générateur local :
         * - pas de rand() global
         * - séquence indépendante
         * - résultats contrôlables
         */
        std::mt19937 rng(
            std::random_device{}());

        std::uniform_real_distribution<float> distribution(
            -static_cast<float>(rayLength),
            static_cast<float>(rayLength));

        const CRgbaquad white(
            255,
            255,
            255);

        for (int angle = 0; angle <= 360; ++angle)
        {
            const float radians =
                static_cast<float>(angle) * DegToRad;

            const float randomLength =
                distribution(rng);

            const float fxValue =
                std::cos(radians) * randomLength;

            const float fyValue =
                std::sin(radians) * randomLength;

            const int dx =
                FloatToInt(fxValue);

            const int dy =
                FloatToInt(fyValue);

            line.MidpointLine(
                pBitmap,
                iPosX,
                iPosY,
                iPosX + dx,
                iPosY + dy,
                white,
                0.9f,
                true);

            line.MidpointLine(
                pBitmap,
                iPosX - dx,
                iPosY - dy,
                iPosX,
                iPosY,
                white,
                0.9f,
                true);
        }
    }


    /*
     * ------------------------------------------------------------
     * GRAND CERCLE FINAL
     * ------------------------------------------------------------
     *
     * L'ancien code utilisait le signe de iLargeur pour choisir
     * le côté opposé. On utilise maintenant directement l'axe.
     */

    {
        const float distance =
            axisLength + static_cast<float>(ray);

        const int x =
            iPosX -
            axis.X(distance);

        const int y =
            iPosY -
            axis.Y(distance);

        Circle(
            x,
            y,
            rgbValue1,
            ray * 4,
            0.95f);
    }
}
