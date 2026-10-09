#include <header.h>
#include "BitmapEditor.h"
#include <ViewerTheme.h>
#include <FiltreEffet.h>
#include <ViewerThemeInit.h>
#include <RGBAQuad.h>
#include <ImageLoadingFormat.h>
#include <Draw.h>
using namespace Regards::Viewer;

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
    numActifLayer = numLayer;
}

void CBitmapEditor::OnUpdateLayerBitmap(wxCommandEvent& event)
{
    int numLayer = event.GetInt();
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
                listOfLayer[numLayer]->SetPicture(picture);
                listOfLayer.UpdatePictureToShow();

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
