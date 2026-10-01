#pragma once

#include "EffectParameter.h"
#include <vector>
#include <opencv2/core.hpp>
#include <wx/string.h>
#include <wx/colour.h>

namespace Regards::Filter {

    struct STextTrace {
        wxPoint position;       // Position d'ancrage réelle sur l'image
        wxString text;          // Contenu du texte
        wxString fontName;      // Nom de la police (ex: "Arial")
        cv::Scalar color;       // Couleur au format OpenCV (BGR)
        int fontSize = 20;      // Taille de la police
        bool isBold = false;    // Option Gras
        bool isItalic = false;  // Option Italique
        int opacity = 255;      // Transparence (0 à 255)
		int  fontIndex = 0;      // Index OpenCV de la police (cv::HersheyFonts)
    };

    class CTextFilterParameter : public CEffectParameter
    {
    public:
        CTextFilterParameter() : apply(false), fontSize(24), fontIndex(0), isBold(false), isItalic(false), textToDraw("Texte") {
            opacity = 255;
        }


        bool IfNeedTransparence() override { return false; }

        wxColour ConvertScalarToWxColour() {
            int blue = static_cast<int>(color[0]);
            int green = static_cast<int>(color[1]);
            int red = static_cast<int>(color[2]);
            return wxColour(red, green, blue, opacity);
        }

        bool apply;
        std::vector<STextTrace> listTexts;

        // Paramètres courants de l'interface
        wxString textToDraw;
        wxString fontName;
        int fontSize;
        int fontIndex;       // Index OpenCV de la police (cv::HersheyFonts)
        cv::Scalar color;
        bool isBold;
        bool isItalic;
    };
}
