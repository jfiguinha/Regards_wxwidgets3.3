#pragma once
#include <Draw.h> // Assurez-vous que cette classe de base virtuelle possède bien la méthode virtuelle DessinerSurMat
#include <opencv2/core.hpp>
#include <wx/gdicmn.h>
#include <vector>



#define SELECT_RECTANGLE 0
#define SELECT_ELLIPSE   1
#define SELECT_LASSO     2

class CEffectParameter;
struct SSelectionStyleDraw; // À déclarer selon votre structure existante

namespace Regards {
    namespace FiltreEffet {

        struct SSelectionStyleDraw {
            wxPoint startPoint;
            wxPoint endPoint;
            std::vector<wxPoint> points;
            int selectType;
            wxColour color;
            int penSize;
            int opacity;
            bool isFilled;
        };


        class CSelectDraw : public CDraw
        {
        public:
            CSelectDraw();
            virtual ~CSelectDraw() {}

            void Reset();
            void MouseUp();
            void MouseDown(CEffectParameter* effect);
            void GetPoint(wxPoint& pt);

            void InitPoint(CEffectParameter* effect, const long& m_lx, const long& m_ly, const long& m_lHScroll, const long& m_lVScroll, const float& ratio);
            void MouseMove(const long& xNewSize, const long& yNewSize, const long& m_lHScroll, const long& m_lVScroll, const float& ratio);

            // MODIFICATIONS : Surcharges pour le dessin direct sur Mat et UMat (comme CRectangleDraw)
            void DessinerSurMat(cv::Mat& matrix, const long& hScroll, const long& vScroll, const float& ratio);
            void DessinerSurMat(cv::UMat& matrix, const long& hScroll, const long& vScroll, const float& ratio);

            // Accesseur utilisé par CSelectFilter pour récupérer les tracés
            const std::vector<SSelectionStyleDraw>& GetTousLesTraces() const { return m_tousLesTraces; }


            std::vector<SSelectionStyleDraw> m_tousLesTraces;

        private:
            // Méthodes internes de dessin pour factoriser le code
            void DrawSelectOnMat(cv::Mat& matrix, SSelectionStyleDraw& select, const long& hScroll, const long& vScroll, const float& ratio);
            void DrawSelectOnMat(cv::UMat& matrix, SSelectionStyleDraw& select, const long& hScroll, const long& vScroll, const float& ratio);

            
            bool m_isDrawing;
        };

    }
}
