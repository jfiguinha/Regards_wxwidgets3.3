#pragma once
#include "EffectParameter.h"

class CCropEffectParameter : public CEffectParameter
{
public:
	CCropEffectParameter()
	{
	};

	wxRect rcZoom = { 0,0,0,0 };
	bool cropApply = false;
};
