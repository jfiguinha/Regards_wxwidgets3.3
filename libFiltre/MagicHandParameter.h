#pragma once
#include "EffectParameter.h"
#include <opencv2/core.hpp>
#include <vector>

class CMagicHandParameter : public CEffectParameter
{
public:
    CMagicHandParameter() {}
    bool IfNeedTransparence() override { return true; }

    bool apply = false;
    int tolerancePercent = 10;                     // 0 à 100%
    wxPoint magicWandPoint = wxPoint(-1, -1);      // Point réel cliqué
    std::vector<std::vector<cv::Point>> contours;  // Contours finaux calculés
};
