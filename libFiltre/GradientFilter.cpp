#include <header.h>
#include "GradientFilter.h"
#include "GradientFilterParameter.h"
#include "GradientDraw.h"
#include <LibResource.h>
#include <FiltreEffet.h>
#include <ImageLoadingFormat.h>
#include <BitmapDisplay.h>
#include <Metadata.h>
#include <treetypeid.h>
#include <opencv2/imgproc.hpp>
#include <cmath>

using namespace Regards::Filter;
using namespace cv;
using namespace std;

CGradientFilter::CGradientFilter()
{
    libelleColorStart = "Effect.Color Start"; // Modifiable par CLibResource::LoadStringFromResource
    libelleColorEnd = "Effect.Color End"; // Modifiable par CLibResource::LoadStringFromResource
    libelleGradientType = "Effect.Gradient Type"; // Modifiable par CLibResource::LoadStringFromResource
}

CGradientFilter::~CGradientFilter() {}

int CGradientFilter::GetTypeFilter() { return IDM_GRADIENTFILTER; } // Affectez un ID unique disponible dans effect_id.h
int CGradientFilter::GetNameFilter() { return IDM_GRADIENTFILTER; }
wxString CGradientFilter::GetFilterLabel() { return "Gradient Filter"; }

CEffectParameter* CGradientFilter::GetEffectPointer() { return new CGradientFilterParameter(); }
CDraw* CGradientFilter::GetDrawingPt() { return new Regards::FiltreEffet::CGradientDraw(); }

void CGradientFilter::AddMetadataElement(vector<CMetadata>& element, wxString value, int key)
{
    CMetadata data;
    data.value = value;
    data.depth = key;
    element.push_back(data);
}

void CGradientFilter::Filter(CEffectParameter* effectParameter, cv::Mat& source, const wxString& filename, IFiltreEffectInterface* filtreInterface)
{
    this->source = source;
    this->filename = filename;
    auto param = static_cast<CGradientFilterParameter*>(effectParameter);

    vector<int> elementDummy;

    // Ajout des sélecteurs de couleur
    filtreInterface->AddTreeInfos(libelleColorStart, new CTreeElementValueColor(param->GetWxColorStart()), &elementDummy, TYPE_COLOR, TYPE_COLOR);
    filtreInterface->AddTreeInfos(libelleColorEnd, new CTreeElementValueColor(param->GetWxColorEnd()), &elementDummy, TYPE_COLOR, TYPE_COLOR);

    // Ajout de la liste déroulante des types de dégradé
    vector<CMetadata> gradientOptions;
    AddMetadataElement(gradientOptions, "Linear", GRADIENT_LINEAR);
    AddMetadataElement(gradientOptions, "Linear Reflected", GRADIENT_REFLECTED);
    AddMetadataElement(gradientOptions, "Diamond", GRADIENT_DIAMOND);
    AddMetadataElement(gradientOptions, "Radial", GRADIENT_RADIAL);
    AddMetadataElement(gradientOptions, "Conical", GRADIENT_CONICAL);
    AddMetadataElement(gradientOptions, "Spiral", GRADIENT_SPIRAL);

    filtreInterface->AddTreeInfos(libelleGradientType, new CTreeElementValueInt(param->gradientType), &gradientOptions, 3, TYPE_COMBOBOX);
}

void CGradientFilter::FilterChangeParam(CEffectParameter* effectParameter, CTreeElementValue* valueData, const wxString& key)
{
    auto param = static_cast<CGradientFilterParameter*>(effectParameter);

    if (key == libelleColorStart && valueData->GetType() == 4) {
        wxColour c = static_cast<CTreeElementValueColor*>(valueData)->GetValue();
        param->colorStart = cv::Scalar(c.Blue(), c.Green(), c.Red(), 255);
    }
    else if (key == libelleColorEnd && valueData->GetType() == 4) {
        wxColour c = static_cast<CTreeElementValueColor*>(valueData)->GetValue();
        param->colorEnd = cv::Scalar(c.Blue(), c.Green(), c.Red(), 255);
    }
    else if (key == libelleGradientType && valueData->GetType() == TYPE_ELEMENT_INT) {
        param->gradientType = static_cast<CTreeElementValueInt*>(valueData)->GetValue();
    }
}

// Génère la texture mathématique du dégradé pixel par pixel sur la totalité du canvas
void CGradientFilter::GenerateGradientMap(cv::Mat& outMat, const cv::Point& p1, const cv::Point& p2, const cv::Scalar& c1, const cv::Scalar& c2, int type)
{
    int width = outMat.cols;
    int height = outMat.rows;

    // Vecteur directeur AB
    double dx = p2.x - p1.x;
    double dy = p2.y - p1.y;
    double lenSq = dx * dx + dy * dy;
    if (lenSq < 1e-4) lenSq = 1.0; // Évite la division par zéro si cliqués au même endroit
    double len = sqrt(lenSq);

#pragma omp parallel for collapse(2) // Optimisation multithreading si OpenMP disponible
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            double t = 0.0; // Facteur d'interpolation entre 0.0 et 1.0

            double px = x - p1.x;
            double py = y - p1.y;

            switch (type) {
            case GRADIENT_LINEAR: {
                // Projection orthogonale du pixel sur le segment
                t = (px * dx + py * dy) / lenSq;
                break;
            }
            case GRADIENT_REFLECTED: {
                t = (px * dx + py * dy) / lenSq;
                t = abs(t); // Symétrie miroir par rapport à l'origine
                break;
            }
            case GRADIENT_DIAMOND: {
                // Calcul de distance de Manhattan projetée
                double projX = abs(px * dx + py * dy) / len;
                double projY = abs(px * (-dy) + py * dx) / len;
                t = (projX + projY) / len;
                break;
            }
            case GRADIENT_RADIAL: {
                // Distance euclidienne pure depuis le centre p1
                double dist = sqrt(px * px + py * py);
                t = dist / len;
                break;
            }
            case GRADIENT_CONICAL: {
                // Interpolation angulaire (0 à 2PI)
                double angle = atan2(py, px) - atan2(dy, dx);
                if (angle < 0) angle += 2.0 * M_PI;
                t = angle / (2.0 * M_PI);
                break;
            }
            case GRADIENT_SPIRAL: {
                double dist = sqrt(px * px + py * py);
                double angle = atan2(py, px) - atan2(dy, dx);
                if (angle < 0) angle += 2.0 * M_PI;
                // Mix linéaire entre la distance et l'angle pour créer la spirale
                t = fmod((dist / len) + (angle / (2.0 * M_PI)), 1.0);
                break;
            }
            }

            // Clamping de sécurité pour borner le dégradé entre 0.0 et 1.0
            t = std::max(0.0, std::min(1.0, t));

            // Interpolation linéaire des canaux de couleurs (Linear Blending)
            cv::Vec4b& pixel = outMat.at<cv::Vec4b>(y, x);
            pixel[0] = static_cast<uchar>(c1[0] * (1.0 - t) + c2[0] * t); // Blue
            pixel[1] = static_cast<uchar>(c1[1] * (1.0 - t) + c2[1] * t); // Green
            pixel[2] = static_cast<uchar>(c1[2] * (1.0 - t) + c2[2] * t); // Red
            pixel[3] = 255;                                               // Alpha Alpha-opaque
        }
    }
}

void CGradientFilter::Drawing(cv::Mat& matrix, IBitmapDisplay* bitmapViewer, CDraw* m_cDessin)
{
    if (matrix.empty() || m_cDessin == nullptr || bitmapViewer == nullptr) return;

    int hpos = bitmapViewer->GetHPos();
    int vpos = bitmapViewer->GetVPos();
    float ratio = bitmapViewer->GetRatio();

    if (matrix.channels() == 3)
        cv::cvtColor(matrix, matrix, cv::COLOR_BGR2BGRA);

    // 1. Demande à CGradientDraw de tracer le trait guide à l'écran
    m_cDessin->DessinerSurMat(matrix, hpos, vpos, ratio);

    auto gradientDraw = static_cast<Regards::FiltreEffet::CGradientDraw*>(m_cDessin);
    auto param = static_cast<CGradientFilterParameter*>(bitmapViewer->GetEffectPointer());

    if (!gradientDraw->GetIsDrawing() && gradientDraw->GetIsInit())
    {
        int screenX = static_cast<int>(gradientDraw->XDrawingPosition(static_cast<float>(param->ptStart.x), hpos, ratio));
        int screenY = static_cast<int>(gradientDraw->YDrawingPosition(static_cast<float>(param->ptStart.y), vpos, ratio));

		cv::Point ptStart = cv::Point(screenX, screenY);

        screenX = static_cast<int>(gradientDraw->XDrawingPosition(static_cast<float>(param->ptEnd.x), hpos, ratio));
        screenY = static_cast<int>(gradientDraw->YDrawingPosition(static_cast<float>(param->ptEnd.y), vpos, ratio));

        cv::Point ptEnd = cv::Point(screenX, screenY);


        GenerateGradientMap(matrix, ptStart, ptEnd, param->colorStart, param->colorEnd, param->gradientType);
    }
    else
    {
       

        if (param != nullptr && gradientDraw != nullptr)
        {

            // 2. Met à jour en temps réel les coordonnées réelles de l'effet
            param->ptStart = cv::Point(gradientDraw->GetStartPoint().x, gradientDraw->GetStartPoint().y);
            param->ptEnd = cv::Point(gradientDraw->GetEndPoint().x, gradientDraw->GetEndPoint().y);


        }
    }

}

CImageLoadingFormat* CGradientFilter::ApplyEffect(CEffectParameter* effectParameter, IBitmapDisplay* bitmapViewer)
{
    auto param = static_cast<CGradientFilterParameter*>(effectParameter);
    if (source.empty()) return nullptr;

    CImageLoadingFormat* imageLoad = new CImageLoadingFormat();
    imageLoad->SetPicture(source);
    imageLoad->RotateExif(orientation);

    cv::Mat& matrix = imageLoad->GetMatImage();
    if (matrix.channels() == 3)
        cv::cvtColor(matrix, matrix, cv::COLOR_BGR2BGRA);

    // Applique le dégradé sur l'entièreté de la matrice
    GenerateGradientMap(matrix, param->ptStart, param->ptEnd, param->colorStart, param->colorEnd, param->gradientType);

    param->apply = true;
    return imageLoad;
}

void CGradientFilter::RenderEffect(CFiltreEffet* filtreEffet, CEffectParameter* effectParameter, const bool& preview)
{
    auto param = static_cast<CGradientFilterParameter*>(effectParameter);
    if (!param->apply) return;

    CImageLoadingFormat* imageLoad = new CImageLoadingFormat();
    cv::Mat picture = filtreEffet->GetBitmap(true);
    imageLoad->SetPicture(picture);
    imageLoad->RotateExif(orientation);

    cv::Mat& matrix = imageLoad->GetMatImage();
    if (matrix.channels() == 3)
        cv::cvtColor(matrix, matrix, cv::COLOR_BGR2BGRA);

    GenerateGradientMap(matrix, param->ptStart, param->ptEnd, param->colorStart, param->colorEnd, param->gradientType);

    filtreEffet->SetBitmap(imageLoad);
}
