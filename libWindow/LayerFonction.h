#pragma once

#include "LayerBlend.h"

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

    // Normalise les images source en BGRA 8 bits. Les données d'entrée ne sont
    // converties/redimensionnées qu'une seule fois par calque et par composition.
    static cv::Mat PrepareLayerImage(const cv::Mat& input, int width, int height)
    {
        if (input.empty() || width <= 0 || height <= 0)
            return {};

        cv::Mat eightBit;
        if (input.depth() == CV_8U)
            eightBit = input;
        else
            input.convertTo(eightBit, CV_MAKETYPE(CV_8U, input.channels()));

        cv::Mat bgra;
        if (eightBit.channels() == 4)
            bgra = eightBit;
        else if (eightBit.channels() == 3)
            cv::cvtColor(eightBit, bgra, cv::COLOR_BGR2BGRA);
        else if (eightBit.channels() == 1)
            cv::cvtColor(eightBit, bgra, cv::COLOR_GRAY2BGRA);
        else
            return {};

        if (bgra.cols != width || bgra.rows != height)
        {
            cv::Mat resized;
            const int interpolation =
                (bgra.cols > width || bgra.rows > height) ? cv::INTER_AREA : cv::INTER_LINEAR;
            cv::resize(bgra, resized, cv::Size(width, height), 0.0, 0.0, interpolation);
            return resized;
        }
        return bgra;
    }


    // Normalise les images source en BGRA 8 bits. Les données d'entrée ne sont
// converties/redimensionnées qu'une seule fois par calque et par composition.
    static cv::UMat PrepareLayerImageOpenCL(const cv::UMat& input, int width, int height)
    {
        if (input.empty() || width <= 0 || height <= 0)
            return {};

        cv::UMat eightBit;
        if (input.depth() == CV_8U)
            eightBit = input;
        else
            input.convertTo(eightBit, CV_MAKETYPE(CV_8U, input.channels()));

        cv::UMat bgra;
        if (eightBit.channels() == 4)
            bgra = eightBit;
        else if (eightBit.channels() == 3)
            cv::cvtColor(eightBit, bgra, cv::COLOR_BGR2BGRA);
        else if (eightBit.channels() == 1)
            cv::cvtColor(eightBit, bgra, cv::COLOR_GRAY2BGRA);
        else
            return {};

        if (bgra.cols != width || bgra.rows != height)
        {
            cv::UMat resized;
            const int interpolation =
                (bgra.cols > width || bgra.rows > height) ? cv::INTER_AREA : cv::INTER_LINEAR;
            cv::resize(bgra, resized, cv::Size(width, height), 0.0, 0.0, interpolation);
            return resized;
        }
        return bgra;
    }

    // Composition source-over avec alpha droit (straight alpha), et mode de
    // fusion appliqué aux couleurs. Aucun split/merge ni Mat flottant temporaire.
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

                    // Formule W3C de composition avec mode de fusion :
                    // Co = (1-as)*ad*Cd + (1-ad)*as*Cs + as*ad*B(Cs,Cd)
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

    static bool CompositeOneLayer(cv::Mat& canvas, LayerElement* layer,
        int width, int height)
    {
        if (!layer || !layer->isVisible || !layer->GetPicture() ||
            !layer->GetPicture()->IsOk())
            return false;

        const float opacity = Clamp01(layer->opacity / 100.0f);
        if (opacity <= 0.0f)
            return false;

        cv::Mat source = PrepareLayerImage(layer->GetPicture()->GetMatImage(), width, height);
        if (source.empty())
            return false;

        CompositeLayer(canvas, source, opacity, layer->blendMode);
        return true;
    }
};