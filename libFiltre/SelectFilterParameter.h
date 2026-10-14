#pragma once

#include "EffectParameter.h"
#include <vector>
#include <opencv2/core.hpp>

#define SELECT_RECTANGLE 0
#define SELECT_ELLIPSE   1
#define SELECT_LASSO     2
#define SELECT_MAGICWAND 3 

struct SSelectionTrace {
	wxPoint startPoint;
	wxPoint endPoint;
	std::vector<wxPoint> points; // Pour le mode Lasso
	int selectType = SELECT_RECTANGLE;
	int penSize = 1;             // Fixé par le filtre
	cv::Scalar color = cv::Scalar(0, 0, 0, 0); // Blanc en dur
};

class CSelectFilterParameter : public CEffectParameter
{
public:
	CSelectFilterParameter() {
	};

	bool IfNeedTransparence() override { return true; } // Contour net

	bool apply = false;
	std::vector<SSelectionTrace> listSelections;
	int selectType = SELECT_RECTANGLE;
	int penSize = 1; // Fixé en dur

	// Utilisation d'un pourcentage (0 à 100)
	int tolerancePercent = 10;                // 10% par défaut
	wxPoint magicWandPoint = wxPoint(-1, -1);
};
