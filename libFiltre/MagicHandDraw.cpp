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

        if (seedPoint.x >= 0 && seedPoint.x < matrix.cols && seedPoint.y >= 0 && seedPoint.y < matrix.rows) {

            // 1. GESTION DU BGRA : On isole une matrice BGR (3 canaux) pour le calcul de FloodFill
            cv::Mat srcBGR;
            if (matrix.channels() == 4) {
                cv::cvtColor(matrix, srcBGR, cv::COLOR_BGRA2BGR);
            }
            else {
                srcBGR = matrix; // Déjà en BGR ou niveaux de gris
            }

            // 2. Préparation des tolérances sur 3 canaux
            double diff = (param->tolerancePercent / 100.0) * 255.0;
            cv::Scalar loDiff(diff, diff, diff);
            cv::Scalar upDiff(diff, diff, diff);

            // Le masque de FloodFill doit obligatoirement faire +2 pixels par rapport à l'image source
            cv::Mat mask = cv::Mat::zeros(srcBGR.rows + 2, srcBGR.cols + 2, CV_8UC1);

            // 3. EXÉCUTION DU FLOODFILL
            // Note cruciale pour FLOODFILL_MASK_ONLY : La valeur de remplissage dans le masque DOIT être passée 
            // dans les bits de poids fort de l'indicateur (flags). Ici, on écrit la valeur 255 dans le masque.
            int flags = 4 | cv::FLOODFILL_MASK_ONLY | (255 << 8);

            cv::floodFill(srcBGR, mask, seedPoint, cv::Scalar(255), nullptr, loDiff, upDiff, flags);

            // 4. CORRECTION DU RECTANGLE : Extraction de la zone utile du masque
            // On recadre le masque pour enlever la bordure de 1 pixel ajoutée par OpenCV (+2 au total)
            cv::Mat actualMask = mask(cv::Rect(1, 1, srcBGR.cols, srcBGR.rows));

            // Extraction des contours géométriques sur le masque nettoyé
            std::vector<std::vector<cv::Point>> localContours;
            cv::findContours(actualMask, localContours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);

            // 5. Remplissage des paramètres et projection écran
            param->contours.clear();
            std::vector<std::vector<cv::Point>> contoursAffichage;

            for (const auto& contour : localContours) {
                std::vector<cv::Point> ajustedContourOpenCV;
                std::vector<cv::Point> contourEcran;

                for (const auto& pt : contour) {
                    // Les coordonnées ici sont désormais parfaites (0 à cols-1, 0 à rows-1)
                    int xReel = pt.x;
                    int yReel = pt.y;

                    ajustedContourOpenCV.push_back(cv::Point(xReel, yReel));

                    // Projection à la volée des coordonnées écran (Scroll + Zoom Ratio)
                    int screenX = static_cast<int>(XDrawingPosition(static_cast<float>(xReel), hScroll, ratio));
                    int screenY = static_cast<int>(YDrawingPosition(static_cast<float>(yReel), vScroll, ratio));
                    contourEcran.push_back(cv::Point(screenX, screenY));
                }

                if (!ajustedContourOpenCV.empty()) {
                    param->contours.push_back(ajustedContourOpenCV);
                    contoursAffichage.push_back(contourEcran);
                }
            }

            // 6. Rendu immédiat des contours sur la matrice d'affichage d'origine (BGRA)
            if (!contoursAffichage.empty()) {
                // Assurer que la matrice principale est en BGRA pour l'affichage graphique de l'UI
                if (matrix.channels() == 3)
                    cv::cvtColor(matrix, matrix, cv::COLOR_BGR2BGRA);

                // Dessin d'une ligne noire bien nette
                cv::drawContours(matrix, contoursAffichage, -1, cv::Scalar(0, 0, 0, 255), 1, cv::LINE_AA);
            }
        }
    }
    catch (cv::Exception& e) {
        std::cout << "CMagicHandDraw::DessinerSurMat - Exception : " << e.what() << std::endl;
    }
}
