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

    if (numActifLayer < 0 || numActifLayer >= listOfLayer.size())
        return;

    LayerElement* layerElement = listOfLayer[numActifLayer];
    loadBitmap = true;
    if (numActifLayer == 1)
    {
        printf("toto");
    }

    Regards::Picture::CPictureArray mat;
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



            }

            updateFilter = false;

            widthOutputOld = widthOutput;
            heightOutputOld = heightOutput;

#ifdef RENDER_OPENGL

            if (bitmapLoad && GetWidth() > 0 && GetHeight() > 0)
            {
                // 1. Initialiser le fond de l'écran (votre damier ou couleur de fond)
                renderOpenGL->CreateScreenRender(GetWidth() * scale_factor, GetHeight() * scale_factor,
                    CRgbaquad(themeBitmap.colorBack.Red(), themeBitmap.colorBack.Green(), themeBitmap.colorBack.Blue()));

                // Activer le Blending natif d'OpenGL pour la gestion de l'opacité et de l'alpha
                glEnable(GL_BLEND);
                glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); // Formule standard Source-Over

                // 2. Parcourir et afficher chaque calque directement sur le GPU
                for (int index = 0; index < listOfLayer.size(); index++)
                {
                    LayerElement* layer = listOfLayer[static_cast<size_t>(index)];
                    if (layer == nullptr || !layer->isVisible)
                        continue;

                    float opacity = static_cast<float>(layer->opacity) / 100.0f;
                    if (opacity <= 0.0f)
                        continue;

                    // Récupérer ou créer la texture OpenGL propre à CE calque
                    GLTexture* layerTexture = layer->GetOpenGLTexture();

                    // Si le calque a été modifié (ou n'a pas encore de texture), on met à jour la texture GPU
                    if (layerTexture == nullptr || layer->needTextureUpdate || index == numActifLayer)
                    {
                        cv::Mat sourceMat;
                        if (index == numActifLayer) {
                            Regards::Picture::CPictureArray activePicture(filtreEffet->GetMatrix());
                            sourceMat = activePicture.getMat();
                        }
                        else {
                            sourceMat = layer->GetPicture()->GetMatImage();
                        }

                        if (sourceMat.empty()) continue;

                        // Si la texture n'existe pas, demandez à votre RenderOpenGL de la créer
                        if (layerTexture == nullptr) {
                            layerTexture = new Regards::OpenGL::GLTexture();
                            layer->SetOpenGLTexture(layerTexture);
                        }

                        // Convertir la matrice en texture GPU (Opération lourde, faite UNIQUEMENT au changement)
                        Regards::Picture::CPictureArray picArray(sourceMat);
                        layerTexture->SetData(picArray, renderOpenGL->GetOpenCLContext());

                        layer->needTextureUpdate = false;
                    }

                    // 3. Configurer le mode de fusion (Blend Mode) sous OpenGL
                    // Note: Pour des modes complexes (Overlay, Multiply), un Shader personnalisé (GLSL) sera requis.
                    // Pour le mode normal / opacité simple :
                    glBlendColor(1.0f, 1.0f, 1.0f, opacity);
                    glBlendFunc(GL_CONSTANT_ALPHA, GL_ONE_MINUS_CONSTANT_ALPHA);

                    // Rendre le calque directement à l'écran
                    RenderTexture(layerTexture, invert, widthOutput, heightOutput);
                }

                glDisable(GL_BLEND);
                AfterRender();
            }
        }
    }

#else

            // 1. Initialiser le fond de l'écran avec la couleur de thème
            renderOpenGL->CreateScreenRender(GetWidth()* scale_factor, GetHeight()* scale_factor,
                CRgbaquad(themeBitmap.colorBack.Red(), themeBitmap.colorBack.Green(),
                    themeBitmap.colorBack.Blue()));

            GLTexture* textureDisplay = renderOpenGL->GetDisplayTexture(widthOutput, heightOutput);
            if (textureDisplay == nullptr)
                return;
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
            /*
            else
            {
                // Le damier de previsualisation doit etre opaque.
                for (int y = 0; y < composed.rows; ++y)
                {
                    auto* row = composed.ptr<cv::Vec4b>(y);
                    for (int x = 0; x < composed.cols; ++x)
                        row[x][3] = 255;
                }
            }
            */


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

            // RenderToScreen() affiche la texture partagee du renderer.
            // On l'appelle une seule fois, apres la composition complete.
            //glDisable(GL_BLEND);
            RenderTexture(textureDisplay, invert, widthOutput, heightOutput);
            
        }

        AfterRender();


    }
#endif

    canvas->SwapBuffers();
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
