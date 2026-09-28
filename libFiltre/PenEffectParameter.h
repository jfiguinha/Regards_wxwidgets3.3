#pragma once
#include "EffectParameter.h"

struct SLineTrace {
	std::vector<wxPoint> points;
	cv::Scalar color;
	int penSize = 4;
	int typeBrush = 0;
};

class CPenFilterParameter : public CEffectParameter
{
public:



	CPenFilterParameter() {};

	bool apply = false;
	std::vector<SLineTrace> listLines; // Liste de tous les tracés avec leurs styles respectifs
	int penSize = 4;        // Taille globale courante
	cv::Scalar color;       // Couleur globale courante
	int typeBrush = 0;
};

