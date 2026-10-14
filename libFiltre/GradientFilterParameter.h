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
        :gradientType(GRADIENT_LINEAR)
        , ptStart(0, 0)
        , ptEnd(0, 0)
        , apply(false)
    {}

    bool IfNeedTransparence() override { return true; }

    wxColour GetWxColorStart() { return this->GetColor1(); }
    wxColour GetWxColorEnd() { return this->GetColor2(); }


    int gradientType;

    // Points réels sur le bitmap (coordonnées de l'image)
    cv::Point ptStart;
    cv::Point ptEnd;

    bool apply;
};
