#pragma once

// Rendu unique d'un trait du crayon, partagé par la preview (CPenDraw::DessinerSurMat)
// et par l'application sur l'image source (CPenFilter::RenderEffect / ApplyEffect).
// Garantit que les deux utilisent exactement les mêmes règles de dessin.

#include <vector>
#include <algorithm>
#include <iostream>
#include <opencv2/imgproc.hpp>
#include <wx/gdicmn.h>

namespace Regards::FiltreEffet
{
    // Convention unique : penSize = rayon du pinceau (en pixels de l'image source).
    // Diamètre = penSize * kPenDiameterFactor. Mettre 1 si penSize doit être un diamètre.
    constexpr int kPenDiameterFactor = 2;

    // Épaisseur de trait (diamètre) à l'échelle demandée, jamais inférieure à 1
    // (cv::line exige une épaisseur > 0).
    inline int PenThickness(const int penSize, const float scale)
    {
        return std::max(1, static_cast<int>(penSize * kPenDiameterFactor * scale));
    }

    // Dessine un trait sur 'target'.
    //  - points   : points du trait en coordonnées image source
    //  - scale    : 1.0f pour la source, ratio de zoom pour la preview
    //  - mapPoint : wxPoint (coordonnées image) -> cv::Point (coordonnées dans 'target')
    //  - color    : couleur calculée par l'appelant (inchangée par rapport à l'existant)
    template <typename PointMapper>
    void RenderPenStroke(cv::Mat& target, const std::vector<wxPoint>& points, const int penSize,
        const int typeBrush, const int opacity, const cv::Scalar& color, const float scale,
        PointMapper&& mapPoint)
    {
        if (target.empty() || points.empty() || opacity <= 0)
            return;

        const int thickness = PenThickness(penSize, scale);
        const int lineType = (typeBrush == 1) ? cv::LINE_8 : cv::LINE_AA;

        std::vector<cv::Point> pts;
        pts.reserve(points.size());
        for (const auto& point : points)
            pts.push_back(mapPoint(point));

        // 'origin' permet de dessiner dans une sous-zone (ROI) de la matrice
        auto draw = [&](cv::Mat& canvas, const cv::Point& origin)
        {
            if (pts.size() == 1)
            {
                cv::circle(canvas, pts[0] - origin, std::max(1, thickness / 2), color, -1, lineType);
                return;
            }

            for (size_t i = 1; i < pts.size(); ++i)
            {
                if (typeBrush == 2 && i % 2 != 0) continue; // Pointillés
                cv::line(canvas, pts[i - 1] - origin, pts[i] - origin, color, thickness, lineType);
            }
        };

        try
        {
            // Trait opaque : rendu direct, sans copie
            if (opacity >= 255)
            {
                draw(target, cv::Point(0, 0));
                return;
            }

            // Trait semi-transparent : overlay limité à la zone touchée par le trait
            const int margin = thickness + 2;
            cv::Rect zone = cv::boundingRect(pts);
            zone.x -= margin;
            zone.y -= margin;
            zone.width += 2 * margin;
            zone.height += 2 * margin;
            zone &= cv::Rect(0, 0, target.cols, target.rows);
            if (zone.empty())
                return;

            cv::Mat region = target(zone);
            cv::Mat overlay = region.clone();
            draw(overlay, zone.tl());

            const double alpha = opacity / 255.0;
            cv::addWeighted(overlay, alpha, region, 1.0 - alpha, 0, region);
        }
        catch (cv::Exception& e)
        {
            std::cout << "RenderPenStroke - exception caught: " << e.what() << std::endl;
        }
    }
}
