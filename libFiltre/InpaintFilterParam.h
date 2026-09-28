#pragma once

#include "EffectParameter.h"

class CInpaintFilterParameter : public CEffectParameter
{
public:
	CInpaintFilterParameter()
	{
		rcZoom = { 0,0,0,0 };
		algo = 0; //RECURS_FILTER 
	};

	bool cropApply = false;
	wxRect rcZoom;
	int algo;
};

