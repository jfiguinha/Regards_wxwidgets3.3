#include <header.h>
#include "PaintBucketFilter.h"
#include "PaintBucketFilterParameter.h"
#include "PaintBucketDraw.h"
#include <LibResource.h>
#include <FiltreEffet.h>
#include <ImageLoadingFormat.h>
#include <BitmapDisplay.h>
#include <Metadata.h>
#include <treetypeid.h>
#include <opencv2/imgproc.hpp>

using namespace Regards::Filter;
using namespace cv;
using namespace std;

CPaintBucketFilter::CPaintBucketFilter()
{
    libelleColor = CLibResource::LoadStringFromResource("LBLCOLOR", 1);
    libelleTolerance = CLibResource::LoadStringFromResource("LBLTOLERANCE", 1);
}

CPaintBucketFilter::~CPaintBucketFilter() {}

int CPaintBucketFilter::GetTypeFilter() { return IDM_PAINTBUCKETFILTER; }
int CPaintBucketFilter::GetNameFilter() { return IDM_PAINTBUCKETFILTER; }
wxString CPaintBucketFilter::GetFilterLabel() { return CLibResource::LoadStringFromResource("LBLPAINTBUCKETFILTER", 1);
}

CEffectParameter* CPaintBucketFilter::GetEffectPointer() { return new CPaintBucketFilterParameter(); }
CDraw* CPaintBucketFilter::GetDrawingPt() { return new Regards::FiltreEffet::CPaintBucketDraw(); }

void CPaintBucketFilter::Filter(CEffectParameter* effectParameter, cv::Mat& source, const wxString& filename, IFiltreEffectInterface* filtreInterface)
{
    this->source = source;
    this->filename = filename;
    auto param = static_cast<CPaintBucketFilterParameter*>(effectParameter);

    vector<int> elementDummy;
    filtreInterface->AddTreeInfos(libelleColor, new CTreeElementValueColor(param->GetWxFillColor()), &elementDummy, TYPE_COLOR, TYPE_COLOR);

    vector<int> elementTolerance;
    for (auto i = 0; i <= 255; i++)
        elementTolerance.push_back(i);

    filtreInterface->AddTreeInfos(libelleTolerance, new CTreeElementValueInt(param->tolerance), &elementTolerance);
}

void CPaintBucketFilter::FilterChangeParam(CEffectParameter* effectParameter, CTreeElementValue* valueData, const wxString& key)
{
    auto param = static_cast<CPaintBucketFilterParameter*>(effectParameter);

    if (key == libelleColor && valueData->GetType() == 4) {
        wxColour c = static_cast<CTreeElementValueColor*>(valueData)->GetValue();
        param->fillColor = cv::Scalar(c.Blue(), c.Green(), c.Red(), 255);
    }
    else if (key == libelleTolerance && valueData->GetType() == TYPE_ELEMENT_INT) {
        param->tolerance = static_cast<CTreeElementValueInt*>(valueData)->GetValue();
    }
}

void CPaintBucketFilter::Drawing(cv::Mat& matrix, IBitmapDisplay* bitmapViewer, CDraw* m_cDessin)
{
    if (matrix.empty() || m_cDessin == nullptr || bitmapViewer == nullptr) return;

    auto paintDraw = static_cast<Regards::FiltreEffet::CPaintBucketDraw*>(m_cDessin);
    auto param = static_cast<CPaintBucketFilterParameter*>(bitmapViewer->GetEffectPointer());

    int hpos = bitmapViewer->GetHPos();
    int vpos = bitmapViewer->GetVPos();
    float ratio = bitmapViewer->GetRatio();

    if (param != nullptr && paintDraw != nullptr)
    {
        if (paintDraw->HasClicked())
        {
            // 1. Création d'un nouvel enregistrement d'action avec les paramètres courants du clic
            SPaintBucketAction nouvelleAction;
            nouvelleAction.ptClick = cv::Point(paintDraw->GetClickPoint().x, paintDraw->GetClickPoint().y);
            nouvelleAction.fillColor = param->fillColor;
            nouvelleAction.tolerance = param->tolerance;

            // 2. Empilage de la zone dans le vecteur de sauvegarde
            param->actionsHistory.push_back(nouvelleAction);
        }
        // 3. Redessine TOUTES les zones sauvegardées séquentiellement sur l'image source d'origine
        for (const auto& action : param->actionsHistory)
        {
            int screenX = static_cast<int>(paintDraw->XDrawingPosition(static_cast<float>(action.ptClick.x), hpos, ratio));
            int screenY = static_cast<int>(paintDraw->YDrawingPosition(static_cast<float>(action.ptClick.y), vpos, ratio));

            cv::Point ptClick = cv::Point(screenX, screenY);

            if (screenX >= 0 && screenX < matrix.cols && screenY >= 0 && screenY < matrix.rows)
            {
                cv::Scalar diff(action.tolerance, action.tolerance, action.tolerance, action.tolerance);
                cv::Mat mask = cv::Mat::zeros(matrix.rows + 2, matrix.cols + 2, CV_8UC1);
                cv::floodFill(matrix, mask, ptClick, action.fillColor, nullptr, diff, diff, 4 | cv::FLOODFILL_FIXED_RANGE);
            }
        }

        paintDraw->ResetClick();
        param->apply = true;
    }
}

CImageLoadingFormat* CPaintBucketFilter::ApplyEffect(CEffectParameter* effectParameter, IBitmapDisplay* bitmapViewer)
{
    auto param = static_cast<CPaintBucketFilterParameter*>(effectParameter);
    if (source.empty()) return nullptr;

    CImageLoadingFormat* imageLoad = new CImageLoadingFormat();
    imageLoad->SetPicture(source);
    imageLoad->RotateExif(orientation);

    cv::Mat& matrix = imageLoad->GetMatImage();

    // 3. Redessine TOUTES les zones sauvegardées séquentiellement sur l'image source d'origine
    for (const auto& action : param->actionsHistory)
    {
        if (action.ptClick.x >= 0 && action.ptClick.x < matrix.cols && action.ptClick.y >= 0 && action.ptClick.y < matrix.rows)
        {
            cv::Scalar diff(action.tolerance, action.tolerance, action.tolerance, action.tolerance);
            cv::Mat mask = cv::Mat::zeros(matrix.rows + 2, matrix.cols + 2, CV_8UC1);
            cv::floodFill(matrix, mask, action.ptClick, action.fillColor, nullptr, diff, diff, 4 | cv::FLOODFILL_FIXED_RANGE);
        }
    }

    param->apply = true;
    return imageLoad;
}

void CPaintBucketFilter::RenderEffect(CFiltreEffet* filtreEffet, CEffectParameter* effectParameter, const bool& preview)
{
    auto param = static_cast<CPaintBucketFilterParameter*>(effectParameter);
    if (!param->apply) return;

    CImageLoadingFormat* imageLoad = new CImageLoadingFormat();
    cv::Mat picture = filtreEffet->GetBitmap(true);
    imageLoad->SetPicture(picture);
    imageLoad->RotateExif(orientation);

    cv::Mat& matrix = imageLoad->GetMatImage();

    // Même processus de ré-application en cascade pour le moteur de rendu de sortie
    for (const auto& action : param->actionsHistory)
    {
        if (action.ptClick.x >= 0 && action.ptClick.x < matrix.cols && action.ptClick.y >= 0 && action.ptClick.y < matrix.rows)
        {
            cv::Scalar diff(action.tolerance, action.tolerance, action.tolerance, action.tolerance);
            cv::Mat mask = cv::Mat::zeros(matrix.rows + 2, matrix.cols + 2, CV_8UC1);
            cv::floodFill(matrix, mask, action.ptClick, action.fillColor, nullptr, diff, diff, 4 | cv::FLOODFILL_FIXED_RANGE);
        }
    }

    filtreEffet->SetBitmap(imageLoad);
}
