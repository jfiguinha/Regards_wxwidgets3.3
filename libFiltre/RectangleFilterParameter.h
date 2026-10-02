#pragma once

#include "EffectParameter.h"
#include <vector>
#include <opencv2/core.hpp>

// Types de dessin
#define SHAPE_RECTANGLE 0
#define SHAPE_CIRCLE    1
#define SHAPE_LINE      2

struct SShapeTrace {
	wxPoint startPoint;
	wxPoint endPoint;
	int shapeType = SHAPE_RECTANGLE; // Rectangle, Cercle, Ligne
	cv::Scalar color;
	int penSize = 4;
	int typeBrush = 0; // Forme du trait (Standard, Carré, Pointillés)
	int opacity = 255;
	bool isFilled = false; // Dessin rempli ou non

	cv::Scalar GetColorWithTransparancy()
	{
		return cv::Scalar(color[0], color[1], color[2], static_cast<double>(opacity));
	}
};

class CRectangleFilterParameter : public CEffectParameter
{
public:
	wxColour ConvertScalarToWxColour()
	{
		int blue = static_cast<int>(color[0]);
		int green = static_cast<int>(color[1]);
		int red = static_cast<int>(color[2]);
		return wxColour(red, green, blue, opacity);
	}

	bool IfNeedTransparence() override
	{
		return true;
	}

	bool IsApplyBeforeInterpolation() override
	{
		return false;
	}


	CRectangleFilterParameter() {};

	bool apply = false;
	std::vector<SShapeTrace> listShapes;
	int shapeType = SHAPE_RECTANGLE;
	int penSize = 4;
	cv::Scalar color;
	int typeBrush = 0;
	bool isFilled = false;
};
