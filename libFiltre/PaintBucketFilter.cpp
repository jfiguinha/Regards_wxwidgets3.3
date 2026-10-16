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

    vector<int> elementTolerance;
    for (auto i = 0; i <= 255; i++)
        elementTolerance.push_back(i);

    filtreInterface->AddTreeInfos(libelleTolerance, new CTreeElementValueInt(param->tolerance), &elementTolerance);
}

void CPaintBucketFilter::FilterChangeParam(CEffectParameter* effectParameter, CTreeElementValue* valueData, const wxString& key)
{
    auto param = static_cast<CPaintBucketFilterParameter*>(effectParameter);

    if (key == libelleTolerance && valueData->GetType() == TYPE_ELEMENT_INT) {
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
            SPaintBucketAction nouvelleAction;
            nouvelleAction.ptClick = cv::Point(paintDraw->GetClickPoint().x, paintDraw->GetClickPoint().y);
            nouvelleAction.fillColor = param->color1;
            nouvelleAction.tolerance = param->tolerance;
            param->actionsHistory.push_back(nouvelleAction);
        }

        for (const auto& action : param->actionsHistory)
        {
            int screenX = static_cast<int>(paintDraw->XDrawingPosition(static_cast<float>(action.ptClick.x), hpos, ratio));
            int screenY = static_cast<int>(paintDraw->YDrawingPosition(static_cast<float>(action.ptClick.y), vpos, ratio));

            cv::Point ptClick = cv::Point(screenX, screenY);

            if (screenX >= 0 && screenX < matrix.cols && screenY >= 0 && screenY < matrix.rows)
            {
                try
                {
                    cv::Scalar diff(action.tolerance, action.tolerance, action.tolerance, action.tolerance);
                    cv::Mat mask = cv::Mat::zeros(matrix.rows + 2, matrix.cols + 2, CV_8UC1);

                    if (matrix.channels() == 4)
                    {
                        // 1. Isoler la couleur (BGR) et l'Alpha (A)
                        cv::Mat bgr(matrix.size(), CV_8UC3);
                        cv::Mat alpha(matrix.size(), CV_8UC1);

                        cv::Mat out[] = { bgr, alpha };
                        int from_to[] = { 0,0, 1,1, 2,2, 3,3 }; // B->B, G->G, R->R, A->A
                        cv::mixChannels(&matrix, 1, out, 2, from_to, 4);

                        // 2. Extraire la couleur de remplissage en BGR uniquement (3 canaux)
                        cv::Scalar fillColorBGR(action.fillColor[0], action.fillColor[1], action.fillColor[2]);

                        // 3. Appliquer le floodFill sur les 3 canaux
                        cv::floodFill(bgr, mask, ptClick, fillColorBGR, nullptr, diff, diff, 4 | cv::FLOODFILL_FIXED_RANGE);

                        // 4. Recombiner le BGR modifié avec l'Alpha d'origine
                        cv::Mat in[] = { bgr, alpha };
                        cv::mixChannels(in, 2, &matrix, 1, from_to, 4);
                    }
                    else
                    {
                        // Comportement standard pour 1 et 3 canaux
                        cv::floodFill(matrix, mask, ptClick, action.fillColor, nullptr, diff, diff, 4 | cv::FLOODFILL_FIXED_RANGE);
                    }
                }
                catch (cv::Exception& e)
                {
                    const char* err_msg = e.what();
                    std::cout << "PaintBucket exception caught: " << err_msg << std::endl;
                }
            }
        }

        paintDraw->ResetClick();
        param->apply = true;
    }
}

void CPaintBucketFilter::Drawing(cv::UMat& matrix, IBitmapDisplay* bitmapViewer, CDraw* m_cDessin)
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
            nouvelleAction.fillColor = param->color1;
            nouvelleAction.tolerance = param->tolerance;

            // 2. Empilage de la zone dans le vecteur de sauvegarde
            param->actionsHistory.push_back(nouvelleAction);
        }

        // On ouvre un scope/bloc de mémoire pour forcer la libération et la synchronisation de la Mat CPU vers l'UMat GPU à la fin
        {
            // Récupération d'une matrice CPU synchronisée en lecture/écriture
            cv::Mat matCPU = matrix.getMat(cv::ACCESS_RW);

            // 3. Redessine TOUTES les zones sauvegardées séquentiellement
            for (const auto& action : param->actionsHistory)
            {
                int screenX = static_cast<int>(paintDraw->XDrawingPosition(static_cast<float>(action.ptClick.x), hpos, ratio));
                int screenY = static_cast<int>(paintDraw->YDrawingPosition(static_cast<float>(action.ptClick.y), vpos, ratio));

                cv::Point ptClick = cv::Point(screenX, screenY);

                if (screenX >= 0 && screenX < matCPU.cols && screenY >= 0 && screenY < matCPU.rows)
                {
                    try
                    {
                        cv::Scalar diff(action.tolerance, action.tolerance, action.tolerance, action.tolerance);
                        cv::Mat mask = cv::Mat::zeros(matCPU.rows + 2, matCPU.cols + 2, CV_8UC1);

                        if (matCPU.channels() == 4)
                        {
                            // 1. Isoler la couleur (BGR) et l'Alpha (A) sur l'image CPU
                            cv::Mat bgr(matCPU.size(), CV_8UC3);
                            cv::Mat alpha(matCPU.size(), CV_8UC1);

                            cv::Mat out[] = { bgr, alpha };
                            int from_to[] = { 0,0, 1,1, 2,2, 3,3 }; // B->B, G->G, R->R, A->A
                            cv::mixChannels(&matCPU, 1, out, 2, from_to, 4);

                            // 2. Extraire la couleur de remplissage en BGR (3 canaux)
                            cv::Scalar fillColorBGR(action.fillColor[0], action.fillColor[1], action.fillColor[2]);

                            // 3. Appliquer le floodFill sur les 3 canaux
                            cv::floodFill(bgr, mask, ptClick, fillColorBGR, nullptr, diff, diff, 4 | cv::FLOODFILL_FIXED_RANGE);

                            // 4. Recombiner le BGR modifié avec l'Alpha d'origine
                            cv::Mat in[] = { bgr, alpha };
                            cv::mixChannels(in, 2, &matCPU, 1, from_to, 4);
                        }
                        else
                        {
                            // Comportement standard pour 1 et 3 canaux
                            cv::floodFill(matCPU, mask, ptClick, action.fillColor, nullptr, diff, diff, 4 | cv::FLOODFILL_FIXED_RANGE);
                        }
                    }
                    catch (cv::Exception& e)
                    {
                        const char* err_msg = e.what();
                        std::cout << "PaintBucket UMat exception caught: " << err_msg << std::endl;
                    }
                }
            }
        } // Fin du bloc : l'objet `matCPU` est détruit, ce qui valide et renvoie les modifications vers la mémoire GPU de `matrix`

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

    if (matrix.empty()) return imageLoad;
    if (matrix.channels() == 4) cv::cvtColor(matrix, matrix, cv::COLOR_BGRA2BGR);

    // 3. Redessine TOUTES les zones sauvegardées séquentiellement sur l'image source d'origine
    for (const auto& action : param->actionsHistory)
    {
        if (action.ptClick.x >= 0 && action.ptClick.x < matrix.cols && action.ptClick.y >= 0 && action.ptClick.y < matrix.rows)
        {
            try
            {
                cv::Scalar diff(action.tolerance, action.tolerance, action.tolerance, action.tolerance);
                cv::Mat mask = cv::Mat::zeros(matrix.rows + 2, matrix.cols + 2, CV_8UC1);
                cv::floodFill(matrix, mask, action.ptClick, action.fillColor, nullptr, diff, diff, 4 | cv::FLOODFILL_FIXED_RANGE);
            }
            catch (cv::Exception& e)
            {
                const char* err_msg = e.what();
                std::cout << "BilateralEffect exception caught: " << err_msg << std::endl;
                std::cout << "wrong file format, please input the name of an IMAGE file" << std::endl;
            }
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

	int nbChannels = matrix.channels();

    if (matrix.empty()) return;
    if (nbChannels == 4) cv::cvtColor(matrix, matrix, cv::COLOR_BGRA2BGR);

    // Même processus de ré-application en cascade pour le moteur de rendu de sortie
    for (const auto& action : param->actionsHistory)
    {
        if (action.ptClick.x >= 0 && action.ptClick.x < matrix.cols && action.ptClick.y >= 0 && action.ptClick.y < matrix.rows)
        {
            try
            {
                cv::Scalar diff(action.tolerance, action.tolerance, action.tolerance, action.tolerance);
                cv::Mat mask = cv::Mat::zeros(matrix.rows + 2, matrix.cols + 2, CV_8UC1);
                cv::floodFill(matrix, mask, action.ptClick, action.fillColor, nullptr, diff, diff, 4 | cv::FLOODFILL_FIXED_RANGE);
            }
            catch (cv::Exception& e)
            {
                const char* err_msg = e.what();
                std::cout << "BilateralEffect exception caught: " << err_msg << std::endl;
                std::cout << "wrong file format, please input the name of an IMAGE file" << std::endl;
            }
        }
    }

    if (nbChannels == 4) cv::cvtColor(matrix, matrix, cv::COLOR_BGR2BGRA);
    filtreEffet->SetBitmap(imageLoad);
}
