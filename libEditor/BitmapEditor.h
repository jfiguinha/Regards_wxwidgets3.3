#pragma once
#include <BitmapWndViewer.h>


class CBitmapEditor : public Regards::Control::CBitmapWndViewer
{
public:
	CBitmapEditor(CSliderInterface* slider, wxWindowID mainViewerId, const CThemeBitmapWindow& theme,
		CBitmapInterface* bitmapInterface);
	~CBitmapEditor(void);

	void Resize(const int& widthOut, const int& heightOut, const int& method);
	void SetFilename(const wxString& filename);
	void SetRealSize();
};