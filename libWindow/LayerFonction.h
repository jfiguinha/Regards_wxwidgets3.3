#pragma once

#include "LayerBlend.h"
#include <opencv2/opencv.hpp>
#include <opencv2/core/ocl.hpp>
#include <algorithm>

class LayerFonction
{
public:
    static inline float Clamp01(float value)
    {
        return std::max(0.0f, std::min(1.0f, value));
    }

    static inline float BlendChannel(float source, float destination, LayerBlendMode mode)
    {
        switch (mode)
        {
        case LayerBlendMode::Multiply:
            return source * destination;
        case LayerBlendMode::Screen:
            return source + destination - source * destination;
        case LayerBlendMode::Overlay:
            return destination <= 0.5f
                ? 2.0f * source * destination
                : 1.0f - 2.0f * (1.0f - source) * (1.0f - destination);
        case LayerBlendMode::Darken:
            return std::min(source, destination);
        case LayerBlendMode::Lighten:
            return std::max(source, destination);
        case LayerBlendMode::Add:
            return std::min(1.0f, source + destination);
        case LayerBlendMode::Subtract:
            return std::max(0.0f, destination - source);
        case LayerBlendMode::Normal:
        default:
            return source;
        }
    }
    static cv::Mat PrepareLayerImage(const cv::Mat& input, int width, int height, int layerX = 0, int layerY = 0, float ratio = 1.0f)
    {
        if (input.empty() || width <= 0 || height <= 0)
            return {};

        // Calcul immédiat des décalages réels
        int offsetX = static_cast<int>(layerX * ratio);
        int offsetY = static_cast<int>(layerY * ratio);

        // PAS DE COPIE : Si l'image correspond déjà exactement aux critères requis
        if (input.cols == width && input.rows == height && input.type() == CV_8UC4 && offsetX == 0 && offsetY == 0)
        {
            return input;
        }

        // 1. Conversion de la profondeur de pixel en 8 bits
        cv::Mat eightBit = (input.depth() == CV_8U) ? input : cv::Mat();
        if (eightBit.empty())
            input.convertTo(eightBit, CV_MAKETYPE(CV_8U, input.channels()));

        // 2. Normalisation en format 4 canaux BGRA
        cv::Mat bgra = eightBit;
        if (eightBit.channels() == 3)
            cv::cvtColor(eightBit, bgra, cv::COLOR_BGR2BGRA);
        else if (eightBit.channels() == 1)
            cv::cvtColor(eightBit, bgra, cv::COLOR_GRAY2BGRA);
        else if (eightBit.channels() != 4)
            return {};

        // 3. Redimensionnement adaptatif
        cv::Mat resized = bgra;
        if (bgra.cols != width || bgra.rows != height)
        {
            const int interpolation = (bgra.cols > width || bgra.rows > height) ? cv::INTER_AREA : cv::INTER_LINEAR;
            cv::resize(bgra, resized, cv::Size(width, height), 0.0, 0.0, interpolation);
        }

        // Si après normalisation/redimensionnement il n'y a toujours pas d'offset, on évite le canvas intermédiaire
        if (offsetX == 0 && offsetY == 0 && resized.cols == width && resized.rows == height)
        {
            return resized;
        }

        // 4. Création du canvas et translation géométrique
        cv::Mat finalCanvas = cv::Mat::zeros(height, width, CV_8UC4);
        cv::Rect dstROI = cv::Rect(offsetX, offsetY, resized.cols, resized.rows) & cv::Rect(0, 0, width, height);

        if (!dstROI.empty())
        {
            cv::Rect srcROI(dstROI.x - offsetX, dstROI.y - offsetY, dstROI.width, dstROI.height);
            resized(srcROI).copyTo(finalCanvas(dstROI));
        }

        return finalCanvas;
    }

    // MODIFICATION : Version OpenCL prenant en compte les offsets et le ratio
    static cv::UMat PrepareLayerImageOpenCL(const cv::UMat& input, int width, int height, int layerX = 0, int layerY = 0, float ratio = 1.0f)
    {
        if (input.empty() || width <= 0 || height <= 0)
            return {};

        // 1. Conversion de la profondeur de pixel en 8 bits
        cv::UMat eightBit;
        if (input.depth() == CV_8U)
            eightBit = input;
        else
            input.convertTo(eightBit, CV_MAKETYPE(CV_8U, input.channels()));

        // 2. Normalisation en format 4 canaux BGRA
        cv::UMat bgra;
        if (eightBit.channels() == 4)
            bgra = eightBit;
        else if (eightBit.channels() == 3)
            cv::cvtColor(eightBit, bgra, cv::COLOR_BGR2BGRA);
        else if (eightBit.channels() == 1)
            cv::cvtColor(eightBit, bgra, cv::COLOR_GRAY2BGRA);
        else
            return {};

        // 3. Redimensionnement OpenCL
        cv::UMat resized = bgra;
        if (bgra.cols != width || bgra.rows != height)
        {
            const int interpolation =
                (bgra.cols > width || bgra.rows > height) ? cv::INTER_AREA : cv::INTER_LINEAR;
            cv::resize(bgra, resized, cv::Size(width, height), 0.0, 0.0, interpolation);
        }

        // 4. TRANSLATION OPENCL VIA TECHNIQUE DE CANVAS TRANSPARENT (Rembobinage CPU temporaire pour ROI sécurisé si nécessaire)
        // Crée une matrice transparente de base
        cv::Mat canvasMat = cv::Mat::zeros(height, width, CV_8UC4);
        cv::UMat finalCanvas = canvasMat.getUMat(cv::ACCESS_WRITE);

        int offsetX = static_cast<int>(layerX * ratio);
        int offsetY = static_cast<int>(layerY * ratio);

        cv::Rect dstROI = cv::Rect(offsetX, offsetY, resized.cols, resized.rows) & cv::Rect(0, 0, width, height);

        if (!dstROI.empty())
        {
            cv::Rect srcROI(dstROI.x - offsetX, dstROI.y - offsetY, dstROI.width, dstROI.height);
            // Copie matérielle OpenCL (UMat vers UMat)
            resized(srcROI).copyTo(finalCanvas(dstROI));
        }

        return finalCanvas;
    }


    struct ParallelCompositeInvoker : public cv::ParallelLoopBody
    {
        cv::Mat& destination;
        const cv::Mat& source;
        float layerOpacity;
        LayerBlendMode mode;

        ParallelCompositeInvoker(cv::Mat& dst, const cv::Mat& src, float opacity, LayerBlendMode m)
            : destination(dst), source(src), layerOpacity(opacity), mode(m) {}

        virtual void operator()(const cv::Range& range) const override
        {
            const int width = destination.cols;

            for (int y = range.start; y < range.end; ++y)
            {
                auto* dst = destination.ptr<cv::Vec4b>(y);
                const auto* src = source.ptr<cv::Vec4b>(y);

                for (int x = 0; x < width; ++x)
                {
                    const float sourceAlpha = (src[x][3] / 255.0f) * layerOpacity;
                    if (sourceAlpha <= 0.0f)
                        continue;

                    const float destinationAlpha = dst[x][3] / 255.0f;
                    const float outputAlpha = sourceAlpha + destinationAlpha * (1.0f - sourceAlpha);

                    for (int c = 0; c < 3; ++c)
                    {
                        const float sourceColor = src[x][c] / 255.0f;
                        const float destinationColor = dst[x][c] / 255.0f;

                        // Appel statique à votre logique de fusion existante
                        const float blendedColor = LayerFonction::BlendChannel(sourceColor, destinationColor, mode);

                        const float premultiplied =
                            (1.0f - sourceAlpha) * destinationAlpha * destinationColor +
                            (1.0f - destinationAlpha) * sourceAlpha * sourceColor +
                            sourceAlpha * destinationAlpha * blendedColor;

                        const float outputColor = outputAlpha > 0.0f ? (premultiplied / outputAlpha) : 0.0f;
                        dst[x][c] = cv::saturate_cast<uchar>(LayerFonction::Clamp01(outputColor) * 255.0f);
                    }

                    dst[x][3] = cv::saturate_cast<uchar>(LayerFonction::Clamp01(outputAlpha) * 255.0f);
                }
            }
        }
    };

    static void CompositeLayer(cv::Mat& destination, const cv::Mat& source, float layerOpacity, LayerBlendMode mode)
    {
        if (destination.empty() || source.empty() || layerOpacity <= 0.0f)
            return;

        CV_Assert(destination.type() == CV_8UC4 && source.type() == CV_8UC4);

        // Déclenchement du traitement parallèle parallélisé sur les lignes de l'image
        cv::parallel_for_(cv::Range(0, destination.rows), ParallelCompositeInvoker(destination, source, layerOpacity, mode));
    }

    /*
    static void CompositeLayer(cv::Mat& destination, const cv::Mat& source,
        float layerOpacity, LayerBlendMode mode)
    {
        if (destination.empty() || source.empty() || layerOpacity <= 0.0f)
            return;

        CV_Assert(destination.type() == CV_8UC4 && source.type() == CV_8UC4);
        const int width = destination.cols;

        for (int y = 0; y < destination.rows; ++y)
        {
            auto* dst = destination.ptr<cv::Vec4b>(y);
            const auto* src = source.ptr<cv::Vec4b>(y);

            for (int x = 0; x < width; ++x)
            {
                const float sourceAlpha = (src[x][3] / 255.0f) * layerOpacity;
                if (sourceAlpha <= 0.0f)
                    continue;

                const float destinationAlpha = dst[x][3] / 255.0f;
                const float outputAlpha = sourceAlpha + destinationAlpha * (1.0f - sourceAlpha);

                for (int c = 0; c < 3; ++c)
                {
                    const float sourceColor = src[x][c] / 255.0f;
                    const float destinationColor = dst[x][c] / 255.0f;
                    const float blendedColor = BlendChannel(sourceColor, destinationColor, mode);

                    const float premultiplied =
                        (1.0f - sourceAlpha) * destinationAlpha * destinationColor +
                        (1.0f - destinationAlpha) * sourceAlpha * sourceColor +
                        sourceAlpha * destinationAlpha * blendedColor;

                    const float outputColor = outputAlpha > 0.0f
                        ? premultiplied / outputAlpha
                        : 0.0f;
                    dst[x][c] = cv::saturate_cast<uchar>(Clamp01(outputColor) * 255.0f);
                }

                dst[x][3] = cv::saturate_cast<uchar>(Clamp01(outputAlpha) * 255.0f);
            }
        }
    }
    */
    static bool CompositeOneLayer(cv::Mat& canvas, LayerElement* layer,
        int width, int height)
    {
        if (!layer || !layer->isVisible || !layer->GetPicture() ||
            !layer->GetPicture()->IsOk())
            return false;

        const float opacity = Clamp01(layer->opacity / 100.0f);
        if (opacity <= 0.0f)
            return false;

        // Note : En cas d'appel générique depuis l'extérieur, on passe le positionnement (si géré au niveau global)
        cv::Mat source = PrepareLayerImage(layer->GetPicture()->GetMatImage(), width, height, layer->x, layer->y, 1.0f);
        if (source.empty())
            return false;

        CompositeLayer(canvas, source, opacity, layer->blendMode);
        return true;
    }
};
