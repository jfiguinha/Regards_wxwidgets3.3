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
    CPaintBucketFilterParameter()
        : fillColor(cv::Scalar(0, 0, 0, 255)) // Noir par défaut (BGR)
        , tolerance(20)                       // Tolérance par défaut
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
        return wxColour(fillColor[0], fillColor[1], fillColor[2], 255);
    }

    cv::Scalar fillColor; // Couleur courante sélectionnée pour le prochain clic
    int tolerance;        // Tolérance courante sélectionnée pour le prochain clic

    // Historique complet des zones cliquées
    std::vector<SPaintBucketAction> actionsHistory;

    bool apply;
};
