#pragma once

class CResourceCursor
{
public:
	CResourceCursor()
	{
	};

	~CResourceCursor()
	{
	};
	static wxCursor GetClosedHand();
	static wxCursor CreateDynamicZoomCursor(bool zoomIn);
};
