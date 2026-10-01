#pragma once
#include "EffectParameter.h"
#include <opencv2/core.hpp>
#include <wx/colour.h>

enum EGradientType {
    GRADIENT_LINEAR = 0,
    GRADIENT_REFLECTED,
    GRADIENT_DIAMOND,
    GRADIENT_RADIAL,
    GRADIENT_CONICAL,
    GRADIENT_SPIRAL
};

class CGradientFilterParameter : public CEffectParameter
{
public:
    CGradientFilterParameter()
        : colorStart(cv::Scalar(255, 0, 0, 255)) // Bleu par défaut (BGR)
        , colorEnd(cv::Scalar(0, 0, 255, 255))   // Rouge par défaut (BGR)
        , gradientType(GRADIENT_LINEAR)
        , ptStart(0, 0)
        , ptEnd(0, 0)
        , apply(false)
    {}

    bool IfNeedTransparence() override { return true; }

    wxColour GetWxColorStart() { return wxColour(colorStart[0], colorStart[1], colorStart[2]); }
    wxColour GetWxColorEnd() { return wxColour(colorEnd[0], colorEnd[1], colorEnd[2]); }

    cv::Scalar colorStart;
    cv::Scalar colorEnd;
    int gradientType;

    // Points réels sur le bitmap (coordonnées de l'image)
    cv::Point ptStart;
    cv::Point ptEnd;

    bool apply;
};
