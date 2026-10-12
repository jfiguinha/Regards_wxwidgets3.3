
#include <header.h>
#include "LayerCompositorOpenCL.h"


namespace
{
    constexpr float INV_255 = 1.0f / 255.0f;
    constexpr float ALPHA_EPSILON = 1.0e-6f;

    // Les matrices d'entrée et de sortie sont en CV_32F, avec des
    // valeurs normalisées dans [0, 1].
    void BlendChannelsOpenCL(
        const cv::UMat& src,
        const cv::UMat& dst,
        cv::UMat& out,
        int mode)
    {
        switch (mode)
        {
        case 1: // Multiply
            cv::multiply(src, dst, out);
            break;

        case 2: // Screen
        {
            cv::UMat sum, product;
            cv::add(src, dst, sum);
            cv::multiply(src, dst, product);
            cv::subtract(sum, product, out);
            break;
        }

        case 4: // Darken
            cv::min(src, dst, out);
            break;

        case 5: // Lighten
            cv::max(src, dst, out);
            break;

        case 6: // Add
        {
            cv::UMat sum;
            cv::add(src, dst, sum);
            cv::min(sum, 1.0, out);
            break;
        }

        case 7: // Subtract : destination - source
        {
            cv::UMat difference;
            cv::subtract(dst, src, difference);
            cv::max(difference, 0.0, out);
            break;
        }

        case 0: // Normal
        default:
            out = src;
            break;
        }
    }

    // Composition W3C en espace couleur non prémultiplié.
    // Toutes les entrées sont normalisées dans [0, 1].
    void CompositeLayerOpenCL(
        const cv::UMat& src,
        cv::UMat& dst,
        float opacity,
        int blendMode)
    {
        std::vector<cv::UMat> srcChannels;
        std::vector<cv::UMat> dstChannels;

        cv::split(src, srcChannels);
        cv::split(dst, dstChannels);

        // Alpha de la source tenant compte de l'opacité du calque.
        cv::UMat sa;
        cv::multiply(srcChannels[3], opacity, sa);

        // Alpha de destination.
        const cv::UMat& da = dstChannels[3];

        // Facteurs communs à tous les canaux couleur.
        cv::UMat invSa, invDa;
        cv::subtract(1.0, sa, invSa);
        cv::subtract(1.0, da, invDa);

        // Ao = As + Ad * (1 - As)
        cv::UMat termAlpha, oa;
        cv::multiply(da, invSa, termAlpha);
        cv::add(sa, termAlpha, oa);

        // Calcul des couleurs de fusion.
        std::vector<cv::UMat> srcRGB = {
            srcChannels[0], srcChannels[1], srcChannels[2]
        };

        std::vector<cv::UMat> dstRGB = {
            dstChannels[0], dstChannels[1], dstChannels[2]
        };

        cv::UMat srcColor, dstColor, blendedColor;
        cv::merge(srcRGB, srcColor);
        cv::merge(dstRGB, dstColor);

        BlendChannelsOpenCL(
            srcColor, dstColor, blendedColor, blendMode);

        std::vector<cv::UMat> blendedChannels;
        cv::split(blendedColor, blendedChannels);

        // Co = (1-As)*Ad*Cd
        //    + (1-Ad)*As*Cs
        //    + As*Ad*B(Cs,Cd)
        //
        // Toutes les opérations sont en flottant :
        // aucun écrêtage à 8 bits n'a lieu entre les étapes.

        cv::UMat factorDestination, factorSource;
        cv::multiply(invSa, da, factorDestination);
        cv::multiply(invDa, sa, factorSource);

        std::vector<cv::UMat> outputChannels(4);

        // Évite les divisions par zéro lorsque Ao = 0.
        cv::UMat safeAlpha;
        cv::max(oa, ALPHA_EPSILON, safeAlpha);

        for (int c = 0; c < 3; ++c)
        {
            cv::UMat t1, t2, t3, sum, premultiplied;

            cv::multiply(
                factorDestination, dstRGB[c], t1);

            cv::multiply(
                factorSource, srcRGB[c], t2);

            cv::UMat factorBlend;
            cv::multiply(sa, da, factorBlend);

            cv::multiply(
                factorBlend, blendedChannels[c], t3);

            cv::add(t1, t2, sum);
            cv::add(sum, t3, premultiplied);

            cv::divide(premultiplied, safeAlpha,
                outputChannels[c]);
        }

        outputChannels[3] = oa;

        cv::merge(outputChannels, dst);
    }
}

CLayerCompositorOpenCL::CLayerCompositorOpenCL()
{
    if (cv::ocl::haveOpenCL())
        cv::ocl::setUseOpenCL(true);
}

CLayerCompositorOpenCL::~CLayerCompositorOpenCL()
{
    Release();
}

void CLayerCompositorOpenCL::Release()
{
    // Les UMat libèrent leurs ressources par comptage de références.
}

bool CLayerCompositorOpenCL::Compose(
    const cv::UMat& background,
    const std::vector<LayerPassOpenCL>& layers,
    cv::UMat& target,
    int width,
    int height)
{
    if (!IsValid() ||
        background.empty() ||
        width <= 0 ||
        height <= 0 ||
        background.type() != CV_8UC4)
    {
        return false;
    }

    const cv::Size canvasSize(width, height);

    // Préparation du fond.
    cv::UMat resizedBackground;

    if (background.size() != canvasSize)
    {
        cv::resize(
            background,
            resizedBackground,
            canvasSize,
            0.0,
            0.0,
            cv::INTER_LINEAR);
    }
    else
    {
        resizedBackground = background;
    }

    // Une seule conversion de l'image de fond vers le flottant.
    // La composition complète reste ensuite en CV_32FC4.
    cv::UMat working;
    resizedBackground.convertTo(
        working, CV_32FC4, INV_255);

    for (const auto& layer : layers)
    {
        const float opacity = std::clamp(
            layer.opacity, 0.0f, 1.0f);

        if (layer.texture.empty() || opacity <= 0.0f)
            continue;

        if (layer.texture.type() != CV_8UC4)
            return false;

        cv::UMat resizedLayer;

        if (layer.texture.size() != canvasSize)
        {
            cv::resize(
                layer.texture,
                resizedLayer,
                canvasSize,
                0.0,
                0.0,
                cv::INTER_LINEAR);
        }
        else
        {
            resizedLayer = layer.texture;
        }

        // Conversion du calque courant.
        cv::UMat srcFloat;
        resizedLayer.convertTo(
            srcFloat, CV_32FC4, INV_255);

        CompositeLayerOpenCL(
            srcFloat,
            working,
            opacity,
            layer.blendMode);
    }

    // Une seule conversion finale vers le format BGRA 8 bits.
    working.convertTo(target, CV_8UC4, 255.0);

    return true;
}