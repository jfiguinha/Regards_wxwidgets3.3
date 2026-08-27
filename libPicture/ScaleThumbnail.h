#pragma once

class CImageLoadingFormat;

namespace Regards
{
	namespace Picture
	{
		class CScaleThumbnail
		{
		public:
			CScaleThumbnail(void) = default;
			~CScaleThumbnail(void) = default;

			static void CreateScaleBitmap(CImageLoadingFormat* pBitmap, const int& width, const int& height);
			static float CalculRatio(const int& width, const int& height, const int& xMax, const int& yMax);
		};
	}
}
