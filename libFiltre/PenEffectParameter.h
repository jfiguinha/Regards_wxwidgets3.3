#pragma once

#include "EffectParameter.h"
#include <vector>
#include <opencv2/core.hpp>
#include "PenDrawInfo.h" 

struct SLineTrace {
	std::vector<wxPoint> points;
	cv::Scalar color;
	int penSize = 4;
	int typeBrush = 0;
	int opacity = 255; // <-- Ajout de l'opacité par trait (0 à 255)
};

class CPenFilterParameter : public CEffectParameter
{
public:
	CPenFilterParameter() {};

	bool apply = false;
	std::vector<SLineTrace> listLines;
	int penSize = 4;
	cv::Scalar color;
	int typeBrush = 0;
	int opacity = 255;      // <-- Ajout de l'opacité globale courante (255 = 100% opaque)
};
