// ==================== LayerCompositorOpenCL.cpp ====================
#include <header.h>
#include "LayerCompositorOpenCL.h"
#include <opencv2/imgproc.hpp>
#include <opencv2/core.hpp>

CLayerCompositorOpenCL::CLayerCompositorOpenCL()
{
    // Force l'initialisation d'OpenCL au démarrage si disponible
    if (cv::ocl::haveOpenCL())
    {
        cv::ocl::setUseOpenCL(true);
    }
}

CLayerCompositorOpenCL::~CLayerCompositorOpenCL()
{
    Release();
}

void CLayerCompositorOpenCL::Release()
{
    // cv::UMat gérant sa propre mémoire (compteur de références), 
    // il n'y a pas de structures lourdes à libérer manuellement ici.
}
// ==================== Corrected BlendChannelsOpenCL ====================
static void BlendChannelsOpenCL(const cv::UMat& src, const cv::UMat& dst, cv::UMat& out, int mode)
{
    // Normalisation temporaire en flottant [0.0, 1.0] pour les calculs de fusion (exécuté via OpenCL)
    cv::UMat s, d;
    src.convertTo(s, CV_32F, 1.0 / 255.0);
    dst.convertTo(d, CV_32F, 1.0 / 255.0);

    cv::UMat result;
    switch (mode)
    {
    case 1: // Multiply
        cv::multiply(s, d, result);
        break;

    case 2: // Screen : s + d - s * d
    {
        cv::UMat sum_sd, mul_sd;
        cv::add(s, d, sum_sd);
        cv::multiply(s, d, mul_sd);
        cv::subtract(sum_sd, mul_sd, result);
    }
    break;

    case 4: // Darken
        cv::min(s, d, result);
        break;

    case 5: // Lighten
        cv::max(s, d, result);
        break;

    case 6: // Add : min(1.0, s + d)
    {
        cv::UMat sum_sd;
        cv::add(s, d, sum_sd);
        cv::min(sum_sd, 1.0, result);
    }
    break;

    case 7: // Subtract : max(0.0, d - s)
    {
        cv::UMat sub_ds;
        cv::subtract(d, s, sub_ds);
        cv::max(sub_ds, 0.0, result);
    }
    break;

    case 0: // Normal
    default:
        result = s;
        break;
    }

    result.convertTo(out, CV_8U, 255.0);
}


bool CLayerCompositorOpenCL::Compose(const cv::UMat& background, const std::vector<LayerPassOpenCL>& layers,
    cv::UMat& target, int width, int height)
{
    if (!IsValid() || background.empty() || width <= 0 || height <= 0)
        return false;

    // 1. Initialisation de la cible avec l'image de fond (Redimensionnement OpenCL si nécessaire)
    if (background.cols != width || background.rows != height)
        cv::resize(background, target, cv::Size(width, height), 0, 0, cv::INTER_LINEAR);
    else
        background.copyTo(target);

    // S'assurer que le canevas de destination est bien au format standard BGRA 8 bits
    if (target.type() != CV_8UC4)
        return false;

    // 2. Itération sur chaque calque
    for (const auto& layer : layers)
    {
        if (layer.texture.empty() || layer.opacity <= 0.0f)
            continue;

        // Préparation / redimensionnement du calque courant sur le GPU
        cv::UMat srcLayer;
        if (layer.texture.cols != width || layer.texture.rows != height)
            cv::resize(layer.texture, srcLayer, cv::Size(width, height), 0, 0, cv::INTER_LINEAR);
        else
            srcLayer = layer.texture;

        // Séparation des canaux OpenCL (BGRA)
        std::vector<cv::UMat> srcChannels, dstChannels;
        cv::split(srcLayer, srcChannels);
        cv::split(target, dstChannels);

        // Extraction et application de l'opacité sur l'Alpha du calque (Canal index 3)
        cv::UMat sa;
        cv::multiply(srcChannels[3], layer.opacity, sa); // sa = src.a * opacity

        // Calcul de l'alpha final : oa = sa + da * (1.0 - sa)
        cv::UMat invSa, term2, oa;
        cv::subtract(255.0, sa, invSa);
        cv::multiply(dstChannels[3], invSa, term2, 1.0 / 255.0);
        cv::add(sa, term2, oa);

        // Traitement des canaux de couleurs (B, G, R)
        std::vector<cv::UMat> srcRGB = { srcChannels[0], srcChannels[1], srcChannels[2] };
        std::vector<cv::UMat> dstRGB = { dstChannels[0], dstChannels[1], dstChannels[2] };

        cv::UMat srcRGBMat, dstRGBMat;
        cv::merge(srcRGB, srcRGBMat);
        cv::merge(dstRGB, dstRGBMat);

        // Application du mode de fusion sur la couleur (B(Cs, Cd)) via le pipeline OpenCL
        cv::UMat blendedRGB;
        BlendChannelsOpenCL(srcRGBMat, dstRGBMat, blendedRGB, layer.blendMode);

        std::vector<cv::UMat> blendedChannels;
        cv::split(blendedRGB, blendedChannels);

        // Formule W3C de composition avec Alpha (Source-Over) convertie en opérations vectorielles :
        // Co = (1 - sa)*da*Cd + (1 - da)*sa*Cs + sa*da*Blended
        std::vector<cv::UMat> outRGBChannels(3);
        cv::UMat invDa;
        cv::subtract(255.0, dstChannels[3], invDa);

        for (int c = 0; c < 3; ++c)
        {
            cv::UMat t1, t2, t3, sum1, totalColor;

            // t1 = (1 - sa) * da * Cd
            cv::multiply(invSa, dstChannels[3], t1, 1.0 / 255.0);
            cv::multiply(t1, dstRGB[c], t1, 1.0 / 255.0);

            // t2 = (1 - da) * sa * Cs
            cv::multiply(invDa, sa, t2, 1.0 / 255.0);
            cv::multiply(t2, srcRGB[c], t2, 1.0 / 255.0);

            // t3 = sa * da * Blended
            cv::multiply(sa, dstChannels[3], t3, 1.0 / 255.0);
            cv::multiply(t3, blendedChannels[c], t3, 1.0 / 255.0);

            // Somme prémultipliée
            cv::add(t1, t2, sum1);
            cv::add(sum1, t3, totalColor);

            // Normalisation par l'alpha final (unmap / division) : Co / oa
            // Pour éviter la division par zéro, on utilise cv::divide
            cv::divide(totalColor, oa, outRGBChannels[c], 255.0);
        }

        // Reconstruction de l'image finale combinée pour la passe suivante
        std::vector<cv::UMat> finalChannels = { outRGBChannels[0], outRGBChannels[1], outRGBChannels[2], oa };
        cv::merge(finalChannels, target);
    }

    return true;
}
