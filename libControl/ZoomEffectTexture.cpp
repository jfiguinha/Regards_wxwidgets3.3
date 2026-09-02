#include <header.h>

#include "ZoomEffectTexture.h"

#include <ImageLoadingFormat.h>
#include <picture_utility.h>
#include "BitmapDisplay.h"
#include <effect_id.h>

using namespace Regards::Filter;

void CZoomEffectTextureEffect::AfterRender(
    CImageLoadingFormat* nextPicture,
    CRenderBitmapOpenGL* renderOpenGL,
    IBitmapDisplay* bmpViewer,
    const int& etape,
    const float& scale_factor,
    const bool& isNext,
    float& ratio)
{
    if (nextPicture == nullptr ||
        renderOpenGL == nullptr ||
        bmpViewer == nullptr)
    {
        return;
    }

    
        cv::Mat mat = nextPicture->GetMatrix().getMat();

    const int orientation = nextPicture->GetOrientation();

    CImageLoadingFormat bitmapTemp;
    bitmapTemp.SetPicture(mat);
    bitmapTemp.RotateExif(orientation);

    const float newRatio =
        bmpViewer->CalculPictureRatio(
            bitmapTemp.GetWidth(),
            bitmapTemp.GetHeight());

    const int widthOutput =
        static_cast<int>(bitmapTemp.GetWidth() * newRatio);

    const int heightOutput =
        static_cast<int>(bitmapTemp.GetHeight() * newRatio);

    if (initTexture ||
        pictureNext->GetWidth() != widthOutput ||
        pictureNext->GetHeight() != heightOutput)
    {
        GenerateEffectTexture(nextPicture, bmpViewer);
        initTexture = false;
    }


    // Progression de la transition : 0.0 -> 1.0
    float progress =
        static_cast<float>(etape) / 100.0f;

    progress = std::clamp(progress, 0.0f, 1.0f);


    /*
     * Facteur de zoom.
     *
     * La texture entrante démarre agrandie puis revient
     * progressivement à sa taille normale.
     *
     * Cela donne un vrai effet de ZOOM OUT visuel.
     */
    float zoom = 1.0f;

    if (isNext)
    {
        // Image entrante :
        // 130 % -> 100 %
        zoom = 1.30f - (0.30f * progress);
    }
    else
    {
        // Image entrante lors de la navigation inverse :
        // 70 % -> 100 %
        zoom = 0.70f + (0.30f * progress);
    }


    /*
     * Taille de base de l'image.
     */
    const float baseWidth =
        static_cast<float>(out.width) * scale_factor;

    const float baseHeight =
        static_cast<float>(out.height) * scale_factor;


    /*
     * Nouvelle taille.
     */
    const float zoomWidth =
        baseWidth * zoom;

    const float zoomHeight =
        baseHeight * zoom;


    /*
     * Centre ORIGINAL de l'image.
     *
     * Ce centre doit rester strictement identique pendant
     * toute la transition.
     */
    const float centerX =
        static_cast<float>(out.x) * scale_factor +
        (baseWidth / 2.0f);

    const float centerY =
        static_cast<float>(out.y) * scale_factor +
        (baseHeight / 2.0f);


    /*
     * Nouvelle position calculée autour du même centre.
     *
     * Le changement de X et Y n'est PAS un déplacement :
     * il compense exactement le changement de taille.
     */
    const float zoomX =
        centerX - (zoomWidth / 2.0f);

    const float zoomY =
        centerY - (zoomHeight / 2.0f);


    /*
     * etape reste utilisé pour le fondu alpha.
     *
     * Donc simultanément :
     *
     * - alpha : 0 -> 100
     * - zoom  : selon isNext
     */
    renderOpenGL->RenderTextureWithAlpha(
        GetTexture(0),
        etape,
        zoomWidth,
        zoomHeight,
        zoomX,
        zoomY);
    

}

void CZoomEffectTextureEffect::RenderMoveTexture(
    int& x,
    int& y,
    GLTexture* glTexture,
    const int& etape,
    const bool& isNext)
{
    /*
    * IMPORTANT :
    *
    * Aucun déplacement horizontal ou vertical.
    *
    * Cette méthode existe uniquement parce qu'elle est
    * imposée par CBitmapFusionFilter.
    */
}

int CZoomEffectTextureEffect::GetTypeFilter()
{
    return IDM_AFTEREFFECT_ZOOM;
}
