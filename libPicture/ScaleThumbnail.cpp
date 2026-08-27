#include <header.h>
#include "ScaleThumbnail.h"
#include "ImageLoadingFormat.h"
using namespace Regards::Picture;

float CScaleThumbnail::CalculRatio(const int& width, const int& height, const int& xMax, const int& yMax) {
	if (xMax <= 0 || yMax <= 0) {
		throw std::invalid_argument("Maximum dimensions must be greater than zero.");
	}

	float widthRatio = static_cast<float>(xMax) / width;
	float heightRatio = static_cast<float>(yMax) / height;
	return std::min(widthRatio, heightRatio);
}

void CScaleThumbnail::CreateScaleBitmap(CImageLoadingFormat* pBitmap, const int& width, const int& height) {
	float newRatio = CalculRatio(pBitmap->GetWidth(), pBitmap->GetHeight(), width, height);
	if (newRatio == 0.0)
		return;

	int nTailleAffichageWidth = static_cast<int>(pBitmap->GetWidth() * newRatio);
	int nTailleAffichageHeight = static_cast<int>(pBitmap->GetHeight() * newRatio);

	cv::Mat& originalMat = pBitmap->GetMatrix().getMat();
	if (nTailleAffichageWidth != originalMat.cols || nTailleAffichageHeight != originalMat.rows) {
		cv::Mat resizedMat;
		resize(originalMat, resizedMat, cv::Size(nTailleAffichageWidth, nTailleAffichageHeight), 0, 0, cv::INTER_CUBIC);

	}
}