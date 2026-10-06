#pragma once

class CEffectParameter
{
public:

	static wxColour ConvertScalarToWxColour(cv::Scalar color, int opacity)
	{
		// Extraction des canaux OpenCV (Indices standard : 0 = Blue, 1 = Green, 2 = Red, 3 = Alpha)
		int red = static_cast<int>(color[0]);
		int green = static_cast<int>(color[1]);
		int blue = static_cast<int>(color[2]);

		return wxColour(red, green, blue, opacity);
	}


	wxColour GetColor1()
	{
		int red = static_cast<int>(color1[0]);
		int green = static_cast<int>(color1[1]);
		int blue = static_cast<int>(color1[2]);

		return wxColour(red, green, blue, opacity);
	}


	static cv::Scalar ConvertwxColourToScalar(wxColour color, int opacity)
	{
		// Extraction des canaux OpenCV (Indices standard : 0 = Blue, 1 = Green, 2 = Red, 3 = Alpha)
		int red = static_cast<int>(color.GetBlue());
		int green = static_cast<int>(color.GetGreen());
		int blue = static_cast<int>(color.GetRed());

		return cv::Scalar(red, green, blue, opacity);
	}

	virtual bool IfNeedTransparence()
	{
		return false;
	}

	virtual bool IsApplyBeforeInterpolation()
	{
		return false;
	}

	bool updateEffect = false;
	int opacity = 255;
	cv::Scalar color1;
	cv::Scalar color2;
};
