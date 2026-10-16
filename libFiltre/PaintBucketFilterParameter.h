#pragma once
#include "EffectParameter.h"
#include <opencv2/core.hpp>
#include <vector>
#include <wx/colour.h>

// Structure mémorisant chaque action du pot de peinture
struct SPaintBucketAction {
    cv::Point ptClick;
    cv::Scalar fillColor;
    int tolerance;
};

class CPaintBucketFilterParameter : public CEffectParameter
{
public:
    CPaintBucketFilterParameter() :
        tolerance(20)                       // Tolérance par défaut
        , apply(false)
    {
        actionsHistory.clear();
    }

    bool IfNeedTransparence() override { return true; }

    bool IsApplyBeforeInterpolation() override
    {
        return false;
    }

    wxColour GetWxFillColor() {
        return wxColour(color1[0], color1[1], color1[2], 255);
    }

    int tolerance;        // Tolérance courante sélectionnée pour le prochain clic

    // Historique complet des zones cliquées
    std::vector<SPaintBucketAction> actionsHistory;

    bool apply;
};
