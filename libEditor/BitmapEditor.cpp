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

            // 1. Initialiser le fond de l'écran avec la couleur de thème
            renderOpenGL->CreateScreenRender(GetWidth()* scale_factor, GetHeight()* scale_factor,
                CRgbaquad(themeBitmap.colorBack.Red(), themeBitmap.colorBack.Green(),
                    themeBitmap.colorBack.Blue()));

            // ────────────── EXPLICATION ──────────────
            // Activation globale du Blending pour cumuler les calques transparents
            glEnable(GL_BLEND);
            // Formule standard de transparence : gère le canal Alpha source et inverse de la destination
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
            // Évite les artéfacts si les fonctions internes modifient l'équation de blend
            glBlendEquation(GL_FUNC_ADD);

            GLTexture* textureDisplay = renderOpenGL->GetDisplayTexture(widthOutput, heightOutput);

            if (pictureBackground.empty() || pictureBackground.getHeight() != textureDisplay->GetHeight() || pictureBackground.getWidth() != textureDisplay->GetWidth())
            {
                pictureBackground.SetArray(Regards::Picture::CLibPicture::CreateCheckerboardBackground(textureDisplay->GetWidth(), textureDisplay->GetHeight()));
            }

            textureDisplay->SetData(pictureBackground, renderOpenGL->GetOpenCLContext());

            // Désactivation temporaire du blending pour le fond opaque en damier afin d'optimiser les performances
            glDisable(GL_BLEND);
            RenderTexture(textureDisplay, invert, widthOutput, heightOutput);
            glEnable(GL_BLEND); // Réactivation pour l'empilement des vrais calques

                        // 2. Parcourir de l'arrière-plan vers le premier plan
            for (int numLayer = 0; numLayer < listOfLayer.size(); numLayer++)
            {
                LayerElement* layer = listOfLayer[numLayer];
                if (!layer || !layer->isVisible) continue;

                GLTexture* textureToRender = nullptr;

                if (numLayer == numActifLayer)
                {
                    Regards::Picture::CPictureArray mat(filtreEffet->GetMatrix());
                    textureDisplay->SetData(mat, renderOpenGL->GetOpenCLContext());
                    textureToRender = textureDisplay;
                }
                else
                {
                    if (layer->GetPicture() && layer->GetPicture()->IsOk())
                    {
                        if (layer->GetOpenGLTexture() == nullptr || layer->needTextureUpdate)
                        {
                            cv::Mat layerMat = layer->GetPicture()->GetMatImage();
                            GLTexture* newTex = renderOpenGL->GetDisplayTexture(layerMat.cols, layerMat.rows);

                            cv::Mat bgra;
                            if (layerMat.channels() == 4)       bgra = layerMat;
                            else if (layerMat.channels() == 3)  cv::cvtColor(layerMat, bgra, cv::COLOR_BGR2BGRA);
                            else if (layerMat.channels() == 1)  cv::cvtColor(layerMat, bgra, cv::COLOR_GRAY2BGR);

                            if (newTex && !bgra.empty())
                            {
                                Regards::Picture::CPictureArray pictureArray(bgra);
                                newTex->SetData(pictureArray, renderOpenGL->GetOpenCLContext());
                                layer->SetOpenGLTexture(newTex);
                                layer->needTextureUpdate = false;
                            }
                        }

                        textureToRender = layer->GetOpenGLTexture();
                    }
                }

                if (textureToRender)
                {
                    // --- Prise en compte de l'opacité propre à chaque calque ---
                    // Optionnel : Si votre shader accepte une variable d'opacité uniforme (ex: u_opacity)
                    // float layerAlpha = layer->opacity / 100.0f;
                    // defaultShader->SetUniform("u_opacity", layerAlpha);

                    RenderTexture(textureToRender, invert, widthOutput, heightOutput);
                }
            }

            // Désactivation propre après la boucle des calques
            glDisable(GL_BLEND);
            
        }

        AfterRender();
    }

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
