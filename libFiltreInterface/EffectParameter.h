#pragma once

class CEffectParameter
{
public:

	virtual bool IfNeedTransparence()
	{
		return false;
	}

	bool updateEffect = false;
	int opacity = 255;
};
