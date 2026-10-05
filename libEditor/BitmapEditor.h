#pragma once
#include <BitmapWndViewer.h>


class CBitmapEditor : public Regards::Control::CBitmapWndViewer
{
public:
	CBitmapEditor(CSliderInterface* slider, wxWindowID mainViewerId, const CThemeBitmapWindow& theme,
		CBitmapInterface* bitmapInterface);
	~CBitmapEditor(void);

	void SetRealSize();
};