// ==================== BitmapEditor.cpp ====================
#include <header.h>
#include "BitmapEditor.h"
#include <Draw.h>
#include <FiltreEffet.h>
#include <GLTexture.h>
#include <ImageLoadingFormat.h>
#include <LayerFonction.h>
#include "LayerRendererCPU.h"
#include "LayerRendererGL.h"
#include "LayerRendererOpenCL.h"
#include <ParamInit.h>
#include <RGBAQuad.h>
#include <RegardsConfigParam.h>
#include <ViewerTheme.h>
#include <ViewerThemeInit.h>
#include <libPicture.h>
#include <ClosedHandCursor.h>
#include <window_id.h>
#include <SqlPhotos.h>
#include <appcontext.h>
#include <SelectFilterParameter.h>
#include <FilterData.h>
#include <wx/clipbrd.h>
#include <wx/bitmap.h>
#include <wx/image.h>
#include <HistoryState.h>
#include <LibResource.h>
using namespace Regards::Sqlite;
using namespace Regards::Viewer;

extern AppContext application_context;

//-----------------------------------------------------------------
// Utilitaires locaux (partagés par les backends GL / OpenCL / CPU et les
// outils de sélection)
//-----------------------------------------------------------------
namespace {
    // Couleur des tracés de sélection (blanc opaque)
    const cv::Scalar kSelectionColor(0, 0, 0, 255);

    void LogFallbackOnce(bool& alreadyLogged, const char* message) {
        if (!alreadyLogged) {
            wxLogDebug("%s", message);
            alreadyLogged = true;
        }
    }

    // Copie 'src' dans 'dst' à la position (x, y), recadrée sur l'intersection
    // avec 'dst'. Retourne false si l'intersection est vide.
    bool CopyIntoROI(const cv::Mat& src, cv::Mat& dst, int x, int y) {
        const cv::Rect dstROI = cv::Rect(x, y, src.cols, src.rows) &
            cv::Rect(0, 0, dst.cols, dst.rows);
        if (dstROI.empty()) return false;
        const cv::Rect srcROI(dstROI.x - x, dstROI.y - y, dstROI.width,
            dstROI.height);
        src(srcROI).copyTo(dst(dstROI));
        return true;
    }

    // Conversion en 4 canaux BGRA (le canal alpha porte la transparence).
    cv::Mat ToBGRA(const cv::Mat& source) {
        cv::Mat bgra;
        if (source.channels() == 3)
            cv::cvtColor(source, bgra, cv::COLOR_BGR2BGRA);
        else if (source.channels() == 1)
            cv::cvtColor(source, bgra, cv::COLOR_GRAY2BGRA);
        else
            bgra = source.clone();
        return bgra;
    }

    // Place l'image sur un nouveau canvas (noir/transparent) selon l'ancre choisie,
    // avec recadrage si l'image dépasse du canvas.
    cv::Mat PlaceOnCanvas(const cv::Mat& oldImage,
        const CanvasSizeParameter& canvasSize) {
        const int newWidth = canvasSize.width;
        const int newHeight = canvasSize.height;
        cv::Mat newCanvas = cv::Mat::zeros(newHeight, newWidth, oldImage.type());

        int dstX = 0;
        int dstY = 0;

        switch (canvasSize.position) {
        case CanvasSizeParameter::Position::TopLeft:
            break;

        case CanvasSizeParameter::Position::Center:
            dstX = (newWidth - oldImage.cols) / 2;
            dstY = (newHeight - oldImage.rows) / 2;
            break;

        case CanvasSizeParameter::Position::TopRight:
            dstX = newWidth - oldImage.cols;
            break;

        case CanvasSizeParameter::Position::BottomLeft:
            dstY = newHeight - oldImage.rows;
            break;

        case CanvasSizeParameter::Position::BottomRight:
            dstX = newWidth - oldImage.cols;
            dstY = newHeight - oldImage.rows;
            break;
        }

        CopyIntoROI(oldImage, newCanvas, dstX, dstY);
        return newCanvas;
    }

    // --- Sélections ------------------------------------------------

    // Masque (255 = sélectionné) des sélections, utilisé pour l'inversion.
    template <typename Selections>
    cv::Mat BuildInversionMask(const Selections& selections, int width, int height) {
        cv::Mat mask = cv::Mat::zeros(height, width, CV_8UC1);

        for (const auto& selection : selections)
        {
            switch (selection.selectType)
            {
            case SELECT_RECTANGLE:
            {
                const cv::Point p1(selection.startPoint.x, selection.startPoint.y);
                const cv::Point p2(selection.endPoint.x, selection.endPoint.y);
                cv::rectangle(mask, p1, p2, cv::Scalar(255), cv::FILLED);
                break;
            }

            case SELECT_ELLIPSE:
            {
                const int left = std::min(selection.startPoint.x, selection.endPoint.x);
                const int right = std::max(selection.startPoint.x, selection.endPoint.x);
                const int top = std::min(selection.startPoint.y, selection.endPoint.y);
                const int bottom = std::max(selection.startPoint.y, selection.endPoint.y);

                const cv::Point center(left + (right - left) / 2,
                    top + (bottom - top) / 2);
                const cv::Size axes((right - left) / 2, (bottom - top) / 2);

                if (axes.width > 0 && axes.height > 0)
                    cv::ellipse(mask, center, axes, 0, 0, 360, cv::Scalar(255),
                        cv::FILLED);
                break;
            }

            case SELECT_LASSO:
            {
                if (selection.points.size() < 3)
                    break;

                std::vector<cv::Point> points;
                points.reserve(selection.points.size());
                for (const auto& point : selection.points)
                    points.emplace_back(point.x, point.y);

                cv::fillPoly(mask, std::vector<std::vector<cv::Point>>{ points },
                    cv::Scalar(255));
                break;
            }

            default:
                break;
            }
        }
        return mask;
    }

    // Convertit les contours d'un masque (y compris les contours intérieurs) en
    // tracés LASSO fermés.
    std::vector<SSelectionTrace> MaskToLassoSelections(const cv::Mat& mask) {
        std::vector<std::vector<cv::Point>> contours;
        std::vector<cv::Vec4i> hierarchy;
        cv::findContours(mask, contours, hierarchy, cv::RETR_TREE,
            cv::CHAIN_APPROX_SIMPLE);

        std::vector<SSelectionTrace> result;
        result.reserve(contours.size());

        for (const auto& contour : contours)
        {
            if (contour.size() < 3)
                continue;

            SSelectionTrace trace;
            trace.selectType = SELECT_LASSO;
            trace.penSize = 1;
            trace.color = kSelectionColor;
            trace.startPoint = wxPoint(contour[0].x, contour[0].y);
            trace.endPoint = trace.startPoint;

            trace.points.reserve(contour.size() + 1);
            for (const auto& point : contour)
                trace.points.emplace_back(point.x, point.y);

            // Fermer explicitement le contour.
            if (trace.points.front() != trace.points.back())
                trace.points.push_back(trace.points.front());

            result.push_back(std::move(trace));
        }
        return result;
    }

    // Boîte englobante de toutes les sélections, bornée à l'image.
    // Largeur/hauteur <= 0 si rien d'exploitable.
    template <typename Selections>
    cv::Rect SelectionBounds(const Selections& selections, const cv::Size& imageSize) {
        int minX = imageSize.width, minY = imageSize.height;
        int maxX = 0, maxY = 0;

        for (const auto& s : selections)
        {
            if (s.selectType == SELECT_RECTANGLE || s.selectType == SELECT_ELLIPSE)
            {
                minX = std::min({ minX, s.startPoint.x, s.endPoint.x });
                minY = std::min({ minY, s.startPoint.y, s.endPoint.y });
                maxX = std::max({ maxX, s.startPoint.x, s.endPoint.x });
                maxY = std::max({ maxY, s.startPoint.y, s.endPoint.y });
            }
            else if (s.selectType == SELECT_LASSO)
            {
                for (const auto& pt : s.points)
                {
                    minX = std::min(minX, pt.x);
                    minY = std::min(minY, pt.y);
                    maxX = std::max(maxX, pt.x);
                    maxY = std::max(maxY, pt.y);
                }
            }
        }

        minX = std::max(0, minX);
        minY = std::max(0, minY);
        maxX = std::min(imageSize.width, maxX);
        maxY = std::min(imageSize.height, maxY);
        return cv::Rect(minX, minY, maxX - minX, maxY - minY);
    }

    // Masque (255 = sélectionné) utilisé pour le détourage lors d'une copie.
    template <typename Selections>
    cv::Mat BuildCopyMask(const Selections& selections, const cv::Size& imageSize) {
        cv::Mat mask = cv::Mat::zeros(imageSize, CV_8UC1);

        for (const auto& s : selections)
        {
            const cv::Point p1(s.startPoint.x, s.startPoint.y);
            const cv::Point p2(s.endPoint.x, s.endPoint.y);

            if (s.selectType == SELECT_RECTANGLE)
            {
                cv::rectangle(mask, p1, p2, cv::Scalar(255), cv::FILLED);
            }
            else if (s.selectType == SELECT_ELLIPSE)
            {
                const cv::Point center((p1.x + p2.x) / 2, (p1.y + p2.y) / 2);
                const cv::Size axes(std::abs(p1.x - p2.x) / 2, std::abs(p1.y - p2.y) / 2);
                cv::ellipse(mask, center, axes, 0, 0, 360, cv::Scalar(255), cv::FILLED);
            }
            else if (s.selectType == SELECT_LASSO && !s.points.empty())
            {
                std::vector<cv::Point> cvPoints;
                cvPoints.reserve(s.points.size());
                for (const auto& pt : s.points)
                    cvPoints.emplace_back(pt.x, pt.y);
                cv::fillPoly(mask, std::vector<std::vector<cv::Point>>{ cvPoints },
                    cv::Scalar(255));
            }
        }
        return mask;
    }

    // Pixels hors masque -> alpha = 0 (transparent). 'bgra' doit être en 4 canaux.
    void ApplyMaskToAlpha(cv::Mat& bgra, const cv::Mat& mask) {
        std::vector<cv::Mat> channels;
        cv::split(bgra, channels);
        cv::bitwise_and(channels[3], mask, channels[3]);
        cv::merge(channels, bgra);
    }
}  // namespace



CSelectFilterParameter* CBitmapEditor::GetSelectFilter() {
    if (effectParameter == nullptr || preview != IDM_RECTANGLESELECT || preview != IDM_ELLIPSESELECT || preview != IDM_LASSOSELECT)
        return nullptr;
    return static_cast<CSelectFilterParameter*>(effectParameter);
}

// Outil de sélection actif + calque actif avec une image de taille valide.
bool CBitmapEditor::GetActiveSelectionContext(CSelectFilterParameter*& filter,
    int& imgW, int& imgH) {
    filter = GetSelectFilter();
    if (filter == nullptr) return false;

    LayerElement* layer = GetActiveLayerElement();
    if (layer == nullptr || layer->GetPicture() == nullptr) return false;

    imgW = layer->GetPicture()->GetWidth();
    imgH = layer->GetPicture()->GetHeight();
    return imgW > 0 && imgH > 0;
}

void CBitmapEditor::RefreshSelectionView() {
    m_cDessin->SetUpdateTrace(true);
    needToRefresh = true;
    if (parentRender) parentRender->Refresh();
}

// Prévient l'UI globale que l'image d'un calque a changé.
void CBitmapEditor::NotifyLayerPictureUpdated() {
    if (parentRender == nullptr) return;
    wxWindow* mainWindow = parentRender->FindWindowById(FRAMEEDITOR_ID);
    if (mainWindow) {
        wxCommandEvent evt(wxEVENT_UPDATELAYERPICTURE);
        mainWindow->GetEventHandler()->AddPendingEvent(evt);
    }
}

//-----------------------------------------------------------------
// Passe un calque dans le pipeline d'écran du filtre (Resize / Rotation / Flip).
// Résultat disponible via filter->GetMatrix(). false si le calque n'a pas
// d'image valide.
//-----------------------------------------------------------------
bool CBitmapEditor::PrepareFilteredLayer(LayerElement* layer, CFiltreEffet* filter,
    int width, int height) {
    CImageLoadingFormat* picture = layer->GetPicture();
    if (picture == nullptr || !picture->IsOk()) return false;

    filter->SetBitmap(picture);
    int wOut = width;
    int hOut = height;
    GenerateScreenBitmap(filter, wOut, hOut);
    return true;
}

cv::Mat CBitmapEditor::PrepareLayerSource(LayerElement* layer, int width,
    int height, CFiltreEffet* filter) {
    if (!PrepareFilteredLayer(layer, filter, width, height)) return cv::Mat();

    Regards::Picture::CPictureArray transformPicture(filter->GetMatrix());
    return LayerFonction::PrepareLayerImage(transformPicture.getMat(), width, height, layer->x, layer->y, ratio);
}

cv::UMat CBitmapEditor::PrepareLayerSourceOpenCL(LayerElement* layer, int width,
    int height, CFiltreEffet* filter) {
    if (!PrepareFilteredLayer(layer, filter, width, height)) return cv::UMat();

    Regards::Picture::CPictureArray transformPicture = filter->GetMatrix();
    return LayerFonction::PrepareLayerImageOpenCL(transformPicture.getUMat(), width, height, layer->x, layer->y, ratio);
}

//-----------------------------------------------------------------
// SELECT ALL : sélectionne l'intégralité du calque actif
//-----------------------------------------------------------------
void CBitmapEditor::SelectAll()
{
    CSelectFilterParameter* filter = nullptr;
    int imgW = 0, imgH = 0;
    if (!GetActiveSelectionContext(filter, imgW, imgH))
        return;

    // Un tracé rectangulaire unique couvrant tout le calque remplace les
    // sélections précédentes.
    SSelectionTrace fullSelect;
    fullSelect.selectType = SELECT_RECTANGLE;
    fullSelect.startPoint = wxPoint(0, 0);
    fullSelect.endPoint = wxPoint(imgW, imgH);
    fullSelect.penSize = 1;
    fullSelect.color = kSelectionColor;

    filter->listSelections.clear();
    filter->listSelections.push_back(fullSelect);

    RefreshSelectionView();
}

//-----------------------------------------------------------------
// INVERSE ALL : inverse la sélection (contours du masque inversé -> tracés LASSO)
//-----------------------------------------------------------------
void CBitmapEditor::InverseAll()
{
    CSelectFilterParameter* filter = nullptr;
    int imgW = 0, imgH = 0;
    if (!GetActiveSelectionContext(filter, imgW, imgH))
        return;

    // Aucune sélection : tout sélectionner.
    if (filter->listSelections.empty())
    {
        SelectAll();
        return;
    }

    cv::Mat mask = BuildInversionMask(filter->listSelections, imgW, imgH);
    cv::bitwise_not(mask, mask);

    filter->listSelections = MaskToLassoSelections(mask);

    RefreshSelectionView();
}

//-----------------------------------------------------------------
// CANCEL SELECT : vide toutes les sélections en cours
//-----------------------------------------------------------------
void CBitmapEditor::CancelSelect()
{
    CSelectFilterParameter* filter = GetSelectFilter();
    if (filter == nullptr)
        return;

    filter->listSelections.clear();
    this->m_cDessin->SetUpdateTrace(true);
    needToRefresh = true;
    if (parentRender) parentRender->Refresh();
}

//-----------------------------------------------------------------
// COPY : copie l'image (ou la sélection détourée) dans le presse-papiers
//-----------------------------------------------------------------
bool CBitmapEditor::Copy()
{
    LayerElement* layerElement = GetActiveLayerElement();
    if (layerElement == nullptr)
        return false;

    CImageLoadingFormat* picture = layerElement->GetPicture();
    if (picture == nullptr || !picture->IsOk())
        return false;

    const cv::Mat sourceMat = picture->GetMatImage();
    if (sourceMat.empty())
        return false;

    cv::Mat outMat;  // Matrice finale envoyée au presse-papiers

    CSelectFilterParameter* filter = GetSelectFilter();
    if (filter != nullptr && !filter->listSelections.empty())
    {
        // Détermination de la boîte englobante globale de la sélection
        const cv::Rect roi = SelectionBounds(filter->listSelections, sourceMat.size());
        if (roi.width > 0 && roi.height > 0)
        {
            // Vérification du type du premier élément de sélection pour ajuster le comportement
            int selectType = filter->listSelections.front().selectType;

            if (selectType == SELECT_RECTANGLE)
            {
                // Pour le rectangle, extraction brute de la sous-zone sans gestion de canal alpha forcé
                outMat = sourceMat(roi).clone();
            }
            else if (selectType == SELECT_ELLIPSE || selectType == SELECT_LASSO)
            {
                // Pour l'ellipse et le lasso, on encadre et on applique la transparence à l'extérieur
                cv::Mat bgraMat = ToBGRA(sourceMat);
                ApplyMaskToAlpha(bgraMat, BuildCopyMask(filter->listSelections, sourceMat.size()));
                outMat = bgraMat(roi).clone();
            }
        }
    }

    // Aucune sélection active ou valide : copie de l'image entière
    if (outMat.empty())
        outMat = sourceMat.clone();

    const wxBitmap bitmap(Regards::Picture::CLibPicture::ConvertRegardsBitmapToWXImage(outMat));

    if (wxTheClipboard->Open())
    {
        wxTheClipboard->Clear();
        wxTheClipboard->SetData(new wxBitmapDataObject(bitmap));
        wxTheClipboard->Close();
        return true;
    }

    return false;
}

//-----------------------------------------------------------------
// DELETE : Efface le contenu de la sélection par de la transparence
//-----------------------------------------------------------------
bool CBitmapEditor::DeleteSelect()
{
    LayerElement* layerElement = GetActiveLayerElement();
    if (layerElement == nullptr)
        return false;

    CImageLoadingFormat* picture = layerElement->GetPicture();
    if (picture == nullptr || !picture->IsOk())
        return false;

    const cv::Mat sourceMat = picture->GetMatImage();
    if (sourceMat.empty())
        return false;

    CSelectFilterParameter* filter = GetSelectFilter();
    // S'il n'y a pas de sélection active, la fonction delete ne fait rien ou efface tout selon votre choix.
    // Ici, elle cible uniquement la sélection en cours.
    if (filter != nullptr && !filter->listSelections.empty())
    {
        SaveHistoryState(L"Delete Selection");

        // On convertit l'image en BGRA pour forcer le support du canal alpha (transparence)
        cv::Mat bgraMat = ToBGRA(sourceMat);

        // Construction du masque où 255 représente la zone sélectionnée à effacer
        cv::Mat mask = BuildCopyMask(filter->listSelections, bgraMat.size());

        // Mettre à zéro (transparent) le canal Alpha (index 3) là où le masque est actif (255)
        // Optionnel : Vous pouvez aussi nettoyer les canaux BGR en noir transparent (0, 0, 0, 0)
        bgraMat.setTo(cv::Scalar(0, 0, 0, 0), mask);

        // Assigner la nouvelle image modifiée au calque
        picture->SetPicture(bgraMat);

        // Invalidation des caches graphiques et notification de rafraîchissement
        layerElement->InvalidatePicture();
        layerElement->needTextureUpdate = true;
        layerElement->pictureVersion++;

        listOfLayer->IsChanged(true);

        widthOutputOld = -1;
        heightOutputOld = -1;
        loadBitmap = true;
        forceUpdateFilter = true;
        updateFilter = true;

        NotifyLayerPictureUpdated();
        RefreshSelectionView();

        return true;
    }

    return false;
}



// ============================================================================
// CBITMAPEDITOR : Implémentation du Recadrage (Crop) basé sur la sélection
// ============================================================================
void CBitmapEditor::Crop()
{
    // 1. Récupération du filtre de sélection actif
    CSelectFilterParameter* filter = GetSelectFilter();
    if (filter == nullptr || filter->listSelections.empty())
    {
        wxMessageBox(L"Veuillez effectuer une sélection avant de recadrer l'image.",
            L"Recadrage impossible", wxOK | wxICON_WARNING, parentRender);
        return;
    }

    // 2. Détermination de la taille de référence (taille globale actuelle du document)
    if (bitmapwidth <= 0 || bitmapheight <= 0)
        return;

    cv::Size globalSize(bitmapwidth, bitmapheight);

    // 3. Calcul de la boîte englobante (ROI) de toutes les sélections accumulées
    cv::Rect cropROI = SelectionBounds(filter->listSelections, globalSize);

    // Sécurité : Vérifier que la zone de recadrage est valide et non vide
    if (cropROI.width <= 0 || cropROI.height <= 0)
    {
        wxMessageBox(L"La zone de sélection est invalide pour un recadrage.",
            L"Erreur", wxOK | wxICON_ERROR, parentRender);
        return;
    }

    // 4. Sauvegarde de l'état actuel dans l'historique avant modification irréversible
    SaveHistoryState(L"Recadrage de l'image (Crop)");

    // 5. Parcourir et découper TOUS les calques du projet
    for (size_t i = 0; i < listOfLayer->size(); ++i)
    {
        LayerElement* layer = GetElement(i);
        if (!layer) continue;

        CImageLoadingFormat* layerPicture = layer->GetPicture();
        if (!layerPicture || !layerPicture->IsOk()) continue;

        cv::Mat sourceMat = layerPicture->GetMatImage();
        if (sourceMat.empty()) continue;

        // On s'assure de l'intersection entre le ROI global et la matrice réelle du calque
        cv::Rect layerROI = cropROI & cv::Rect(0, 0, sourceMat.cols, sourceMat.rows);

        cv::Mat croppedMat;
        if (!layerROI.empty())
        {
            // Extraction de la sous-zone sélectionnée
            croppedMat = sourceMat(layerROI).clone();
        }
        else
        {
            // Si le calque est complètement en dehors de la sélection, 
            // on crée une matrice transparente vide aux dimensions du recadrage
            croppedMat = cv::Mat::zeros(cropROI.height, cropROI.width, sourceMat.type());
            if (croppedMat.channels() == 4)
            {
                croppedMat.setTo(cv::Scalar(0, 0, 0, 0));
            }
        }

        // Réassignation de la nouvelle image découpée au calque
        layerPicture->SetPicture(croppedMat);

        // Réinitialisation des coordonnées locales du calque (le calque est recalé à l'origine 0,0)
        layer->x = 0;
        layer->y = 0;
        layer->textureWidth = cropROI.width;
        layer->textureHeight = cropROI.height;

        // Invalidation obligatoire des caches graphiques / textures GPU
        layer->InvalidatePicture();
        layer->needTextureUpdate = true;
        layer->pictureVersion++;
    }

    // 6. Mise à jour des dimensions du Canvas global de la liste
    listOfLayer->UpdateCanvasSize(cropROI.width, cropROI.height);
    listOfLayer->IsChanged(true);

    // 7. Vider la sélection active puisqu'elle a été consommée par le Crop
    filter->listSelections.clear();

    // 8. Forcer la mise à jour des métadonnées du Viewer (`bitmapwidth` et `bitmapheight`)
    if (!UpdateBitmapSizeFromLayers())
    {
        bitmapwidth = cropROI.width;
        bitmapheight = cropROI.height;
    }

    // 9. Forcer la réinitialisation complète des filtres et du pipeline d'affichage
    widthOutputOld = -1;
    heightOutputOld = -1;

    // 10. Application finale des changements géométriques (Reset Zoom, curseurs, rafraîchissement)
    ApplyGeometryChange();
    NotifyLayerPictureUpdated();
}


//-----------------------------------------------------------------
// CUT : copie la sélection puis efface le contenu par transparence
//-----------------------------------------------------------------
bool CBitmapEditor::Cut()
{
    SaveHistoryState(L"Cut la sélection");

    // Appelle Copy pour mettre la sélection dans le presse-papiers
    if (!Copy())
        return false;

    // Appelle Delete pour effacer le contenu de la sélection par transparence
    return DeleteSelect();
}


//-----------------------------------------------------------------
// PASTE : récupère l'image du presse-papiers dans dataCopy (élément « volant »,
// centré sur le document) puis active l'outil de déplacement. Le collage est
// fixé dans le calque au relâchement de la souris (MouseRelease).
//-----------------------------------------------------------------
bool CBitmapEditor::Paste()
{
    if (wxTheClipboard->Open())
    {
        if (wxTheClipboard->IsSupported(wxDF_BITMAP))
        {
            wxBitmapDataObject data;
            wxTheClipboard->GetData(data);
            const wxBitmap bitmap = data.GetBitmap();

            if (bitmap.IsOk())
            {
                dataCopy.SetArray(Regards::Picture::CLibPicture::mat_from_wx(bitmap.ConvertToImage()));
                numLayerCopy = numActifLayer;

                // Centrage sur la taille du document global
                copyX = 0;
                copyY = 0;
                if (bitmapwidth > 0 && bitmapheight > 0)
                {
                    copyX = (bitmapwidth - dataCopy.getWidth()) / 2;
                    copyY = (bitmapheight - dataCopy.getHeight()) / 2;
                }

                
            }
        }
        wxTheClipboard->Close();
    }

    if (!dataCopy.empty())
        SetSelectMoveTool();

    return true;
}


// -----------------------------------------------------------------
// MOUSE RELEASE : Fixation définitive et collage direct de dataCopy
// -----------------------------------------------------------------
void CBitmapEditor::MouseRelease(const int& xPos, const int& yPos)
{
    if (toolOption == WM_SELECTMOVE)
    {
        mouseBlock = false;
        if (parentRender->HasCapture())
            parentRender->ReleaseMouse();

        LayerElement* targetLayer = nullptr;

        // COLLAGE DIRECT ET DEFINITIF DE DATA COPY DANS LE CALQUE CIBLE
        if (!dataCopy.empty() && numLayerCopy >= 0 && numLayerCopy < static_cast<int>(listOfLayer->size()))
        {
            targetLayer = GetElement(numLayerCopy);
            if (targetLayer != nullptr && targetLayer->GetPicture() != nullptr)
            {
                SaveHistoryState(L"Paste picture");

                cv::Mat targetMat = targetLayer->GetPicture()->GetMatImage();
                if (!targetMat.empty())
                {

                    // Calcul de la zone d'intersection (ROI) sécurisée
                    cv::Rect dstROI = cv::Rect(copyX, copyY, dataCopy.getWidth(), dataCopy.getHeight()) & cv::Rect(0, 0, targetMat.cols, targetMat.rows);

                    if (!dstROI.empty())
                    {
                        // Extraction de la sous-zone correspondante de la source
                        cv::Rect srcROI(dstROI.x - copyX, dstROI.y - copyY, dstROI.width, dstROI.height);

                        // Collage direct des pixels (écrase la zone sur la cible CPU)
                        dataCopy.getMat()(srcROI).copyTo(targetMat(dstROI));

                        // REPARATION CRITIQUE : Au lieu de simplement modifier la matrice existante, 
                        // on force CImageLoadingFormat à reconstruire entièrement ses structures (Mat et UMat)
                        // en réassignant une matrice clonée toute neuve.

                        cv::Mat ImageList_Copy = targetMat.clone();
                        targetLayer->GetPicture()->SetPicture(ImageList_Copy);

                        // Incrémentation de la version pour réveiller le pipeline OpenGL
                        targetLayer->pictureVersion++;
                    }
                }
            }
        }

        // Libération finale de la matrice de copie
        dataCopy.Release();

        // Notification de modification de la liste des calques
        listOfLayer->IsChanged(true);

        if (targetLayer)
        {
            // Invalidation des caches de textures matérielles de l'éditeur
            targetLayer->InvalidatePicture();
            targetLayer->needTextureUpdate = true;

            // Réassignation obligatoire au gestionnaire de filtres d'effet
            if (filtreEffet != nullptr)
            {
                filtreEffet->SetBitmap(targetLayer->GetPicture());
            }

            // Invalidation des anciennes tailles de l'écran pour forcer l'interpolation OpenCV
            widthOutputOld = -1;
            heightOutputOld = -1;

            // Force le rechargement global de la scène dans OnPaint3D
            loadBitmap = true;
            forceUpdateFilter = true;
            updateFilter = true;
            testPaste = true;
        }

        needToRefresh = true;



        // Envoi d'un événement pour notifier l'UI globale qu'un calque a été ajouté
        wxWindow* mainWindow = parentRender->FindWindowById(FRAMEEDITOR_ID);
        if (mainWindow) {
            wxCommandEvent evt(wxEVENT_UPDATELAYERPICTURE);
            mainWindow->GetEventHandler()->AddPendingEvent(evt);
        }

        wxSetCursor(*wxSTANDARD_CURSOR);
        return;
    }

    if (CFiltreData::SupportMouseClick(toolOption))
    {
        mouseBlock = false;
        mouseScrollX = xPos;
        mouseScrollY = yPos;
        if (parentRender->HasCapture())
            parentRender->ReleaseMouse();

        m_cDessin->MouseUp();

        if (m_cDessin->ApplyEffectOnMouseRelease())
        {
            ApplyEffectOnMouseRelease();
        }

        updateFilter = true;
        needToRefresh = true;
    }

    if (clickTimer->IsRunning())
        clickTimer->Stop();
}



//-----------------------------------------------------------------
// Sauvegarde l'état actuel avec un libellé descriptif
//-----------------------------------------------------------------
void CBitmapEditor::SaveHistoryState(const wxString& actionLabel)
{
    CHistoryState* historyState = new CHistoryState();
    historyState->libelle = actionLabel;
    historyState->currentLayers = listOfLayer.get();
    // Notifier ou mettre à jour l'interface de l'historique (HistoryDialog / CInfoEffectWnd)
    wxWindow* mainWindow = parentRender->FindWindowById(HISTORYWINDOWID); // ID de votre fenêtre d'historique si applicable
    // Ou via le gestionnaire d'effets/historique actif
    if (mainWindow) {
        wxCommandEvent evt(wxEVENT_REFRESHHISTORYLIST);
        evt.SetClientData(historyState);
        mainWindow->GetEventHandler()->AddPendingEvent(evt);
    }
}


void CBitmapEditor::OnFiltreOk()
{
    LayerElement* layerElement = GetActiveLayerElement();
    if (layerElement != nullptr && layerElement->GetPicture() != nullptr && filtreEffet != nullptr)
    {
        // Détermination du nom de l'effet appliqué
        wxString effectName = L"Application d'un effet";
        if (preview == IDM_RECTANGLESELECT) {
            effectName = CLibResource::LoadStringFromResource("IDM_RECTANGLESELECT", 1);
        }
        else  if (preview == IDM_ELLIPSESELECT)
        {
            effectName = CLibResource::LoadStringFromResource("IDM_ELLIPSESELECT", 1);
        }
        else  if (preview == IDM_LASSOSELECT)
        {
            effectName = CLibResource::LoadStringFromResource("IDM_LASSOSELECT", 1);
        }
        else if (preview > 0) {
            effectName = wxString::Format(L"Effet ou Filtre (ID: %d)", preview);
        }

        // Sauvegarde de l'état avec le nom de l'effet
        SaveHistoryState(effectName);

        //Regards::Picture::CPictureArray processedPicture(filtreEffet->GetMatrix());
        // ... (reste du code de fixation du filtre)

        RemoveListener();
        forceUpdateFilter = false;
        widthOutputOld = 0;
    }

}

CBitmapEditor::CBitmapEditor(CSliderInterface* slider, wxWindowID mainViewerId,
    const CThemeBitmapWindow& theme,
    CBitmapInterface* bitmapInterface)
    : Regards::Control::CBitmapWndViewer(slider, mainViewerId, theme,
        bitmapInterface) {
    fixArrow = false;
}

bool CBitmapEditor::ApplySelectEffect(int& widthOutput, int& heightOutput) {
    return false;
}

CBitmapEditor::~CBitmapEditor(void) {}

void CBitmapEditor::SetRealSize() {
    ratio = 1.0f;
    this->RefreshWindow();
}

void CBitmapEditor::SetFilename(const wxString& filename) {
    this->filename = filename;
}

void CBitmapEditor::GenerateScreenBitmap(CFiltreEffet* filtreEffet,
    int& widthOutput, int& heightOutput) {
    CRegardsConfigParam* regardsParam = CParamInit::getInstance();
    int localAngle = angle;
    int localflipHorizontal = flipHorizontal;
    int localflipVertical = flipVertical;
    int filterInterpolation = 0;

    if (regardsParam != nullptr)
        filterInterpolation = regardsParam->GetInterpolationType();

    int left = 0, top = 0;
    int tailleAffichageWidth = widthOutput;
    int tailleAffichageHeight = heightOutput;

    if (GetWidth() * scale_factor > tailleAffichageWidth)
        left = ((GetWidth() * scale_factor - tailleAffichageWidth) / 2);
    else
        left = 0;

    if (GetHeight() * scale_factor > tailleAffichageHeight)
        top = ((GetHeight() * scale_factor - tailleAffichageHeight) / 2);
    else
        top = 0;

    wxRect rc(0, 0, 0, 0);
    CalculRectPictureInterpolation(rc, widthOutput, heightOutput, left, top,
        false);

    filtreEffet->Interpolation(widthOutput, heightOutput, rc, filterInterpolation,
        localflipHorizontal, localflipVertical, localAngle,
        value[posRatio]);
}

//-----------------------------------------------------------------
// Version ultra-rapide (GPU) : Notifie simplement que l'élément volant 
// a changé de position, la composition réelle est déléguée aux shaders.
//-----------------------------------------------------------------
void CBitmapEditor::ApplyFloatingPasteToFilter() {
    if (dataCopy.empty()) return;

    // On lève le flag pour forcer OnPaint3D et le compositeur à ré-assembler la scène
    forceUpdateFilter = true;
    needToRefresh = true;
}

//-----------------------------------------------------------------
// Régénère le bitmap d'écran du calque actif (filtres / preview /
// redimensionnement). Retourne true si le résultat du calque actif a été
// régénéré.
//-----------------------------------------------------------------
bool CBitmapEditor::UpdateScreenBitmap(LayerElement* activeLayer,
    int& widthOutput, int& heightOutput,
    bool bitmapIsLoad) {
    ApplyFloatingPasteToFilter();

    updateFilter = forceUpdateFilter || preview > 0 || bitmapIsLoad;

    if (updateFilter) {
        // Pas de réassignation du calque au filtre pendant le déplacement d'un collage
        if (dataCopy.empty() && !bitmapIsLoad)
            filtreEffet->SetBitmap(activeLayer->GetPicture());

        BeforeInterpolationBitmap();
        updateFilter = true;  // BeforeInterpolationBitmap peut modifier le drapeau
    }

    const bool regenerate = updateFilter || widthOutputOld != widthOutput ||
        heightOutputOld != heightOutput;
    if (regenerate) {

        GenerateScreenBitmap(filtreEffet.get(), widthOutput, heightOutput);
        ApplyPreviewEffect(widthOutput, heightOutput);
        ApplySelectEffect(widthOutput, heightOutput);

        // Invalider le cache de texture de TOUS les calques secondaires lors d'un
        // changement d'affichage global
        for (size_t i = 0; i < listOfLayer->size(); ++i) {

            LayerElement * element = GetElement(i);
            if (element != nullptr)
                element->needTextureUpdate = true;
        }
    }

    updateFilter = false;
    widthOutputOld = widthOutput;
    heightOutputOld = heightOutput;
    return regenerate;
}



//-----------------------------------------------------------------
// ANNULER (Undo)
//-----------------------------------------------------------------
void CBitmapEditor::Undo()
{
    wxWindow* mainWindow = parentRender->FindWindowById(HISTORYWINDOWID); // ID de votre fenêtre d'historique si applicable
    // Ou via le gestionnaire d'effets/historique actif
    if (mainWindow) {
        wxCommandEvent evt(wxEVENT_UNDOHISTORY);
        mainWindow->GetEventHandler()->AddPendingEvent(evt);
    }

}

//-----------------------------------------------------------------
// REFAIRE (Redo)
void CBitmapEditor::Redo()
{
    wxWindow* mainWindow = parentRender->FindWindowById(HISTORYWINDOWID); // ID de votre fenêtre d'historique si applicable
    // Ou via le gestionnaire d'effets/historique actif
    if (mainWindow) {
        wxCommandEvent evt(wxEVENT_REDOHISTORY);
        mainWindow->GetEventHandler()->AddPendingEvent(evt);
    }
}

void CBitmapEditor::ClearHistory()
{
    wxWindow* mainWindow = parentRender->FindWindowById(HISTORYWINDOWID); // ID de votre fenêtre d'historique si applicable
    // Ou via le gestionnaire d'effets/historique actif
    if (mainWindow) {
        wxCommandEvent evt(wxEVENT_CANCELHISTORY);
        mainWindow->GetEventHandler()->AddPendingEvent(evt);
    }

}

//-----------------------------------------------------------------
// Dessin de l'image
//-----------------------------------------------------------------
void CBitmapEditor::OnPaint3D(wxGLCanvas* canvas, CRenderOpenGL* renderOpenGL) {
#ifndef WIN32
    scale_factor = parentRender->GetContentScaleFactor();
#endif

    LayerElement* layerElement = GetActiveLayerElement();
    if (layerElement == nullptr) return;

    // Changement de calque source ou de version -> rechargement du bitmap
    if (layerElement->layerId != lastSourceLayerId ||
        layerElement->pictureVersion != lastSourceVersion) {
        loadBitmap = true;
        lastSourceLayerId = layerElement->layerId;
        lastSourceVersion = layerElement->pictureVersion;
    }

    CImageLoadingFormat* source = layerElement->GetPicture();
    isOpenGLShow = true;

    if (renderBitmapOpenGL == nullptr) {
        this->renderOpenGL = renderOpenGL;
        renderBitmapOpenGL = std::make_unique<CRenderBitmapOpenGL>(renderOpenGL);
        renderBitmapOpenGL->LoadingResource(scale_factor, themeBitmap.colorArrow);
    }

    int widthOutput = static_cast<int>(GetBitmapWidthWithRatio()) * scale_factor;
    int heightOutput =
        static_cast<int>(GetBitmapHeightWithRatio()) * scale_factor;

    bool bitmapIsLoad = false;

    if (loadBitmap) {
        if (filtreEffet == nullptr) {
            CRgbaquad color;
            filtreEffet = std::make_unique<CFiltreEffet>(
                color, renderOpenGL->GetOpenCLContext(), source);
        }
        else
            filtreEffet->SetBitmap(source);

        loadBitmap = false;
        bitmapIsLoad = true;
    }

    if (bitmapLoad && GetWidth() > 0 && GetHeight() > 0) {
        if (widthOutput < 0 || heightOutput < 0) return;

        const bool activeDirty = UpdateScreenBitmap(layerElement, widthOutput,
            heightOutput, bitmapIsLoad);

        // Initialiser le fond de l'écran avec la couleur de thème
        renderOpenGL->CreateScreenRender(
            GetWidth() * scale_factor, GetHeight() * scale_factor,
            CRgbaquad(themeBitmap.colorBack.Red(), themeBitmap.colorBack.Green(),
                themeBitmap.colorBack.Blue()));
        GLTexture* textureDisplay =
            renderOpenGL->GetDisplayTexture(widthOutput, heightOutput);
        if (textureDisplay == nullptr) return;
        ComposeLayers(layerElement, textureDisplay, widthOutput, heightOutput,
            activeDirty);
        RenderTexture(textureDisplay, true, widthOutput, heightOutput);
    }
    AfterRender();
    canvas->SwapBuffers();
}

//-----------------------------------------------------------------
// Routage de la composition selon le backend du calque ; repli sur le
// renderer CPU en cas d'échec
//-----------------------------------------------------------------
void CBitmapEditor::ComposeLayers(LayerElement* typeSource,
    GLTexture* textureDisplay, int width,
    int height, bool activeDirty) {
    const auto typeLayer = typeSource->TypeLayer();
    ILayerScene& scene = *this;

    if (typeLayer == TYPE_OPENGL) {
        if (!rendererGL)
            rendererGL = std::make_unique<CLayerRendererGL>();
        if (!rendererGL->Compose(scene, textureDisplay, width, height, activeDirty)) {
            LogFallbackOnce(gpuComposeFallbackLogged,
                "CBitmapEditor : composition GPU des calques "
                "indisponible, repli CPU");
            ComposeLayersCPU(scene, textureDisplay, width, height);
        }
    }
    else if (typeLayer == TYPE_OPENCL) {
        if (!rendererOpenCL)
            rendererOpenCL = std::make_unique<CLayerRendererOpenCL>();
        if (!rendererOpenCL->Compose(scene, textureDisplay, width, height, activeDirty)) {
            LogFallbackOnce(openclComposeFallbackLogged,
                "CBitmapEditor : composition OpenCL indisponible ou "
                "erreur, repli CPU");
            ComposeLayersCPU(scene, textureDisplay, width, height);
        }
    }
}

void CBitmapEditor::ComposeLayersCPU(ILayerScene& scene, GLTexture* textureDisplay,
    int width, int height) {
    if (!rendererCPU)
        rendererCPU = std::make_unique<CLayerRendererCPU>();
    rendererCPU->Compose(scene, textureDisplay, width, height, false);
}

//-----------------------------------------------------------------
// ILayerScene : services fournis aux renderers
//-----------------------------------------------------------------
CLayerList * CBitmapEditor::SceneLayers() { return listOfLayer.get(); }
int CBitmapEditor::SceneActiveIndex() const { return numActifLayer; }
float CBitmapEditor::SceneRatio() const { return ratio; }

cv::Mat CBitmapEditor::SceneBackground(int width, int height) {
    return PrepareBackgroundMat(width, height);
}
cv::Mat CBitmapEditor::SceneActiveLayer(int width, int height) {
    return PrepareActiveLayerImage(width, height);
}
cv::UMat CBitmapEditor::SceneActiveLayerOpenCL(int width, int height) {
    return PrepareActiveLayerImageOpenCL(width, height);
}
cv::Mat CBitmapEditor::SceneLayer(LayerElement* layer, int width, int height,
    bool useGLContext) {
    CRgbaquad color;
    CFiltreEffet tempFilter(color,
        useGLContext ? renderOpenGL->GetOpenCLContext() : nullptr,
        layer->GetPicture());
    return PrepareLayerSource(layer, width, height, &tempFilter);
}
cv::UMat CBitmapEditor::SceneLayerOpenCL(LayerElement* layer, int width, int height) {
    CRgbaquad color;
    CFiltreEffet tempFilter(color, renderOpenGL->GetOpenCLContext(),
        layer->GetPicture());
    return PrepareLayerSourceOpenCL(layer, width, height, &tempFilter);
}

bool CBitmapEditor::SceneHasFloating() { return !dataCopy.empty(); }
cv::Mat CBitmapEditor::SceneFloatingMat() { return dataCopy.getMat(); }
cv::UMat CBitmapEditor::SceneFloatingUMat() { return dataCopy.getUMat(); }
cv::Point CBitmapEditor::SceneFloatingPosition() const {
    return cv::Point(
        static_cast<int>(static_cast<float>(copyX) * ratio * scale_factor),
        static_cast<int>(static_cast<float>(copyY) * ratio * scale_factor));
}

bool CBitmapEditor::SceneHasRenderContext() const { return renderOpenGL != nullptr; }
bool CBitmapEditor::SceneUpload(GLTexture& texture, const cv::Mat& image) {
    return UploadToTexture(texture, image);
}

//-----------------------------------------------------------------
// Briques communes aux backends
//-----------------------------------------------------------------
// Fond de prévisualisation opaque (damier) à la taille de sortie ; gris uni si
// la préparation échoue.
cv::Mat CBitmapEditor::PrepareBackgroundMat(int width, int height) {
    if (pictureBackground.empty() || pictureBackground.getWidth() != width ||
        pictureBackground.getHeight() != height) {
        pictureBackground.SetArray(
            Regards::Picture::CLibPicture::CreateCheckerboardBackground(width,
                height));
    }
    cv::Mat background = LayerFonction::PrepareLayerImage(
        pictureBackground.getMat(), width, height);
    if (background.empty())
        background =
        cv::Mat(height, width, CV_8UC4, cv::Scalar(128, 128, 128, 255));
    return background;
}

// Résultat des filtres/preview du calque actif (filtreEffet), préparé à la
// taille de sortie et positionné selon l'offset du calque.
cv::Mat CBitmapEditor::PrepareActiveLayerImage(int width, int height) {
    Regards::Picture::CPictureArray activePicture(filtreEffet->GetMatrix());

    const LayerElement* layer = GetActiveLayerElement();
    const int lx = layer ? layer->x : 0;
    const int ly = layer ? layer->y : 0;
    return LayerFonction::PrepareLayerImage(activePicture.getMat(), width, height, lx, ly, ratio);
}

// Résultat des filtres/preview du calque actif (filtreEffet), préparé à la
// taille de sortie et positionné selon l'offset du calque.
cv::UMat CBitmapEditor::PrepareActiveLayerImageOpenCL(int width, int height) {
    Regards::Picture::CPictureArray activePicture(filtreEffet->GetMatrix());

    const LayerElement* layer = GetActiveLayerElement();
    const int lx = layer ? layer->x : 0;
    const int ly = layer ? layer->y : 0;
    return LayerFonction::PrepareLayerImageOpenCL(activePicture.getUMat(), width, height, lx, ly, ratio);
}


bool CBitmapEditor::UploadToTexture(GLTexture& texture, const cv::Mat& image) {
    cv::Mat local = image;
    Regards::Picture::CPictureArray array(local);
    return texture.SetData(array, renderOpenGL->GetOpenCLContext());
}

void CBitmapEditor::CanvasResize(CanvasSizeParameter canvasSize) {
    // Parcourir l'ensemble des calques
    for (size_t i = 0; i < listOfLayer->size(); ++i) {
        LayerElement* layer = GetElement(i);
        if (!layer) continue;
        CImageLoadingFormat* pictureFormat = layer->GetPicture();
        if (!pictureFormat) continue;
        [[maybe_unused]] const cv::Mat newCanvas =
            PlaceOnCanvas(pictureFormat->GetMatImage(), canvasSize);
        // Mettre à jour l'image du calque avec le nouveau canvas redimensionné
        // pictureFormat->SetPicture(newCanvas);
    }
    listOfLayer->IsChanged(true);
    UpdateBitmapSizeFromLayers();
    ApplyGeometryChange();
}
void CBitmapEditor::Resize(const int& widthOut, const int& heightOut,
    const int& method) {
    // 1. Parcourir et redimensionner chaque calque de la liste
    for (size_t i = 0; i < listOfLayer->size(); ++i) {
        LayerElement* layer = GetElement(i);
        if (!layer) continue;
        CImageLoadingFormat* layerSource = layer->GetPicture();
        if (!layerSource) continue;
        // Initialisation des filtres d'effet OpenCV pour le calque actuel
        CRgbaquad color;
        CFiltreEffet layerFilter(color, nullptr, layerSource);
        // Calcul du redimensionnement via OpenCV
        cv::Mat output = layerFilter.Resize(widthOut, heightOut, method);
        // Appliquer la nouvelle matrice redimensionnée au calque
        layerSource->SetPicture(output);
        // L'image GPU du calque est maintenant perimee.
        layer->InvalidatePicture();
    }
    listOfLayer->IsChanged(true);
    // 2. Mise à jour des métadonnées du Viewer
    if (!UpdateBitmapSizeFromLayers()) {
        // Sécurité si GetPictureToShow() utilise une logique différente ou est nul
        bitmapwidth = widthOut;
        bitmapheight = heightOut;
        orientation = 1;  // Orientation par défaut (Normal)
    }
    ApplyGeometryChange();
}
//-----------------------------------------------------------------
// Taille / orientation de l'éditeur d'après l'image principale. false si aucune
// image.
//-----------------------------------------------------------------
bool CBitmapEditor::UpdateBitmapSizeFromLayers() {
    CImageLoadingFormat* source = listOfLayer->GetPictureToShow();
    if (source == nullptr) return false;
    bitmapwidth = source->GetWidth();
    bitmapheight = source->GetHeight();
    orientation = source->GetOrientation();
    return true;
}
//-----------------------------------------------------------------
// Réinitialise l'état de la vue après un changement d'image
//-----------------------------------------------------------------
void CBitmapEditor::ResetViewState() {
    loadBitmap = true;
    bitmapLoad = true;
    bitmapUpdate = true;
    flipVertical = 0;
    flipHorizontal = 0;
    angle = 0;
}
//-----------------------------------------------------------------
// Fin commune de Resize / CanvasResize : reset d'état et rafraîchissement
//-----------------------------------------------------------------
void CBitmapEditor::ApplyGeometryChange() {
    ResetViewState();
    toolOption = MOVEPICTURE;
    ShrinkImage(false);
    AfterSetBitmap();
    RemoveListener(false);
    if (parentRender) parentRender->Refresh();
    RefreshWindow();
}
void CBitmapEditor::ApplyEffectOnMouseRelease() {
    wxWindow* parameterWindow = parentRender->FindWindowById(PARAMETERWINDOWID);
    if (parameterWindow) {
        wxCommandEvent evt(wxEVENT_FILTREOK);
        parameterWindow->GetEventHandler()->AddPendingEvent(evt);
    }
}
void CBitmapEditor::RemoveListener(const bool& applyCancel) {
    if (m_cDessin)
        if (m_cDessin->ApplyEffectOnMouseRelease()) return;
    if (mouseUpdate != nullptr && applyCancel) {
        mouseUpdate->CancelPreview(this);
        updateFilter = true;
        if (listOfLayer->size() > 0) {
            bitmapwidth = listOfLayer->GetWidth();
            bitmapheight = listOfLayer->GetHeight();
            ShrinkImage();
        }
    }
    mouseUpdate = nullptr;
    effectParameter = nullptr;
    loadBitmap = true;
    needToRefresh = true;
}
void CBitmapEditor::SetActifLayer(const int& numLayer) {
    if (numLayer >= 0 && static_cast<size_t>(numLayer) < listOfLayer->size())
    {
        numActifLayer = numLayer;
        loadBitmap = true;
        needToRefresh = true;
    }
}
void CBitmapEditor::RenderTexture(GLTexture* textureDisplay,
    const bool& invertPos, int width,
    int height) {
    if (listOfLayer->size() > 0) {
        int x = (GetWidth() * scale_factor - width) / 2;
        int y = (GetHeight() * scale_factor - height) / 2;
        renderOpenGL->RenderToScreen(mouseUpdate, effectParameter, x, y, invertPos);
        xPosImage = x;
        yPosImage = y;
    }
}

void CBitmapEditor::SetKey(const int& iKey)
{
    this->iKey = iKey;

    // 1. Gestion de la touche Suppr (Delete) autonome pour annuler la sélection
    if (iKey == WXK_ESCAPE)
    {
        CancelSelect();
        return;
    }

    if (iKey == WXK_DELETE)
    {
        DeleteSelect();
        CancelSelect();
        return;
    }

    // 2. Raccourcis presse-papiers : Exécution STRICTEMENT si la touche CTRL est enfoncée
    if (wxGetKeyState(WXK_CONTROL))
    {
        bool (CBitmapEditor:: * clipboardAction)() = nullptr;

        switch (iKey)
        {
        case 3:  case 'C': case 'c':
            clipboardAction = &CBitmapEditor::Copy;
            break;
        case 22: case 'V': case 'v':
            clipboardAction = &CBitmapEditor::Paste;
            break;
        case 24: case 'X': case 'x':
            clipboardAction = &CBitmapEditor::Cut;
            break;
        default:
            break;
        }

        if (clipboardAction != nullptr)
        {
            (this->*clipboardAction)();
            return;  // Évite d'altérer les curseurs après une action sur le presse-papiers
        }
    }

    // 3. Curseur Zoom de l'outil de déplacement
    if (toolOption == MOVEPICTURE)
    {
        if (iKey == WXK_CONTROL)
            wxSetCursor(hCursorZoomIn);
        else
            wxSetCursor(hCursorZoomOut);
    }
}


int CBitmapEditor::GetKey()
{
    //
    return iKey;
}

void CBitmapEditor::OnUpdateLayerBitmap(wxCommandEvent& event) {
    auto picture = static_cast<CImageLoadingFormat*>(event.GetClientData());
    if (picture == nullptr || !picture->IsOk()) return;
    ResetViewState();
    LayerElement* element = GetElement(numActifLayer);
    if (element)
        element->SetPicture(picture);

    NotifyLayerPictureUpdated();
}

//-----------------------------------------------------------------
// Outils simples (sans objet de dessin) : sélection-déplacement, zoom, déplacement
//-----------------------------------------------------------------
void CBitmapEditor::ActivateSimpleTool(int previewId, int tool, const wxCursor& cursor)
{
    preview = previewId;
    isInUse = true;
    m_cDessin.reset(nullptr);

    forceUpdateFilter = false;
    wxSetCursor(cursor);
    toolOption = tool;
}

void CBitmapEditor::SetSelectMoveTool()
{
    ActivateSimpleTool(WM_SELECTMOVE, WM_SELECTMOVE, *wxHOURGLASS_CURSOR);
}

void CBitmapEditor::SetZoomTool()
{
    ActivateSimpleTool(WM_SELECTZOOM, ZOOMPICTURE, hCursorZoomIn);
}

void CBitmapEditor::SetMoveTool()
{
    ActivateSimpleTool(WM_MOVEPICTURE, MOVEPICTURE, *wxSTANDARD_CURSOR);
}

void CBitmapEditor::OnSetBitmap(wxCommandEvent& event)
{
    auto picture = static_cast<CImageLoadingFormat*>(event.GetClientData());
    uint64_t layerId = static_cast<uint64_t>(event.GetExtraLong());
    if (picture != nullptr)
        SetBitmapByLayerId(picture, layerId);
}


//-----------------------------------------------------------------
// Affectation de l'image
//-----------------------------------------------------------------
void CBitmapEditor::SetBitmapByLayerId(CImageLoadingFormat* bitmapIn, const uint64_t& layerId)
{
    if (bitmapIn == nullptr || !bitmapIn->IsOk())
        return;

    filename = bitmapIn->GetFilename();
    ResetViewState();
    UpdateLayerId(bitmapIn, layerId);

    toolOption = MOVEPICTURE;
    bitmapwidth = bitmapIn->GetWidth();
    bitmapheight = bitmapIn->GetHeight();
    orientation = bitmapIn->GetOrientation();

    // Orientation EXIF mémorisée pour cette photo
    CSqlPhotos sqlPhotos;
    const int exifCode = sqlPhotos.GetPhotoExif(filename);
    if (exifCode != -1)
        application_context.GetAngleAndFlip(exifCode, angle, flipHorizontal, flipVertical);

    ShrinkImage(false);
    AfterSetBitmap();

    RemoveListener(false);

    parentRender->Refresh();
    RefreshWindow();
}

void CBitmapEditor::UpdateLayerId(CImageLoadingFormat* bitmapIn, const uint64_t& layerId)
{
    LayerElement* element = listOfLayer->FindElementByLayerId(layerId);
    if (element == nullptr)
        return;

    element->SetPicture(bitmapIn);
}