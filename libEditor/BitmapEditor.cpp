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

// À insérer à la fin de BitmapEditor.cpp
#include <wx/clipbrd.h>
#include <wx/bitmap.h>
#include <wx/image.h>
using namespace Regards::Sqlite;
using namespace Regards::Viewer;

extern AppContext application_context;
//-----------------------------------------------------------------
// Utilitaires locaux (partagés par les backends GL / OpenCL / CPU)
//-----------------------------------------------------------------
namespace {
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

    // Image d'un calque non actif, préparée à la taille de sortie avec l'effet
    // d'écran appliqué (Resize / Rotation / Flip)
    void LogFallbackOnce(bool& alreadyLogged, const char* message) {
        if (!alreadyLogged) {
            wxLogDebug("%s", message);
            alreadyLogged = true;
        }
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

        // Zone de destination = intersection de l'ancienne image (positionnée) et du
        // nouveau canvas
        const cv::Rect dstROI = cv::Rect(dstX, dstY, oldImage.cols, oldImage.rows) &
            cv::Rect(0, 0, newWidth, newHeight);
        if (!dstROI.empty()) {
            const cv::Rect srcROI(dstROI.x - dstX, dstROI.y - dstY, dstROI.width,
                dstROI.height);
            oldImage(srcROI).copyTo(newCanvas(dstROI));
        }

        return newCanvas;
    }
}  // namespace


cv::UMat CBitmapEditor::PrepareLayerSourceOpenCL(LayerElement* layer, int width,
    int height, CFiltreEffet* filter) {
    CImageLoadingFormat* picture = layer->GetPicture();
    if (picture == nullptr || !picture->IsOk()) return cv::UMat();

    filter->SetBitmap(picture);
    int wOut = width;
    int hOut = height;
    GenerateScreenBitmap(filter, wOut, hOut);

    Regards::Picture::CPictureArray transformPicture = filter->GetMatrix();
    return LayerFonction::PrepareLayerImageOpenCL(transformPicture.getUMat(), width, height, layer->x, layer->y, ratio);
}



// -----------------------------------------------------------------
// COPY : Copie l'image du calque actif dans le presse-papiers
// -----------------------------------------------------------------
// -----------------------------------------------------------------
// COPY : Copie l'image (ou la sélection détourée) dans le presse-papiers
// -----------------------------------------------------------------
bool CBitmapEditor::Copy()
{
    if (numActifLayer < 0 || numActifLayer >= static_cast<int>(listOfLayer.size()))
        return false;

    LayerElement* layerElement = listOfLayer[numActifLayer];
    if (layerElement == nullptr)
        return false;

    CImageLoadingFormat* picture = layerElement->GetPicture();
    if (picture == nullptr || !picture->IsOk())
        return false;

    // Récupération de la matrice OpenCV du calque
    cv::Mat sourceMat = picture->GetMatImage();
    if (sourceMat.empty())
        return false;

    cv::Mat outMat; // Matrice finale qui sera envoyée au presse-papiers

    // VERIFICATION : Y a-t-il une sélection active via l'outil de sélection ?
    if (effectParameter != nullptr && preview == IDM_SELECTFILTER)
    {
        CSelectFilterParameter* selectFilter = (CSelectFilterParameter*)effectParameter;

        if (!selectFilter->listSelections.empty())
        {
            // 1. Déterminer la boîte englobante (ROI) de toutes les sélections cumulées
            int minX = sourceMat.cols, minY = sourceMat.rows;
            int maxX = 0, maxY = 0;

            for (const auto& s : selectFilter->listSelections)
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

            // Sécurité des limites de l'image
            minX = std::max(0, minX);
            minY = std::max(0, minY);
            maxX = std::min(sourceMat.cols, maxX);
            maxY = std::min(sourceMat.rows, maxY);

            int roiWidth = maxX - minX;
            int roiHeight = maxY - minY;

            if (roiWidth > 0 && roiHeight > 0)
            {
                // 2. Création d'un masque global en dur à la taille de l'image (0 = transparent)
                cv::Mat mask = cv::Mat::zeros(sourceMat.size(), CV_8UC1);

                for (const auto& s : selectFilter->listSelections)
                {
                    // Conversion des coordonnées locales
                    cv::Point p1(s.startPoint.x, s.startPoint.y);
                    cv::Point p2(s.endPoint.x, s.endPoint.y);

                    if (s.selectType == SELECT_RECTANGLE)
                    {
                        cv::rectangle(mask, p1, p2, cv::Scalar(255), cv::FILLED);
                    }
                    else if (s.selectType == SELECT_ELLIPSE)
                    {
                        cv::Point center((p1.x + p2.x) / 2, (p1.y + p2.y) / 2);
                        cv::Size axes(std::abs(p1.x - p2.x) / 2, std::abs(p1.y - p2.y) / 2);
                        cv::ellipse(mask, center, axes, 0, 0, 360, cv::Scalar(255), cv::FILLED);
                    }
                    else if (s.selectType == SELECT_LASSO && !s.points.empty())
                    {
                        std::vector<cv::Point> cvPoints;
                        for (const auto& pt : s.points) {
                            cvPoints.push_back(cv::Point(pt.x, pt.y));
                        }
                        std::vector<std::vector<cv::Point>> contours = { cvPoints };
                        cv::fillPoly(mask, contours, cv::Scalar(255));
                    }
                }

                // 3. Forcer l'image source en format 4 canaux (BGRA) pour injecter la transparence
                cv::Mat bgraMat;
                if (sourceMat.channels() == 3) {
                    cv::cvtColor(sourceMat, bgraMat, cv::COLOR_BGR2BGRA);
                }
                else if (sourceMat.channels() == 1) {
                    cv::cvtColor(sourceMat, bgraMat, cv::COLOR_GRAY2BGRA);
                }
                else {
                    bgraMat = sourceMat.clone();
                }

                // Appliquer le masque sur le canal Alpha (Index canal 3 dans BGRA)
                for (int y = 0; y < bgraMat.rows; ++y) {
                    for (int x = 0; x < bgraMat.cols; ++x) {
                        if (mask.at<uchar>(y, x) == 0) {
                            bgraMat.at<cv::Vec4b>(y, x)[3] = 0; // Pixel hors sélection -> Transparent
                        }
                    }
                }

                // 4. Découper la sous-partie ajustée à la boîte englobante de la sélection
                cv::Rect roi(minX, minY, roiWidth, roiHeight);
                outMat = bgraMat(roi).clone();
            }
        }
    }

    // Si aucune sélection n'était active ou valide, on copie l'image entière
    if (outMat.empty())
    {
        outMat = sourceMat;
    }

    // 5. Conversion OpenCV (BGR/BGRA) vers Espace Couleur de wxWidgets (RGB/RGBA)
    cv::Mat rgbMat;
    bool hasAlpha = (outMat.channels() == 4);

    if (hasAlpha) {
        cv::cvtColor(outMat, rgbMat, cv::COLOR_BGRA2RGBA);
    }
    else if (outMat.channels() == 3) {
        cv::cvtColor(outMat, rgbMat, cv::COLOR_BGR2RGB);
    }
    else if (outMat.channels() == 1) {
        cv::cvtColor(outMat, rgbMat, cv::COLOR_GRAY2RGB);
    }
    else {
        return false;
    }

    // 6. Création de l'image et initialisation du canal alpha de wxWidgets si requis
    wxImage wxImg(rgbMat.cols, rgbMat.rows, rgbMat.data, true);

    if (hasAlpha)
    {
        // Allocation du masque alpha pour wxImage
        unsigned char* alphaData = (unsigned char*)malloc(rgbMat.cols * rgbMat.rows);
        if (alphaData != nullptr)
        {
            for (int i = 0; i < rgbMat.cols * rgbMat.rows; ++i) {
                alphaData[i] = rgbMat.data[i * 4 + 3]; // Extraction du pixel A depuis RGBA
            }
            // Passer la propriété à wxImage (il se chargera de faire le free)
            wxImg.SetAlpha(alphaData);
        }
    }

    // Conversion finale en Bitmap pour le presse-papiers
    wxBitmap bitmap(wxImg);

    // 7. Envoi au presse-papiers du système
    if (wxTheClipboard->Open())
    {
        wxTheClipboard->Clear();
        wxTheClipboard->SetData(new wxBitmapDataObject(bitmap));
        wxTheClipboard->Close();
        return true;
    }

    return false;
}


// -----------------------------------------------------------------
// CUT : Copie l'image du calque actif puis efface/vide le calque
// -----------------------------------------------------------------
bool CBitmapEditor::Cut()
{
    // 1. On effectue la copie
    if (!Copy())
        return false;

    // 2. On récupère le calque pour le vider (création d'une matrice noire/vide)
    LayerElement* layerElement = listOfLayer[numActifLayer];
    CImageLoadingFormat* picture = layerElement->GetPicture();

    cv::Mat currentMat = picture->GetMatImage();
    if (!currentMat.empty())
    {
        // Remplace l'image par un fond transparent ou noir de même dimension
        cv::Mat emptyMat = cv::Mat::zeros(currentMat.rows, currentMat.cols, currentMat.type());
        picture->SetPicture(emptyMat);

        // Invalidation des caches et mise à jour géométrique
        layerElement->InvalidatePicture();
        listOfLayer.IsChanged(true);
        ApplyGeometryChange();
    }

    return true;
}

// -----------------------------------------------------------------
// PASTE : Récupère une image du presse-papiers et l'applique au calque actif
// -----------------------------------------------------------------
// -----------------------------------------------------------------
// PASTE : Récupère une image du presse-papiers et l'ajoute dans un NOUVEAU calque
// -----------------------------------------------------------------
bool CBitmapEditor::Paste()
{
    

    bool success = false;

    if (wxTheClipboard->Open())
    {
        // Vérifie si le presse-papiers contient des données de type Image/Bitmap
        if (wxTheClipboard->IsSupported(wxDF_BITMAP))
        {
            wxBitmapDataObject data;
            wxTheClipboard->GetData(data);
            wxBitmap bitmap = data.GetBitmap();

            if (bitmap.IsOk())
            {
                // 1. Conversion wxBitmap -> wxImage pour accéder aux pixels RGB
                wxImage wxImg = bitmap.ConvertToImage();
                int width = wxImg.GetWidth();
                int height = wxImg.GetHeight();

                // 2. Création de la matrice OpenCV (RGB) depuis le buffer wxImage
                cv::Mat rgbMat(height, width, CV_8UC3, (void*)wxImg.GetData());
                cv::Mat bgrMat;

                // Par défaut, nous créons une image au format standard BGR (ou BGRA si vous gérez l'alpha)
                cv::cvtColor(rgbMat, bgrMat, cv::COLOR_RGB2BGR);

                // 3. Création du conteneur d'image pour le nouveau calque
                // Note : Selon votre architecture, assurez-vous que CImageLoadingFormat possède un 
                // constructeur ou une méthode prenant une cv::Mat (ex: CImageLoadingFormat(bgrMat))
                auto* newPicture = new CImageLoadingFormat();
                newPicture->SetPicture(bgrMat);

                // 4. Instanciation du nouveau calque selon le backend par défaut (ex: TYPE_OPENGL ou TYPE_OPENCL)
                // Nous récupérons le type depuis le calque actif pour conserver une cohérence de rendu
                int currentTypeLayer = TYPE_OPENGL;
                if (numActifLayer >= 0 && static_cast<size_t>(numActifLayer) < listOfLayer.size()) {
                    currentTypeLayer = listOfLayer[numActifLayer]->TypeLayer();
                }

                LayerElement* newLayer = nullptr;
                if (currentTypeLayer == TYPE_OPENCL) {
                    newLayer = new LayerElementUMat(); // Si backend OpenCL
                }
                else {
                    newLayer = new LayerElementGL();   // Si backend OpenGL
                }

                // Initialisation du calque
                newLayer->SetPicture(newPicture);
                newLayer->layerName = wxString::Format("Calque %d (Collé)", static_cast<int>(listOfLayer.size() + 1));
                newLayer->isVisible = true;
                newLayer->opacity = 100;
                newLayer->blendMode = LayerBlendMode::Normal; // Mode Normal par défaut
                newLayer->needTextureUpdate = true;

                // 5. Ajout du calque à la liste et mise à jour de l'index actif
                // Note : Utilisez la méthode d'insertion ou d'ajout de votre classe CLayerList (ex: push_back ou Add)
                listOfLayer.push_back(newLayer);
                numActifLayer = static_cast<int>(listOfLayer.size()) - 1; // Le calque collé devient le calque actif

                success = true;
            }
        }
        wxTheClipboard->Close();
    }

    if (success)
    {
        // Notification et rafraîchissement global de la vue de l'éditeur
        listOfLayer.IsChanged(true);
        UpdateBitmapSizeFromLayers();
        ApplyGeometryChange();

        // Envoi d'un événement pour notifier l'UI globale (arbre des calques, etc.) qu'un calque a été ajouté
        wxWindow* mainWindow = parentRender->FindWindowById(FRAMEEDITOR_ID);
        if (mainWindow) {
            wxCommandEvent evt(wxEVENT_UPDATELAYERPICTURE); // Réutilisation ou création d'un événement adapté
            mainWindow->GetEventHandler()->AddPendingEvent(evt);
        }
        return true;
    }

    return false;
}



cv::Mat CBitmapEditor::PrepareLayerSource(LayerElement* layer, int width,
    int height, CFiltreEffet* filter) {
    CImageLoadingFormat* picture = layer->GetPicture();
    if (picture == nullptr || !picture->IsOk()) return cv::Mat();

    filter->SetBitmap(picture);
    int wOut = width;
    int hOut = height;
    GenerateScreenBitmap(filter, wOut, hOut);

    Regards::Picture::CPictureArray transformPicture(filter->GetMatrix());
    return LayerFonction::PrepareLayerImage(transformPicture.getMat(), width, height, layer->x, layer->y, ratio);
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
// Régénère le bitmap d'écran du calque actif (filtres / preview /
// redimensionnement). Retourne true si le résultat du calque actif a été
// régénéré.
//-----------------------------------------------------------------
bool CBitmapEditor::UpdateScreenBitmap(LayerElement* activeLayer,
    int& widthOutput, int& heightOutput,
    bool bitmapIsLoad) {
    updateFilter = forceUpdateFilter || preview > 0 || bitmapIsLoad;

    if (updateFilter) {
        if (!bitmapIsLoad) filtreEffet->SetBitmap(activeLayer->GetPicture());
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

    if (numActifLayer < 0 || numActifLayer >= listOfLayer.size()) return;

    LayerElement* layerElement = listOfLayer[numActifLayer];
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
// taille de sortie.
cv::Mat CBitmapEditor::PrepareActiveLayerImage(int width, int height) {
    Regards::Picture::CPictureArray activePicture(filtreEffet->GetMatrix());
    // Remplacer la ligne finale par :
    LayerElement* layer = (numActifLayer >= 0 && numActifLayer < listOfLayer.size()) ? listOfLayer[numActifLayer] : nullptr;
    int lx = layer ? layer->x : 0;
    int ly = layer ? layer->y : 0;
    return LayerFonction::PrepareLayerImage(activePicture.getMat(), width, height, lx, ly, ratio);
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
    cv::Mat prepared = PrepareActiveLayerImage(width, height);
    if (prepared.empty()) return cv::UMat();
    activeLayerUMat = prepared.getUMat(cv::ACCESS_READ);
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
        // renderOpenGL->RenderQuad(textureDisplay, x, y, invertPos);
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

    // 1. Interception des raccourcis clavier Ctrl+C, Ctrl+V, Ctrl+X
    // On vérifie à la fois les caractères de contrôle (ASCII / wxWidgets) et les codes standards.
    switch (iKey)
    {
    case 3:          // Valeur ASCII / Control Code pour Ctrl+C
    case 'C':        // Au cas où le raccourci remonte en lettre brute
    case 'c':
        // Pour s'assurer que Ctrl est bien enfoncé si on reçoit la lettre brute
        if (wxGetKeyState(WXK_CONTROL))
        {
            this->Copy();
            return; // On arrête le traitement pour ne pas altérer les curseurs
        }
        break;

    case 22:         // Valeur ASCII / Control Code pour Ctrl+V (Synchronisé avec WXK_CONTROL_V si défini)
    case 'V':
    case 'v':
        if (wxGetKeyState(WXK_CONTROL))
        {
            this->Paste();

            SetSelectMoveTool();
            return;
        }
        break;

    case 24:         // Valeur ASCII / Control Code pour Ctrl+X
    case 'X':
    case 'x':
        if (wxGetKeyState(WXK_CONTROL))
        {
            this->Cut();
            return;
        }
        break;
    }

    // 2. Gestion d'origine pour l'outil de déplacement et le changement de curseur Zoom
    switch (toolOption)
    {
    case MOVEPICTURE:
    {
        if (iKey == WXK_CONTROL)
            wxSetCursor(hCursorZoomIn);
        else
            wxSetCursor(hCursorZoomOut);
        break;
    }
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
// Ancienne composition CPU (conservee comme repli).
//-----------------------------------------------------------------
void CBitmapEditor::ComposeLayersCPU(GLTexture* textureDisplay, int widthOutput,
    int heightOutput) {
    // Fond de previsualisation opaque (damier).
    cv::Mat composed = PrepareBackgroundMat(widthOutput, heightOutput);
    ForEachVisibleLayer(listOfLayer, [&](size_t index, LayerElement* layer,
        float opacity) {
            cv::Mat layerImage;
            if (IsActiveLayer(index)) {
                // Le calque actif doit utiliser le resultat des filtres/preview.
                Regards::Picture::CPictureArray activePicture(filtreEffet->GetMatrix());
                const cv::Mat activeMat = activePicture.getMat().clone();
                if (activeMat.empty()) return;
                layerImage = LayerFonction::PrepareLayerImage(activeMat, widthOutput,
                    heightOutput);
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
    wxWindow* mainWindow = parentRender->FindWindowById(FRAMEEDITOR_ID);
    if (mainWindow) {
        wxCommandEvent evt(wxEVENT_UPDATELAYERPICTURE);
        mainWindow->GetEventHandler()->AddPendingEvent(evt);
    }
}

void CBitmapEditor::SetSelectMoveTool()
{
    preview = WM_SELECTMOVE;
    isInUse = true;
    m_cDessin.reset(nullptr);
    if (m_cDessin != nullptr)
    {
        forceUpdateFilter = true;
        toolOption = 0;
        SetDessinRatio();
        m_cDessin->SetCursor();
        isInUse = false;
        SetTool(preview);
    }
    else
    {
        forceUpdateFilter = false;
        wxSetCursor(*wxHOURGLASS_CURSOR);
        toolOption = WM_SELECTMOVE;
    }
}

void CBitmapEditor::SetZoomTool()
{
    preview = WM_SELECTZOOM;
    isInUse = true;

    m_cDessin.reset(nullptr);
    if (m_cDessin != nullptr)
    {
        forceUpdateFilter = true;
        toolOption = 0;
        SetDessinRatio();
        m_cDessin->SetCursor();
        isInUse = false;
        SetTool(preview);
    }
    else
    {
        forceUpdateFilter = false;
        wxSetCursor(hCursorZoomIn);
        toolOption = ZOOMPICTURE;
    }
}

void CBitmapEditor::SetMoveTool()
{
    preview = WM_MOVEPICTURE;
    isInUse = true;

    m_cDessin.reset(nullptr);
    if (m_cDessin != nullptr)
    {
        forceUpdateFilter = true;
        toolOption = 0;
        SetDessinRatio();
        m_cDessin->SetCursor();
        isInUse = false;
        SetTool(preview);
    }
    else
    {
        forceUpdateFilter = false;
        wxSetCursor(*wxSTANDARD_CURSOR);
        toolOption = MOVEPICTURE;
    }
}


void CBitmapEditor::OnSetBitmap(wxCommandEvent& event)
{
    auto picture = static_cast<CImageLoadingFormat*>(event.GetClientData());
    uint64_t layerId = static_cast<uint64_t>(event.GetExtraLong());
    if (picture != nullptr)
        SetBitmapByLayerId(picture, layerId);
}



//-----------------------------------------------------------------
//Affectation de l'image
//-----------------------------------------------------------------
void CBitmapEditor::SetBitmapByLayerId(CImageLoadingFormat* bitmapIn, const uint64_t & layerId)
{
    if (bitmapIn != nullptr)
    {
        if (bitmapIn->IsOk())
        {
            loadBitmap = true;
            bitmapLoad = true;
            filename = bitmapIn->GetFilename();
            bitmapUpdate = true;
            flipVertical = 0;
            flipHorizontal = 0;
            angle = 0;
            UpdateLayerId(bitmapIn, layerId);

            toolOption = MOVEPICTURE;
            bitmapwidth = bitmapIn->GetWidth();
            bitmapheight = bitmapIn->GetHeight();
            orientation = bitmapIn->GetOrientation();


            CSqlPhotos sqlPhotos;
            int exifCode = sqlPhotos.GetPhotoExif(filename);
            if (exifCode != -1)
                application_context.GetAngleAndFlip(exifCode, angle, flipHorizontal, flipVertical);

            //int exifCode = application_context.GetExifFromAngleAndFlip(angle, flipHorizontal, flipVertical);
            //sqlPhotos.UpdatePhotoExif(filename, exifCode);

            ShrinkImage(false);
            AfterSetBitmap();

            RemoveListener(false);

            //needToRefresh = true;
            parentRender->Refresh();

            RefreshWindow();
        }
    }
}


void CBitmapEditor::UpdateLayerId(CImageLoadingFormat* bitmapIn, const uint64_t& layerId)
{
    CRegardsConfigParam* regardsParam = CParamInit::getInstance();
    const bool openCLAvailable = cv::ocl::haveOpenCL() && regardsParam->GetIsOpenCLSupport();
    bool addElement = false;
    LayerElement* element = nullptr;

    element = listOfLayer.FindElementByLayerId(layerId);

    if (element == nullptr)
        return;

    element->SetPicture(bitmapIn);
}