#pragma once
#include "EffectParameter.h"

class CFreeRotateEffectParameter : public CEffectParameter
{
public:
	CFreeRotateEffectParameter()
	{
		color = cv::Scalar(0, 0, 0, 0);
		angle = 0;
	};

	cv::Scalar color;
	int angle;
};
