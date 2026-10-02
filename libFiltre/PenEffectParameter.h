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
	bool rgba = true;
	int opacity = 255; // <-- Ajout de l'opacité par trait (0 à 255)

	cv::Scalar GetColorWithTransparancy()
	{
		// OpenCV utilise le format BGR(A) : 
		// color[0] = Blue, color[1] = Green, color[2] = Red
		// On écrase le 4ème canal (indice 3) avec la valeur d'opacité courante du trait
		if(rgba)
			return cv::Scalar(color[0], color[1], color[2], static_cast<double>(opacity));
		return cv::Scalar(color[2], color[1], color[0], static_cast<double>(opacity));
	}

	wxColour ConvertScalarToWxColour(const cv::Scalar& scalar_color, int alpha)
	{
		// Extraction des canaux OpenCV (Indices standard : 0 = Blue, 1 = Green, 2 = Red, 3 = Alpha)
		int blue = static_cast<int>(scalar_color[0]);
		int green = static_cast<int>(scalar_color[1]);
		int red = static_cast<int>(scalar_color[2]);

		return wxColour(red, green, blue, alpha);
	}
};

class CPenFilterParameter : public CEffectParameter
{
public:

	wxColour ConvertScalarToWxColour()
	{
		// Extraction des canaux OpenCV (Indices standard : 0 = Blue, 1 = Green, 2 = Red, 3 = Alpha)
		int blue = static_cast<int>(color[0]);
		int green = static_cast<int>(color[1]);
		int red = static_cast<int>(color[2]);

		return wxColour(red, green, blue, opacity);
	}

	bool IfNeedTransparence() override
	{
		return true;
	}
	bool IsApplyBeforeInterpolation() override
	{
		return false;
	}


	CPenFilterParameter() {};

	bool apply = false;
	std::vector<SLineTrace> listLines;
	int penSize = 4;
	cv::Scalar color;
	int typeBrush = 0;
	bool rgba = true;
   // <-- Ajout de l'opacité globale courante (255 = 100% opaque)
};
