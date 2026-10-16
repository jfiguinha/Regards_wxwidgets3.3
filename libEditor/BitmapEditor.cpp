// ==================== BitmapEditor.cpp ====================
#include <header.h>
#include "BitmapEditor.h"
#include <Draw.h>
#include <FiltreEffet.h>
#include <GLTexture.h>
#include <ImageLoadingFormat.h>
#include <LayerFonction.h>
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
#include <algorithm>
#include <vector>
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

    // Parcourt les calques dans l'ordre (index 0 -> dernier) en sautant les calques
    // nuls, invisibles ou d'opacité nulle. Opacité normalisée dans [0..1].
    template <typename LayerList, typename Visitor>
    void ForEachVisibleLayer(LayerList& layers, Visitor&& visit) {
        const size_t count = static_cast<size_t>(layers.size());
        for (size_t index = 0; index < count; index++) {
            LayerElement* layer = layers[index];
            if (layer == nullptr || !layer->isVisible) continue;

            const float opacity =
                LayerFonction::Clamp01(static_cast<float>(layer->opacity) / 100.0f);
            if (opacity <= 0.0f) continue;

            visit(index, layer, opacity);
        }
    }

    // Le cache GPU/UMat du calque est-il valide pour cette taille de sortie ?
    bool IsLayerTextureCurrent(const LayerElement* layer, int width, int height) {
        return !layer->needTextureUpdate && layer->textureWidth == width &&
            layer->textureHeight == height;
    }

    void MarkLayerTextureCurrent(LayerElement* layer, int width, int height) {
        layer->needTextureUpdate = false;
        layer->textureWidth = width;
        layer->textureHeight = height;
    }

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

//-----------------------------------------------------------------
// Accès au calque actif / à l'outil de sélection
//-----------------------------------------------------------------
LayerElement* CBitmapEditor::GetActiveLayerElement() {
    if (numActifLayer < 0 ||
        numActifLayer >= static_cast<int>(listOfLayer.size()))
        return nullptr;
    return listOfLayer[numActifLayer];
}

CSelectFilterParameter* CBitmapEditor::GetSelectFilter() {
    if (effectParameter == nullptr || preview != IDM_SELECTFILTER)
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

    cv::Mat outMat;  // Matrice envoyée au presse-papiers

    // Sélection active : détourage + découpe à la boîte englobante
    CSelectFilterParameter* filter = GetSelectFilter();
    if (filter != nullptr && !filter->listSelections.empty())
    {
        const cv::Rect roi = SelectionBounds(filter->listSelections, sourceMat.size());
        if (roi.width > 0 && roi.height > 0)
        {
            cv::Mat bgraMat = ToBGRA(sourceMat);
            ApplyMaskToAlpha(bgraMat, BuildCopyMask(filter->listSelections, sourceMat.size()));
            outMat = bgraMat(roi).clone();
        }
    }

    // Aucune sélection active ou valide : copie de l'image entière
    if (outMat.empty())
        outMat = sourceMat;

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
// CUT : copie l'image du calque actif puis vide le calque
//-----------------------------------------------------------------
bool CBitmapEditor::Cut()
{
    if (!Copy())
        return false;

    LayerElement* layerElement = GetActiveLayerElement();
    CImageLoadingFormat* picture = layerElement->GetPicture();

    const cv::Mat currentMat = picture->GetMatImage();
    if (!currentMat.empty())
    {
        // Remplace l'image par une matrice vide de mêmes dimensions
        cv::Mat emptyMat = cv::Mat::zeros(currentMat.rows, currentMat.cols, currentMat.type());
        picture->SetPicture(emptyMat);

        // Invalidation des caches et mise à jour géométrique
        layerElement->InvalidatePicture();
        listOfLayer.IsChanged(true);
        ApplyGeometryChange();
    }

    return true;
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
                dataCopy = Regards::Picture::CLibPicture::mat_from_wx(bitmap.ConvertToImage());
                numLayerCopy = numActifLayer;

                // Centrage sur la taille du document global
                copyX = 0;
                copyY = 0;
                if (bitmapwidth > 0 && bitmapheight > 0)
                {
                    copyX = (bitmapwidth - dataCopy.size().width) / 2;
                    copyY = (bitmapheight - dataCopy.size().height) / 2;
                }

                
            }
        }
        wxTheClipboard->Close();
    }

    if (!dataCopy.empty())
        SetSelectMoveTool();

    return true;
}

//-----------------------------------------------------------------
// Fixe dataCopy dans le calque cible (numLayerCopy), puis libère dataCopy.
// Retourne le calque cible (nullptr si aucun calque valide).
//-----------------------------------------------------------------
LayerElement* CBitmapEditor::CommitFloatingPaste()
{
    LayerElement* targetLayer = nullptr;

    if (!dataCopy.empty() && numLayerCopy >= 0 &&
        numLayerCopy < static_cast<int>(listOfLayer.size()))
    {
        targetLayer = listOfLayer[numLayerCopy];
        if (targetLayer != nullptr && targetLayer->GetPicture() != nullptr)
        {
            cv::Mat targetMat = targetLayer->GetPicture()->GetMatImage();
            if (!targetMat.empty() && CopyIntoROI(dataCopy, targetMat, copyX, copyY))
            {
                targetLayer->GetPicture()->SetPicture(targetMat);

                // Force le changement de version pour réveiller OnPaint3D
                targetLayer->pictureVersion++;
            }
        }
    }

    dataCopy.release();
    return targetLayer;
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
        if (!dataCopy.empty() && numLayerCopy >= 0 && numLayerCopy < static_cast<int>(listOfLayer.size()))
        {
            targetLayer = listOfLayer[numLayerCopy];
            if (targetLayer != nullptr && targetLayer->GetPicture() != nullptr)
            {
                cv::Mat targetMat = targetLayer->GetPicture()->GetMatImage();
                if (!targetMat.empty())
                {
                    // Aligne le nombre de canaux pour éviter les plantages (copyTo nécessite des formats identiques)
                    if (dataCopy.channels() != targetMat.channels())
                    {
                        if (targetMat.channels() == 4)
                            cv::cvtColor(dataCopy, dataCopy, cv::COLOR_BGR2BGRA);
                        else if (targetMat.channels() == 3)
                            cv::cvtColor(dataCopy, dataCopy, cv::COLOR_BGRA2BGR);
                    }

                    // Calcul de la zone d'intersection (ROI) sécurisée
                    cv::Rect dstROI = cv::Rect(copyX, copyY, dataCopy.cols, dataCopy.rows) & cv::Rect(0, 0, targetMat.cols, targetMat.rows);

                    if (!dstROI.empty())
                    {
                        // Extraction de la sous-zone correspondante de la source
                        cv::Rect srcROI(dstROI.x - copyX, dstROI.y - copyY, dstROI.width, dstROI.height);

                        // Collage direct des pixels (écrase la zone sur la cible CPU)
                        dataCopy(srcROI).copyTo(targetMat(dstROI));

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
        dataCopy.release();

        // Notification de modification de la liste des calques
        listOfLayer.IsChanged(true);

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
        if (parentRender)
            parentRender->Refresh(); // Actualise instantanément la fenêtre à l'écran

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
// Collage « volant » (non destructif) : tant que dataCopy est en cours de
// déplacement, fusionne l'image du calque cible et dataCopy dans un canvas
// temporaire à la taille du document, appliqué au filtre d'affichage uniquement.
//-----------------------------------------------------------------
void CBitmapEditor::ApplyFloatingPasteToFilter() {
    if (dataCopy.empty() || numLayerCopy < 0 ||
        numLayerCopy >= static_cast<int>(listOfLayer.size()))
        return;

    LayerElement* targetLayer = listOfLayer[numLayerCopy];
    if (targetLayer == nullptr || targetLayer->GetPicture() == nullptr) return;

    const cv::Mat originalMat = targetLayer->GetPicture()->GetMatImage();
    if (originalMat.empty()) return;

    // Image d'origine (non modifiée) posée en (0,0) sur un canvas transparent
    cv::Mat displayMat = cv::Mat::zeros(bitmapheight, bitmapwidth, CV_8UC4);
    CopyIntoROI(ToBGRA(originalMat), displayMat, 0, 0);

    // Incrustation de dataCopy (alpha blending, mode Normal) à sa position
    const cv::Rect dstROI = cv::Rect(copyX, copyY, dataCopy.cols, dataCopy.rows) &
        cv::Rect(0, 0, bitmapwidth, bitmapheight);
    if (!dstROI.empty()) {
        const cv::Rect srcROI(dstROI.x - copyX, dstROI.y - copyY, dstROI.width,
            dstROI.height);
        cv::Mat dstView = displayMat(dstROI);
        LayerFonction::CompositeLayer(dstView, dataCopy(srcROI), 1.0f,
            LayerBlendMode::Normal);
    }

    CImageLoadingFormat* imageLoading = new CImageLoadingFormat();
    imageLoading->SetPicture(displayMat);

    // Application TEMPORAIRE du canvas fusionné au filtre d'affichage
    filtreEffet->SetBitmap(imageLoading);
    forceUpdateFilter = true;
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
        for (size_t i = 0; i < listOfLayer.size(); ++i) {
            if (listOfLayer[i] != nullptr && static_cast<int>(i) != numActifLayer) {
                listOfLayer[i]->needTextureUpdate = true;
            }
        }
    }

    updateFilter = false;
    widthOutputOld = widthOutput;
    heightOutputOld = heightOutput;
    return regenerate;
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
// Routage de la composition selon le backend du calque, repli CPU en cas
// d'échec
//-----------------------------------------------------------------
void CBitmapEditor::ComposeLayers(LayerElement* typeSource,
    GLTexture* textureDisplay, int width,
    int height, bool activeDirty) {
    const auto typeLayer = typeSource->TypeLayer();
    if (typeLayer == TYPE_OPENGL) {
        if (!ComposeLayersGPU(textureDisplay, width, height, activeDirty)) {
            LogFallbackOnce(gpuComposeFallbackLogged,
                "CBitmapEditor : composition GPU des calques "
                "indisponible, repli CPU");
            ComposeLayersCPU(textureDisplay, width, height);
        }
    }
    else if (typeLayer == TYPE_OPENCL) {
        if (!ComposeLayersOpenCL(textureDisplay, width, height, activeDirty)) {
            LogFallbackOnce(openclComposeFallbackLogged,
                "CBitmapEditor : composition OpenCL indisponible ou "
                "erreur, repli CPU");
            ComposeLayersCPU(textureDisplay, width, height);
        }
    }
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

//-----------------------------------------------------------------
// [OPENCL] Récupère la texture matérielle OpenCL (cv::UMat) d'un calque
//-----------------------------------------------------------------
cv::UMat CBitmapEditor::GetLayerUMat(LayerElement* layer, int width,
    int height) {
    auto* layerUMat = static_cast<LayerElementUMat*>(layer);
    cv::UMat texture = layerUMat->GetUMatTexture();
    if (!texture.empty() && IsLayerTextureCurrent(layer, width, height))
        return texture;
    CRgbaquad color;
    CFiltreEffet tempFilter(color, renderOpenGL->GetOpenCLContext(),
        layer->GetPicture());
    cv::UMat prepared =
        PrepareLayerSourceOpenCL(layer, width, height, &tempFilter);
    if (prepared.empty()) return cv::UMat();
    // Transfert vers l'espace mémoire d'OpenCL
    cv::UMat newUMatTexture = prepared;
    layerUMat->SetUMatTexture(newUMatTexture);
    MarkLayerTextureCurrent(layer, width, height);
    return newUMatTexture;
}
//-----------------------------------------------------------------
// [OPENCL] Récupère le cv::UMat du calque actif (résultat du pipeline de
// filtres)
//-----------------------------------------------------------------
cv::UMat CBitmapEditor::GetActiveLayerUMat(int width, int height,
    bool activeDirty) {
    const cv::Size size(width, height);
    if (!activeLayerUMat.empty() && !activeDirty && activeLayerUMatSize == size)
        return activeLayerUMat;
    cv::UMat prepared = PrepareActiveLayerImageOpenCL(width, height);

    if (prepared.empty()) return cv::UMat();
    activeLayerUMat = prepared;
    activeLayerUMatSize = size;
    return activeLayerUMat;
}
//-----------------------------------------------------------------
// [OPENCL] Pipeline global de composition OpenCL
//-----------------------------------------------------------------
bool CBitmapEditor::ComposeLayersOpenCL(GLTexture* textureDisplay, int width,
    int height, bool activeDirty) {
    if (width <= 0 || height <= 0 || textureDisplay == nullptr) return false;
    if (!layerCompositorOpenCL)
        layerCompositorOpenCL = std::make_unique<CLayerCompositorOpenCL>();
    if (!layerCompositorOpenCL->IsValid()) return false;
    // 1. Fond en damier (reconstruit uniquement si la taille de sortie change)
    const cv::Size size(width, height);
    if (backgroundUMat.empty() || backgroundUMatSize != size) {
        backgroundUMat =
            PrepareBackgroundMat(width, height).getUMat(cv::ACCESS_READ);
        backgroundUMatSize = size;
    }
    // 2. Remplissage du vecteur de passes OpenCL
    std::vector<LayerPassOpenCL> passes;
    passes.reserve(listOfLayer.size());
    ForEachVisibleLayer(
        listOfLayer, [&](size_t index, LayerElement* layer, float opacity) {
            cv::UMat texture = IsActiveLayer(index)
                ? GetActiveLayerUMat(width, height, activeDirty)
                : GetLayerUMat(layer, width, height);
            if (!texture.empty())
                passes.push_back(
                    LayerPassOpenCL{ texture, opacity,
                        static_cast<int>(layer->blendMode) });
        });
    // 3. Fusion matérielle OpenCL dans un UMat temporaire
    cv::UMat targetUMat;
    const bool composed = layerCompositorOpenCL->Compose(
        backgroundUMat, passes, targetUMat, width, height);
    // 4. Copie du résultat vers la texture OpenGL d'affichage
    if (composed && !targetUMat.empty())
        UploadToTexture(*textureDisplay, targetUMat.getMat(cv::ACCESS_READ));
    return composed;
}
void CBitmapEditor::CanvasResize(CanvasSizeParameter canvasSize) {
    // Parcourir l'ensemble des calques
    for (size_t i = 0; i < listOfLayer.size(); ++i) {
        LayerElement* layer = listOfLayer[i];
        if (!layer) continue;
        CImageLoadingFormat* pictureFormat = layer->GetPicture();
        if (!pictureFormat) continue;
        [[maybe_unused]] const cv::Mat newCanvas =
            PlaceOnCanvas(pictureFormat->GetMatImage(), canvasSize);
        // Mettre à jour l'image du calque avec le nouveau canvas redimensionné
        // pictureFormat->SetPicture(newCanvas);
    }
    listOfLayer.IsChanged(true);
    UpdateBitmapSizeFromLayers();
    ApplyGeometryChange();
}
void CBitmapEditor::Resize(const int& widthOut, const int& heightOut,
    const int& method) {
    // 1. Parcourir et redimensionner chaque calque de la liste
    for (size_t i = 0; i < listOfLayer.size(); ++i) {
        LayerElement* layer = listOfLayer[i];
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
    listOfLayer.IsChanged(true);
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
    CImageLoadingFormat* source = listOfLayer.GetPictureToShow();
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
        if (listOfLayer.size() > 0) {
            bitmapwidth = listOfLayer.GetWidth();
            bitmapheight = listOfLayer.GetHeight();
            ShrinkImage();
        }
    }
    mouseUpdate = nullptr;
    effectParameter = nullptr;
    loadBitmap = true;
    needToRefresh = true;
}
void CBitmapEditor::SetActifLayer(const int& numLayer) {
    if (numLayer >= 0 && static_cast<size_t>(numLayer) < listOfLayer.size())
    {
        numActifLayer = numLayer;
        loadBitmap = true;
        needToRefresh = true;
    }
}
void CBitmapEditor::RenderTexture(GLTexture* textureDisplay,
    const bool& invertPos, int width,
    int height) {
    if (listOfLayer.size() > 0) {
        int x = (GetWidth() * scale_factor - width) / 2;
        int y = (GetHeight() * scale_factor - height) / 2;
        renderOpenGL->RenderToScreen(mouseUpdate, effectParameter, x, y, invertPos);
        xPosImage = x;
        yPosImage = y;
    }
}
//-----------------------------------------------------------------
// Composition GPU : meme ordre que l'ancienne boucle (index 0 -> dernier),
// memes regles de saut (invisible, opacite 0, image invalide).
//-----------------------------------------------------------------
bool CBitmapEditor::ComposeLayersGPU(GLTexture* textureDisplay, int width,
    int height, bool activeDirty) {
    if (renderOpenGL == nullptr || width <= 0 || height <= 0) return false;
    if (!layerCompositor)
        layerCompositor = std::make_unique<CLayerCompositorGL>();
    if (!layerCompositor->IsValid()) return false;
    // Fond de previsualisation (damier), reconstruit uniquement si la taille de
    // sortie change.
    const cv::Size size(width, height);
    if (!backgroundTexture || backgroundTextureSize != size) {
        auto texture = std::make_unique<GLTexture>();
        if (!UploadToTexture(*texture, PrepareBackgroundMat(width, height)))
            return false;
        backgroundTexture = std::move(texture);
        backgroundTextureSize = size;
    }
    std::vector<LayerPass> passes;
    passes.reserve(listOfLayer.size());
    ForEachVisibleLayer(listOfLayer, [&](size_t index, LayerElement* layer,
        float opacity) {
            GLTexture* texture = IsActiveLayer(index)
                ? GetActiveLayerTexture(width, height, activeDirty)
                : GetLayerTexture(layer, width, height);
            if (texture != nullptr)
                passes.push_back(
                    LayerPass{ texture, opacity,
                        static_cast<int>(layer->blendMode) });
        });
    return layerCompositor->Compose(backgroundTexture.get(), passes,
        textureDisplay, width, height);
}


void CBitmapEditor::SetKey(const int& iKey)
{
    this->iKey = iKey;

    // 1. Raccourcis presse-papiers Ctrl+C / Ctrl+V / Ctrl+X (code de contrôle
    // ASCII ou lettre brute si Ctrl est enfoncé)
    bool (CBitmapEditor:: * clipboardAction)() = nullptr;
    switch (iKey)
    {
    case 3:  case 'C': case 'c': clipboardAction = &CBitmapEditor::Copy;  break;
    case 22: case 'V': case 'v': clipboardAction = &CBitmapEditor::Paste; break;
    case 24: case 'X': case 'x': clipboardAction = &CBitmapEditor::Cut;   break;
    default: break;
    }

    if (clipboardAction != nullptr && wxGetKeyState(WXK_CONTROL))
    {
        (this->*clipboardAction)();
        return;  // Ne pas altérer les curseurs
    }

    // 2. Curseur Zoom de l'outil de déplacement
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

//-----------------------------------------------------------------
// Texture GPU d'un calque non actif (image redimensionnee a la taille de
// sortie). Rechargee uniquement si l'image ou la taille de sortie a change.
//-----------------------------------------------------------------
GLTexture* CBitmapEditor::GetLayerTexture(LayerElement* layer, int width,
    int height) {
    if (layer->TypeLayer() != TYPE_OPENGL) return nullptr;
    auto* layerGL = static_cast<LayerElementGL*>(layer);
    GLTexture* texture = layerGL->GetOpenGLTexture();
    if (texture != nullptr && IsLayerTextureCurrent(layer, width, height))
        return texture;
    CRgbaquad color;
    CFiltreEffet tempFilter(color, renderOpenGL->GetOpenCLContext(),
        layer->GetPicture());
    cv::Mat prepared =
        PrepareLayerSource(layer, width, height, &tempFilter);
    if (prepared.empty()) return nullptr;
    auto* newTexture = new GLTexture();
    if (!UploadToTexture(*newTexture, prepared)) {
        delete newTexture;
        return nullptr;
    }
    layerGL->SetOpenGLTexture(
        newTexture);  // detruit l'ancienne texture (contexte courant)
    MarkLayerTextureCurrent(layer, width, height);
    return newTexture;
}
//-----------------------------------------------------------------
// Texture GPU du calque actif : resultat des filtres/preview (filtreEffet).
// Rechargee uniquement quand filtreEffet a regenere son image ou que la taille
// change.
//-----------------------------------------------------------------
GLTexture* CBitmapEditor::GetActiveLayerTexture(int width, int height,
    bool activeDirty) {
    const cv::Size size(width, height);
    if (activeLayerTexture && !activeDirty && activeLayerTextureSize == size)
        return activeLayerTexture.get();
    cv::Mat prepared = PrepareActiveLayerImage(width, height);
    if (prepared.empty()) return nullptr;
    if (!activeLayerTexture || activeLayerTextureSize != size) {
        activeLayerTexture = std::make_unique<GLTexture>();
        activeLayerTextureSize = size;
    }
    if (!UploadToTexture(*activeLayerTexture, prepared)) {
        activeLayerTexture.reset();
        return nullptr;
    }
    return activeLayerTexture.get();
}
//-----------------------------------------------------------------
// Ancienne composition CPU (conservée comme repli).
//-----------------------------------------------------------------
void CBitmapEditor::ComposeLayersCPU(GLTexture* textureDisplay, int widthOutput,
    int heightOutput) {
    // Fond de prévisualisation opaque (damier).
    cv::Mat composed = PrepareBackgroundMat(widthOutput, heightOutput);
    ForEachVisibleLayer(listOfLayer, [&](size_t index, LayerElement* layer,
        float opacity) {
            cv::Mat layerImage;
            if (IsActiveLayer(index)) {
                // Le calque actif utilise le résultat des filtres/preview.
                layerImage = PrepareActiveLayerImage(widthOutput, heightOutput);
            }
            else {
                CRgbaquad color;
                CFiltreEffet tempFilter(color, nullptr, layer->GetPicture());
                layerImage = PrepareLayerSource(layer, widthOutput, heightOutput,
                    &tempFilter);
            }
            if (layerImage.empty()) return;
            LayerFonction::CompositeLayer(composed, layerImage, opacity,
                layer->blendMode);
        });
    UploadToTexture(*textureDisplay, composed);
}

void CBitmapEditor::OnUpdateLayerBitmap(wxCommandEvent& event) {
    auto picture = static_cast<CImageLoadingFormat*>(event.GetClientData());
    if (picture == nullptr || !picture->IsOk()) return;
    ResetViewState();
    listOfLayer[numActifLayer]->SetPicture(picture);
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
    LayerElement* element = listOfLayer.FindElementByLayerId(layerId);
    if (element == nullptr)
        return;

    element->SetPicture(bitmapIn);
}