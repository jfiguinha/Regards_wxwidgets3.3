#include <header.h>
#include "BitmapEditor.h"
#include <ViewerTheme.h>
#include <FiltreEffet.h>
#include <ViewerThemeInit.h>
#include <RGBAQuad.h>
#include <ImageLoadingFormat.h>
#include <Draw.h>
#include <LayerElement.h>
#include <libPicture.h>
#include <ParamInit.h>
#include <RegardsConfigParam.h>
#include <GLTexture.h>
#include <LayerCompositorGL.h>
#include <vector>
using namespace Regards::Viewer;

#define RENDER_OPENGL

CBitmapEditor::CBitmapEditor(CSliderInterface* slider, wxWindowID mainViewerId, const CThemeBitmapWindow& theme,
	CBitmapInterface* bitmapInterface) : Regards::Control::CBitmapWndViewer(slider, mainViewerId, theme, bitmapInterface)
{
	fixArrow = false;
}

bool CBitmapEditor::ApplySelectEffect(int& widthOutput, int& heightOutput)
{ 
	return false; 
}

CBitmapEditor::~CBitmapEditor(void)
{}

void CBitmapEditor::SetRealSize()
{
	ratio = 1.0f;
	this->RefreshWindow();
}

void CBitmapEditor::SetFilename(const wxString& filename)
{
	this->filename = filename;
}


//-----------------------------------------------------------------
//Dessin de l'image
//-----------------------------------------------------------------
void CBitmapEditor::OnPaint3D(wxGLCanvas* canvas, CRenderOpenGL* renderOpenGL)
{
#ifndef WIN32
    scale_factor = parentRender->GetContentScaleFactor();
#endif

    // Contexte GL courant : on libere ici les textures des calques supprimes.
    listOfLayer.FlushPendingTextureDeletes();

    if (numActifLayer < 0 || numActifLayer >= listOfLayer.size())
        return;

    LayerElement* layerElement = listOfLayer[numActifLayer];
    if (layerElement == nullptr)
        return;

    // filtreEffet ne doit etre recharge que si le calque actif ou son image a change
    // (changement de calque, SetPicture, Resize...). Avant, loadBitmap etait force a
    // true a chaque frame, ce qui relancait tout le pipeline de filtres a chaque paint.
    if (layerElement->layerId != lastSourceLayerId || layerElement->pictureVersion != lastSourceVersion)
    {
        loadBitmap = true;
        lastSourceLayerId = layerElement->layerId;
        lastSourceVersion = layerElement->pictureVersion;
    }

    CImageLoadingFormat* source = layerElement->GetPicture();

    isOpenGLShow = true;

    if (renderBitmapOpenGL == nullptr)
    {
        this->renderOpenGL = renderOpenGL;
        renderBitmapOpenGL = std::make_unique<CRenderBitmapOpenGL>(renderOpenGL);
        renderBitmapOpenGL->LoadingResource(scale_factor, themeBitmap.colorArrow);
    }



    if (renderBitmapOpenGL != nullptr)
    {
        CRgbaquad color;
        CRegardsConfigParam* regardsParam = CParamInit::getInstance();
        bool useCuda = regardsParam->GetIsUseCuda();
        bool useOpenCL = regardsParam->GetIsOpenCLSupport();

        int widthOutput = static_cast<int>(GetBitmapWidthWithRatio()) * scale_factor;
        int heightOutput = static_cast<int>(GetBitmapHeightWithRatio()) * scale_factor;


        bool invert = true;
        bool bitmapIsLoad = false;
        // true quand le resultat des filtres (calque actif) a ete regenere dans cette frame.
        bool activeDirty = false;

        if (loadBitmap)
        {
            if (filtreEffet == nullptr)
                filtreEffet = std::make_unique<CFiltreEffet>(color, renderOpenGL->GetOpenCLContext(), source);
            else
            {
                filtreEffet->SetBitmap(source);
            }

            loadBitmap = false;
            bitmapIsLoad = true;
        }

        if (bitmapLoad && GetWidth() > 0 && GetHeight() > 0)
        {
            if (widthOutput < 0 || heightOutput < 0)
                return;

            // Application de votre logique d'optimisation
// Si la mise à jour est forcée OU qu'un effet de prévisualisation est actif (preview > 0),
// on doit obligatoirement lever le flag pour rafraîchir la source brute.
            if (forceUpdateFilter || preview > 0 || bitmapIsLoad)
            {
                updateFilter = true;
            }
            else
            {
                updateFilter = false;
            }


            if (updateFilter)// || mouseUpdate != nullptr)
            {
                if (!bitmapIsLoad)
                    filtreEffet->SetBitmap(source);
                BeforeInterpolationBitmap();
                updateFilter = true;
            }


            //printf("widthOutput : %d heightOutput %d \n", widthOutput, heightOutput);
            if (updateFilter || widthOutputOld != widthOutput || heightOutputOld != heightOutput)
            {
                GenerateScreenBitmap(filtreEffet.get(), widthOutput, heightOutput);

                ApplyPreviewEffect(widthOutput, heightOutput);

                ApplySelectEffect(widthOutput, heightOutput);

                activeDirty = true;
            }

            updateFilter = false;

            widthOutputOld = widthOutput;
            heightOutputOld = heightOutput;

            // 1. Initialiser le fond de l'écran avec la couleur de thème
            renderOpenGL->CreateScreenRender(GetWidth()* scale_factor, GetHeight()* scale_factor,
                CRgbaquad(themeBitmap.colorBack.Red(), themeBitmap.colorBack.Green(),
                    themeBitmap.colorBack.Blue()));

            GLTexture* textureDisplay = renderOpenGL->GetDisplayTexture(widthOutput, heightOutput);
            if (textureDisplay == nullptr)
                return;

            // Composition GPU des calques directement dans la texture d'affichage ;
            // repli sur l'ancienne composition CPU si le GPU n'est pas utilisable.
            if (!ComposeLayersGPU(textureDisplay, widthOutput, heightOutput, activeDirty))
            {
                if (!gpuComposeFallbackLogged)
                {
                    wxLogDebug("CBitmapEditor : composition GPU des calques indisponible, repli CPU");
                    gpuComposeFallbackLogged = true;
                }
                ComposeLayersCPU(textureDisplay, widthOutput, heightOutput);
            }

            // RenderToScreen() affiche la texture partagee du renderer.
            // On l'appelle une seule fois, apres la composition complete.
            //glDisable(GL_BLEND);
            RenderTexture(textureDisplay, invert, widthOutput, heightOutput);
            
        }

        AfterRender();
    }


    canvas->SwapBuffers();
}


//-----------------------------------------------------------------
// Texture GPU d'un calque non actif (image redimensionnee a la taille de sortie).
// Rechargee uniquement si l'image ou la taille de sortie a change.
//-----------------------------------------------------------------
GLTexture* CBitmapEditor::GetLayerTexture(LayerElement* layer, int width, int height)
{
    GLTexture* texture = layer->GetOpenGLTexture();
    if (texture != nullptr && !layer->needTextureUpdate &&
        layer->textureWidth == width && layer->textureHeight == height)
        return texture;

    CImageLoadingFormat* picture = layer->GetPicture();
    if (picture == nullptr || !picture->IsOk())
        return nullptr;

    cv::Mat prepared = LayerFonction::PrepareLayerImage(picture->GetMatImage(), width, height);
    if (prepared.empty())
        return nullptr;

    auto* newTexture = new GLTexture();
    Regards::Picture::CPictureArray array(prepared);
    if (!newTexture->SetData(array, renderOpenGL->GetOpenCLContext()))
    {
        delete newTexture;
        return nullptr;
    }

    layer->SetOpenGLTexture(newTexture); // detruit l'ancienne texture (contexte courant)
    layer->needTextureUpdate = false;
    layer->textureWidth = width;
    layer->textureHeight = height;
    return newTexture;
}

//-----------------------------------------------------------------
// Texture GPU du calque actif : resultat des filtres/preview (filtreEffet).
// Rechargee uniquement quand filtreEffet a regenere son image ou que la taille change.
//-----------------------------------------------------------------
GLTexture* CBitmapEditor::GetActiveLayerTexture(int width, int height, bool activeDirty)
{
    if (activeLayerTexture && !activeDirty &&
        activeLayerTextureWidth == width && activeLayerTextureHeight == height)
        return activeLayerTexture.get();

    Regards::Picture::CPictureArray activePicture(filtreEffet->GetMatrix());
    cv::Mat prepared = LayerFonction::PrepareLayerImage(activePicture.getMat(), width, height);
    if (prepared.empty())
        return nullptr;

    if (!activeLayerTexture || activeLayerTextureWidth != width || activeLayerTextureHeight != height)
    {
        activeLayerTexture = std::make_unique<GLTexture>();
        activeLayerTextureWidth = width;
        activeLayerTextureHeight = height;
    }

    Regards::Picture::CPictureArray array(prepared);
    if (!activeLayerTexture->SetData(array, renderOpenGL->GetOpenCLContext()))
    {
        activeLayerTexture.reset();
        return nullptr;
    }
    return activeLayerTexture.get();
}

//-----------------------------------------------------------------
// Composition GPU : meme ordre que l'ancienne boucle (index 0 -> dernier),
// memes regles de saut (invisible, opacite 0, image invalide).
//-----------------------------------------------------------------
bool CBitmapEditor::ComposeLayersGPU(GLTexture* textureDisplay, int width, int height, bool activeDirty)
{
    if (renderOpenGL == nullptr || width <= 0 || height <= 0)
        return false;

    if (!layerCompositor)
        layerCompositor = std::make_unique<CLayerCompositorGL>();
    if (!layerCompositor->IsValid())
        return false;

    // Fond de previsualisation opaque (damier), identique a la version CPU.
    if (pictureBackground.empty() ||
        pictureBackground.getWidth() != width ||
        pictureBackground.getHeight() != height)
    {
        pictureBackground.SetArray(
            Regards::Picture::CLibPicture::CreateCheckerboardBackground(width, height));
    }

    if (!backgroundTexture || backgroundTextureWidth != width || backgroundTextureHeight != height)
    {
        cv::Mat background = LayerFonction::PrepareLayerImage(pictureBackground.getMat(), width, height);
        if (background.empty())
            background = cv::Mat(height, width, CV_8UC4, cv::Scalar(128, 128, 128, 255));

        auto texture = std::make_unique<GLTexture>();
        Regards::Picture::CPictureArray array(background);
        if (!texture->SetData(array, renderOpenGL->GetOpenCLContext()))
            return false;

        backgroundTexture = std::move(texture);
        backgroundTextureWidth = width;
        backgroundTextureHeight = height;
    }

    std::vector<LayerPass> passes;
    passes.reserve(listOfLayer.size());

    for (size_t index = 0; index < listOfLayer.size(); index++)
    {
        LayerElement* layer = listOfLayer[index];
        if (layer == nullptr || !layer->isVisible)
            continue;

        const float opacity = LayerFonction::Clamp01(static_cast<float>(layer->opacity) / 100.0f);
        if (opacity <= 0.0f)
            continue;

        GLTexture* texture = (static_cast<int>(index) == numActifLayer)
            ? GetActiveLayerTexture(width, height, activeDirty)
            : GetLayerTexture(layer, width, height);
        if (texture == nullptr)
            continue;

        passes.push_back(LayerPass{ texture, opacity, static_cast<int>(layer->blendMode) });
    }

    const bool composed = layerCompositor->Compose(backgroundTexture.get(), passes, textureDisplay, width, height);

    listOfLayer.FlushPendingTextureDeletes();
    return composed;
}

//-----------------------------------------------------------------
// Ancienne composition CPU (conservee telle quelle comme repli).
//-----------------------------------------------------------------
void CBitmapEditor::ComposeLayersCPU(GLTexture* textureDisplay, int widthOutput, int heightOutput)
{
            // Fond de previsualisation opaque (damier).
            if (pictureBackground.empty() ||
                pictureBackground.getWidth() != widthOutput ||
                pictureBackground.getHeight() != heightOutput)
            {
                pictureBackground.SetArray(
                    Regards::Picture::CLibPicture::CreateCheckerboardBackground(
                        widthOutput, heightOutput));
            }

            cv::Mat composed = LayerFonction::PrepareLayerImage(
                pictureBackground.getMat(), widthOutput, heightOutput);

            if (composed.empty())
            {
                composed = cv::Mat(heightOutput, widthOutput, CV_8UC4,
                    cv::Scalar(128, 128, 128, 255));
            }

            for (int index = 0;
                index < listOfLayer.size(); index++)
            {
                LayerElement* layer = listOfLayer[static_cast<size_t>(index)];
                if (layer == nullptr || !layer->isVisible)
                    continue;

                const float opacity = LayerFonction::Clamp01(
                    static_cast<float>(layer->opacity) / 100.0f);
                if (opacity <= 0.0f)
                    continue;

                cv::Mat sourceMat;
                if (index == numActifLayer)
                {
                    Regards::Picture::CPictureArray activePicture(filtreEffet->GetMatrix());
                    const cv::Mat activeMat = activePicture.getMat().clone();

                    // Le calque actif doit utiliser le resultat des filtres/preview.
                    if (activeMat.empty())
                        continue;
                    sourceMat = activeMat;
                }
                else
                {
                    CImageLoadingFormat* picture = layer->GetPicture();
                    if (picture == nullptr || !picture->IsOk())
                        continue;

                    sourceMat = picture->GetMatImage();
                }

                cv::Mat layerImage = LayerFonction::PrepareLayerImage(
                    sourceMat, widthOutput, heightOutput);
                if (layerImage.empty())
                    continue;

                LayerFonction::CompositeLayer(
                    composed, layerImage, opacity, layer->blendMode);
            }

            Regards::Picture::CPictureArray finalPicture(composed);
            textureDisplay->SetData(finalPicture, renderOpenGL->GetOpenCLContext());
}


void CBitmapEditor::RenderTexture(GLTexture* textureDisplay, const bool& invertPos, int width, int height)
{
    if (listOfLayer.size() > 0)
    {
        int x = (GetWidth() * scale_factor - width) / 2;
        int y = (GetHeight() * scale_factor - height) / 2;

       renderOpenGL->RenderToScreen(mouseUpdate, effectParameter, x, y, invertPos);


        //renderOpenGL->RenderQuad(textureDisplay, x, y, invertPos);

        xPosImage = x;
        yPosImage = y;
    }
}


void CBitmapEditor::ApplyEffectOnMouseRelease()
{
    wxWindow* parameterWindow = parentRender->FindWindowById(PARAMETERWINDOWID);
    if (parameterWindow)
    {
        wxCommandEvent evt(wxEVENT_FILTREOK);
        parameterWindow->GetEventHandler()->AddPendingEvent(evt);
    }
}

void CBitmapEditor::RemoveListener(const bool& applyCancel)
{
    if (m_cDessin)
        if (m_cDessin->ApplyEffectOnMouseRelease())
            return;

    if (mouseUpdate != nullptr && applyCancel)
    {
        mouseUpdate->CancelPreview(this);
        updateFilter = true;
        if (listOfLayer.size() > 0)
        {
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

void CBitmapEditor::SetActifLayer(const int& numLayer)
{
    if(listOfLayer.size() > numLayer && numLayer != -1)
        numActifLayer = numLayer;
}

void CBitmapEditor::OnUpdateLayerBitmap(wxCommandEvent& event)
{
    //int numLayer = event.GetInt();
    auto picture = static_cast<CImageLoadingFormat*>(event.GetClientData());
    if (picture != nullptr)
    {
        if (picture != nullptr)
        {
            if (picture->IsOk())
            {

                loadBitmap = true;
                bitmapLoad = true;

                bitmapUpdate = true;
                flipVertical = 0;
                flipHorizontal = 0;
                angle = 0;
                listOfLayer[numActifLayer]->SetPicture(picture);
                //listOfLayer.UpdatePictureToShow();

                wxWindow* mainWindow = parentRender->FindWindowById(FRAMEEDITOR_ID);
                if (mainWindow)
                {
                    wxCommandEvent evt(wxEVENT_UPDATELAYERPICTURE);
                    mainWindow->GetEventHandler()->AddPendingEvent(evt);
                }
            }
        }
    }
}

void CBitmapEditor::CanvasResize(CanvasSizeParameter canvasSize)
{
    // 1. Déterminer les nouvelles dimensions
    int newWidth = canvasSize.width;
    int newHeight = canvasSize.height;

    // 2. Parcourir l'ensemble des calques (LayerElement) contenus dans listOfLayer
    for (size_t i = 0; i < listOfLayer.size(); ++i)
    {
        LayerElement* layer = listOfLayer[i];
        if (!layer) continue;

        CImageLoadingFormat* pictureFormat = layer->GetPicture();
        if (!pictureFormat) continue;

        // Récupérer la matrice OpenCV actuelle et ses dimensions
        cv::Mat oldImage = pictureFormat->GetMatImage();
        int oldWidth = oldImage.cols;
        int oldHeight = oldImage.rows;

        // Créer une nouvelle zone de travail transparente ou noire (selon votre type d'image, ici en RGBA/8UC4)
        // Si vous utilisez du RVB simple, utilisez cv::Scalar(0, 0, 0) et le bon type.
        cv::Mat newCanvas = cv::Mat::zeros(newHeight, newWidth, oldImage.type());

        // 3. Calculer les coordonnées de destination (X, Y) basées sur l'ancre choisie
        int dstX = 0;
        int dstY = 0;

        switch (canvasSize.position)
        {
        case CanvasSizeParameter::Position::TopLeft:
            dstX = 0;
            dstY = 0;
            break;

        case CanvasSizeParameter::Position::Center:
            dstX = (newWidth - oldWidth) / 2;
            dstY = (newHeight - oldHeight) / 2;
            break;

        case CanvasSizeParameter::Position::TopRight:
            dstX = newWidth - oldWidth;
            dstY = 0;
            break;

        case CanvasSizeParameter::Position::BottomLeft:
            dstX = 0;
            dstY = newHeight - oldHeight;
            break;

        case CanvasSizeParameter::Position::BottomRight:
            dstX = newWidth - oldWidth;
            dstY = newHeight - oldHeight;
            break;
        }

        // 4. Gérer les cas de débordement ou de recadrage (clipping)
        int srcX = 0;
        int srcY = 0;
        int copyWidth = oldWidth;
        int copyHeight = oldHeight;

        // Si l'ancienne image sort à gauche ou en haut du nouveau canvas
        if (dstX < 0) { srcX = -dstX; copyWidth += dstX; dstX = 0; }
        if (dstY < 0) { srcY = -dstY; copyHeight += dstY; dstY = 0; }

        // Si l'ancienne image dépasse à droite ou en bas du nouveau canvas
        if (dstX + copyWidth > newWidth) { copyWidth = newWidth - dstX; }
        if (dstY + copyHeight > newHeight) { copyHeight = newHeight - dstY; }

        // 5. Copier les pixels de l'ancienne image vers la nouvelle zone si une zone valide existe
        if (copyWidth > 0 && copyHeight > 0)
        {
            cv::Rect srcROI(srcX, srcY, copyWidth, copyHeight);
            cv::Rect dstROI(dstX, dstY, copyWidth, copyHeight);

            oldImage(srcROI).copyTo(newCanvas(dstROI));
        }

        // Mettre à jour l'image du calque avec le nouveau canvas redimensionné
       // pictureFormat->SetPicture(newCanvas);
    }

    listOfLayer.IsChanged(true);

    // 6. Mettre à jour les variables de taille de l'éditeur sur la base de l'image principale
    CImageLoadingFormat* source = listOfLayer.GetPictureToShow();
    if (source)
    {
        bitmapwidth = source->GetWidth();
        bitmapheight = source->GetHeight();
        orientation = source->GetOrientation();
    }

    // 7. Forcer le rafraîchissement de l'interface graphique (repris de votre méthode Resize)
    loadBitmap = true;
    bitmapLoad = true;
    bitmapUpdate = true;
    flipVertical = 0;
    flipHorizontal = 0;
    angle = 0;
    toolOption = MOVEPICTURE;

    ShrinkImage(false);
    AfterSetBitmap();
    RemoveListener(false);

    if (parentRender)
        parentRender->Refresh();

    RefreshWindow();
}

void CBitmapEditor::Resize(const int& widthOut, const int& heightOut, const int& method)
{
    // 1. Parcourir et redimensionner chaque calque de la liste
    for (size_t i = 0; i < listOfLayer.size(); ++i)
    {
        LayerElement* layer = listOfLayer[i];
        if (!layer) continue;

        CImageLoadingFormat* layerSource = layer->GetPicture();
        if (!layerSource) continue;

        // Initialisation des filtres d'effet OpenCV pour le calque actuel
        CRgbaquad color;
        CFiltreEffet filtreEffet(color, nullptr, layerSource);

        // Calcul du redimensionnement via OpenCV
        cv::Mat output = filtreEffet.Resize(widthOut, heightOut, method);

        // Appliquer la nouvelle matrice redimensionnée au calque
        layerSource->SetPicture(output);

        // L'image GPU du calque est maintenant perimee.
        layer->InvalidatePicture();
    }

    listOfLayer.IsChanged(true);

    // 2. Récupérer l'image globale/principale pour mettre à jour les métadonnées du Viewer
    CImageLoadingFormat* source = listOfLayer.GetPictureToShow();
    if (source)
    {
        bitmapwidth = source->GetWidth();
        bitmapheight = source->GetHeight();
        orientation = source->GetOrientation();
    }
    else
    {
        // Sécurité si GetPictureToShow() utilise une logique différente ou est nul
        bitmapwidth = widthOut;
        bitmapheight = heightOut;
        orientation = 1; // Orientation par défaut (Normal)
    }

    // 3. Réinitialisation des états et rafraîchissement de l'interface
    loadBitmap = true;
    bitmapLoad = true;
    bitmapUpdate = true;
    flipVertical = 0;
    flipHorizontal = 0;
    angle = 0;

    toolOption = MOVEPICTURE;

    ShrinkImage(false);
    AfterSetBitmap();

    RemoveListener(false);

    if (parentRender)
    {
        parentRender->Refresh();
    }

    RefreshWindow();
}
