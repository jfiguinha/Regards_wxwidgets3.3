#include <header.h>
#include "BitmapEditor.h"
#include <ViewerTheme.h>
#include <FiltreEffet.h>
#include <ViewerThemeInit.h>
#include <RGBAQuad.h>
#include <ImageLoadingFormat.h>
using namespace Regards::Viewer;

CBitmapEditor::CBitmapEditor(CSliderInterface* slider, wxWindowID mainViewerId, const CThemeBitmapWindow& theme,
	CBitmapInterface* bitmapInterface) : Regards::Control::CBitmapWndViewer(slider, mainViewerId, theme, bitmapInterface)
{
	fixArrow = false;
}

bool CBitmapEditor::ApplySelectEffect(int& widthOutput, int& heightOutput)
{ 
	return false; 
}

CBitmapEditor::~CBitmapEditor(void)
{}

void CBitmapEditor::SetRealSize()
{
	ratio = 1.0f;
	this->RefreshWindow();
}

void CBitmapEditor::SetFilename(const wxString& filename)
{
	this->filename = filename;
}

void CBitmapEditor::Resize(const int& widthOut, const int& heightOut, const int& method)
{
	CImageLoadingFormat* source = listOfLayer.GetPictureToShow();

	CRgbaquad color;
	CFiltreEffet filtreEffet(color, nullptr, source);
	cv::Mat output = filtreEffet.Resize(widthOut, heightOut, method);

	source->SetPicture(output);

	loadBitmap = true;
	bitmapLoad = true;
	bitmapUpdate = true;
	flipVertical = 0;
	flipHorizontal = 0;
	angle = 0;

	toolOption = MOVEPICTURE;
	bitmapwidth = source->GetWidth();
	bitmapheight = source->GetHeight();
	orientation = source->GetOrientation();


	ShrinkImage(false);
	AfterSetBitmap();

	RemoveListener(false);

	//needToRefresh = true;
	parentRender->Refresh();

	RefreshWindow();
}