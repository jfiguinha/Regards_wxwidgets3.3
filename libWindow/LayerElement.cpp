#include <header.h>
#include "LayerElement.h"
#include <ImageLoadingFormat.h>
#include <RGBAQuad.h>

CImageLoadingFormat* CLayerList::GetPictureToShow()
{
    // S'il n'y a aucun calque, on ne renvoie rien
    if (m_layers.empty())
        return nullptr;

    if (m_layers.size() == 1)
        return m_layers[0]->GetPicture();

    if (!finalImage)
    {
        finalImage = new CImageLoadingFormat();
        // Initialiser le canevas avec une image transparente au format BGRA aux bonnes dimensions
        cv::Mat canvas = cv::Mat::zeros(canvasHeight, canvasWidth, CV_8UC4);
        finalImage->SetPicture(canvas);
        isChanged = true;
    }

    if (finalImage && !isChanged)
        return finalImage;

    // Réinitialiser le canevas sous forme de matrice transparente
    cv::Mat canvas = cv::Mat::zeros(canvasHeight, canvasWidth, CV_8UC4);

    // 2. Parcourir la liste des calques À L'ENVERS (du fond vers le premier plan)
    for (auto it = m_layers.rbegin(); it != m_layers.rend(); ++it)
    {
        LayerElement* layer = *it;

        // Condition stricte : le calque doit exister, ÊTRE VISIBLE, et posséder une image valide
        if (layer && layer->isVisible && layer->GetPicture() && layer->GetPicture()->IsOk())
        {
            // Calcul du facteur d'opacité globale du calque (sur une base de 100)
            float layerOpacityFactor = layer->opacity / 100.0f;
            if (layerOpacityFactor < 0.0f) layerOpacityFactor = 0.0f;
            if (layerOpacityFactor > 1.0f) layerOpacityFactor = 1.0f;

            if (layerOpacityFactor == 0.0f)
                continue;

            // Récupération de la matrice OpenCV source du calque (BGRA)
            cv::Mat srcImg = layer->GetPicture()->GetMatImage();

            // Ajuster la taille de la source si elle diffère du canevas
            if (srcImg.cols != canvasWidth || srcImg.rows != canvasHeight)
            {
                cv::resize(srcImg, srcImg, cv::Size(canvasWidth, canvasHeight));
            }

            // ----------------------------------------------------------------
            // FUSION OPTIMISÉE AVEC OPENCV (Formule Alpha Blending Vectorielle)
            // ----------------------------------------------------------------

            // 1. Conversion des matrices en flottants (0.0 à 255.0) pour éviter les débordements d'octets
            cv::Mat srcF, dstF;
            srcImg.convertTo(srcF, CV_32FC4);
            canvas.convertTo(dstF, CV_32FC4);

            // 2. Séparation des canaux (B, G, R, A) pour la source et la destination
            std::vector<cv::Mat> srcChannels(4);
            std::vector<cv::Mat> dstChannels(4);
            cv::split(srcF, srcChannels);
            cv::split(dstF, dstChannels);

            // 3. Calcul des masques d'alpha normalisés (0.0 à 1.0)
            // L'alpha de la source prend en compte l'opacité globale du calque
            cv::Mat alphaSrc = (srcChannels[3] / 255.0f) * layerOpacityFactor;
            cv::Mat alphaDst = dstChannels[3] / 255.0f;

            // 4. Calcul de l'alpha de sortie : outAlpha = alphaSrc + alphaDst * (1.0 - alphaSrc)
            cv::Mat outAlpha = alphaSrc + alphaDst.mul(1.0f - alphaSrc);

            // Éviter la division par zéro sur les pixels entièrement transparents
            cv::Mat mask = (outAlpha > 0.0f);
            cv::Mat safeOutAlpha = cv::Mat::ones(outAlpha.size(), outAlpha.type());
            outAlpha.copyTo(safeOutAlpha, mask);

            // 5. Fusion des canaux de couleur (B, G, R)
            std::vector<cv::Mat> outChannels(4);
            for (int i = 0; i < 3; ++i)
            {
                // Formule : (srcColor * alphaSrc + dstColor * alphaDst * (1.0 - alphaSrc)) / outAlpha
                cv::Mat blendedColor = srcChannels[i].mul(alphaSrc) + dstChannels[i].mul(alphaDst).mul(1.0f - alphaSrc);
                cv::divide(blendedColor, safeOutAlpha, outChannels[i]);
            }

            // Réassigner l'alpha final mis à l'échelle (0.0 à 255.0)
            outChannels[3] = outAlpha * 255.0f;

            // 6. Fusionner les canaux isolés et reconvertir en format 8 bits (CV_8UC4)
            cv::Mat blendedF;
            cv::merge(outChannels, blendedF);
            blendedF.convertTo(canvas, CV_8UC4);
        }
    }

    // Mettre à jour l'image finale avec le résultat de la fusion OpenCV
    finalImage->SetPicture(canvas);

    isChanged = false;
    return finalImage;
}


void CLayerList::SetPicture(CImageLoadingFormat* bitmapIn)
{
    if (!bitmapIn)
        return;

    if (m_layers.size() == 0 && bitmapIn)
    {
        canvasWidth = bitmapIn->GetWidth();
        canvasHeight = bitmapIn->GetHeight();

        LayerElement* element = new LayerElement();
        element->isVisible = true;
        element->numLayer = m_layers.size();
        element->opacity = 255;
        element->layerName = bitmapIn->GetFilename();
        element->SetPicture(bitmapIn);

        m_layers.push_back(element);
    }
    else
    {
        m_layers[0]->SetPicture(bitmapIn);
    }
    isChanged = true;



}

CLayerList::~CLayerList()
{
    for (LayerElement* layerElement : m_layers)
    {
        if (layerElement)
            delete layerElement;
    }
}

void CLayerList::push_back(LayerElement* element) 
{ 
	if (!element)
		return;

	if (!element->GetPicture())
		return;

	if (m_layers.size() == 0 && element->GetPicture())
	{
		canvasWidth = element->GetPicture()->GetWidth();
		canvasHeight = element->GetPicture()->GetHeight();
	}

    isChanged = true;

	m_layers.push_back(element); 
}

int CLayerList::GetWidth()
{
	return canvasWidth;
}

int CLayerList::GetHeight()
{
	return canvasHeight;
}

void CLayerList::IsChanged(bool isChanged)
{
    this->isChanged = isChanged;
}