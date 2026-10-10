#pragma once
#include "EffectParameter.h"
#include <RGBAQuad.h>

class CCloudsEffectParameter : public CEffectParameter
{
public:
	CCloudsEffectParameter()
	{
		colorFront = cv::Scalar(0, 0, 0);
		colorBack = cv::Scalar(255, 255, 255);

		transparency = 0;
		amplitude = 1;
		frequence = 65;
		octave = 8;
	};

	cv::Scalar colorFront;
	cv::Scalar colorBack;

	int octave;
	int amplitude;
	int frequence;
	int transparency;
};
