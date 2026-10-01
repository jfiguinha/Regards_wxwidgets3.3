#include <header.h>
#include "MagicHandDraw.h"
#include "MagicHandParameter.h"
#include <opencv2/imgproc.hpp>
#include <algorithm>

using namespace Regards::FiltreEffet;

CMagicHandDraw::CMagicHandDraw() : m_isDrawing(false) {}

void CMagicHandDraw::Reset() {
    m_isDrawing = false;
    m_ptReelClic = wxPoint(-1, -1);
}

void CMagicHandDraw::MouseDown(CEffectParameter* effect) {
    m_isDrawing = true;
}

void CMagicHandDraw::MouseUp() {
    m_isDrawing = false;
}

void CMagicHandDraw::GetPoint(wxPoint& pt) {
    pt = m_ptReelClic;
}

void CMagicHandDraw::InitPoint(CEffectParameter* effect, const long& m_lx, const long& m_ly,
    const long& m_lHScroll, const long& m_lVScroll, const float& ratio) {

    const wxPoint point(static_cast<int>(m_lx), static_cast<int>(m_ly));
    if (!VerifierValiditerPoint(point)) return;

    MouseDown(effect);

    float realX = XRealPosition(static_cast<float>(m_lx), m_lHScroll, ratio);
    float realY = YRealPosition(static_cast<float>(m_ly), m_lVScroll, ratio);
    m_ptReelClic = wxPoint(static_cast<int>(realX), static_cast<int>(realY));

    auto param = static_cast<CMagicHandParameter*>(effect);
    if (param) {
        param->magicWandPoint = m_ptReelClic;
    }
}

void CMagicHandDraw::DessinerSurMat(cv::Mat& matrix, const long& hScroll, const long& vScroll, const float& ratio, CMagicHandParameter* param) {
    if (matrix.empty() || m_ptReelClic == wxPoint(-1, -1) || !param) return;

    try {


        // Conversion des coordonnées du point unique avec les fonctions de m_cDessin
        int screenX = static_cast<int>(XDrawingPosition(static_cast<float>(m_ptReelClic.x), hScroll, ratio));
        int screenY = static_cast<int>(YDrawingPosition(static_cast<float>(m_ptReelClic.y), vScroll, ratio));

        cv::Point seedPoint(screenX, screenY);


        if (seedPoint.x < 0 ||
            seedPoint.x >= matrix.cols ||
            seedPoint.y < 0 ||
            seedPoint.y >= matrix.rows)
        {
            return;
        }

        // ------------------------------------------------------------
        // 3. Conversion de la tolérance
        //
        // 0 %   -> couleur exactement identique
        // 10 %  -> environ 25 niveaux par canal
        // 100 % -> 255 niveaux par canal
        // ------------------------------------------------------------

        const double tolerance =
            std::clamp(param->tolerancePercent, 0, 100) * 255.0 / 100.0;

        const cv::Scalar loDiff(
            tolerance,
            tolerance,
            tolerance);

        const cv::Scalar upDiff(
            tolerance,
            tolerance,
            tolerance);

        // ------------------------------------------------------------
        // 4. Création du masque OpenCV
        //
        // floodFill exige un masque de taille :
        // image + 2 pixels sur chaque dimension.
        // ------------------------------------------------------------

        cv::Mat mask = cv::Mat::zeros(
            matrix.rows + 2,
            matrix.cols + 2,
            CV_8UC1);

        // 4-connectivité :
        // seuls les pixels reliés horizontalement/verticalement
        // sont considérés comme appartenant à la sélection.
        //
        // FLOODFILL_MASK_ONLY :
        // l'image source n'est pas modifiée.
        //
        // 255 << 8 :
        // valeur écrite dans le masque.
        const int flags =
            4 |
            cv::FLOODFILL_FIXED_RANGE |
            cv::FLOODFILL_MASK_ONLY |
            (255 << 8);

        // ------------------------------------------------------------
        // 5. Sélection Magic Hand
        // ------------------------------------------------------------

        cv::floodFill(
            matrix,
            mask,
            seedPoint,
            cv::Scalar(255),
            nullptr,
            loDiff,
            upDiff,
            flags);

        // ------------------------------------------------------------
        // 6. Suppression de la bordure de 1 pixel du masque
        // ------------------------------------------------------------

        cv::Mat selectionMask = mask(
            cv::Rect(
                1,
                1,
                matrix.cols,
                matrix.rows));

        // ------------------------------------------------------------
        // 7. Recherche des contours de la sélection
        // ------------------------------------------------------------

        std::vector<std::vector<cv::Point>> contours;

        cv::findContours(
            selectionMask,
            contours,
            cv::RETR_EXTERNAL,
            cv::CHAIN_APPROX_SIMPLE);

        // Supprimer l'ancienne sélection
        param->contours.clear();

        // ------------------------------------------------------------
        // 8. Stockage des contours
        //
        // IMPORTANT :
        // Les contours restent dans les coordonnées de l'image.
        //
        // Ils ne doivent PAS être convertis avec
        // XDrawingPosition/YDrawingPosition ici.
        // ------------------------------------------------------------

        for (const auto& contour : contours)
        {
            if (!contour.empty())
            {
                param->contours.push_back(contour);
            }
        }

        // ------------------------------------------------------------
        // 9. Affichage de la sélection
        //
        // drawContours travaille également dans les coordonnées
        // de la matrice.
        // ------------------------------------------------------------

        if (!param->contours.empty())
        {
            cv::Scalar contourColor;

            switch (matrix.channels())
            {
            case 4:
                contourColor = cv::Scalar(0, 0, 0, 255);
                break;

            case 3:
                contourColor = cv::Scalar(0, 0, 0);
                break;

            case 1:
                contourColor = cv::Scalar(0);
                break;

            default:
                return;
            }

            cv::drawContours(
                matrix,
                param->contours,
                -1,
                contourColor,
                1,
                cv::LINE_AA);
        }
    }
    catch (cv::Exception& e) {
        std::cout << "CMagicHandDraw::DessinerSurMat - Exception : " << e.what() << std::endl;
    }
}
