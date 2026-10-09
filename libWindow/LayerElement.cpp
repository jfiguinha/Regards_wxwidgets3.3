#include <header.h>
#include "LayerElement.h"
#include <ImageLoadingFormat.h>
#include <RGBAQuad.h>
#include <opencv2/imgproc.hpp>
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <limits>
#include <unordered_set>

namespace
{
    inline float Clamp01(float value)
    {
        return std::max(0.0f, std::min(1.0f, value));
    }

    inline float BlendChannel(float source, float destination, LayerBlendMode mode)
    {
        switch (mode)
        {
        case LayerBlendMode::Multiply:
            return source * destination;
        case LayerBlendMode::Screen:
            return source + destination - source * destination;
        case LayerBlendMode::Overlay:
            return destination <= 0.5f
                ? 2.0f * source * destination
                : 1.0f - 2.0f * (1.0f - source) * (1.0f - destination);
        case LayerBlendMode::Darken:
            return std::min(source, destination);
        case LayerBlendMode::Lighten:
            return std::max(source, destination);
        case LayerBlendMode::Add:
            return std::min(1.0f, source + destination);
        case LayerBlendMode::Subtract:
            return std::max(0.0f, destination - source);
        case LayerBlendMode::Normal:
        default:
            return source;
        }
    }

    // Normalise les images source en BGRA 8 bits. Les données d'entrée ne sont
    // converties/redimensionnées qu'une seule fois par calque et par composition.
    cv::Mat PrepareLayerImage(const cv::Mat& input, int width, int height)
    {
        if (input.empty() || width <= 0 || height <= 0)
            return {};

        cv::Mat eightBit;
        if (input.depth() == CV_8U)
            eightBit = input;
        else
            input.convertTo(eightBit, CV_MAKETYPE(CV_8U, input.channels()));

        cv::Mat bgra;
        if (eightBit.channels() == 4)
            bgra = eightBit;
        else if (eightBit.channels() == 3)
            cv::cvtColor(eightBit, bgra, cv::COLOR_BGR2BGRA);
        else if (eightBit.channels() == 1)
            cv::cvtColor(eightBit, bgra, cv::COLOR_GRAY2BGRA);
        else
            return {};

        if (bgra.cols != width || bgra.rows != height)
        {
            cv::Mat resized;
            const int interpolation =
                (bgra.cols > width || bgra.rows > height) ? cv::INTER_AREA : cv::INTER_LINEAR;
            cv::resize(bgra, resized, cv::Size(width, height), 0.0, 0.0, interpolation);
            return resized;
        }
        return bgra;
    }

    // Composition source-over avec alpha droit (straight alpha), et mode de
    // fusion appliqué aux couleurs. Aucun split/merge ni Mat flottant temporaire.
    void CompositeLayer(cv::Mat& destination, const cv::Mat& source,
                        float layerOpacity, LayerBlendMode mode)
    {
        if (destination.empty() || source.empty() || layerOpacity <= 0.0f)
            return;

        CV_Assert(destination.type() == CV_8UC4 && source.type() == CV_8UC4);
        const int width = destination.cols;

        for (int y = 0; y < destination.rows; ++y)
        {
            auto* dst = destination.ptr<cv::Vec4b>(y);
            const auto* src = source.ptr<cv::Vec4b>(y);

            for (int x = 0; x < width; ++x)
            {
                const float sourceAlpha = (src[x][3] / 255.0f) * layerOpacity;
                if (sourceAlpha <= 0.0f)
                    continue;

                const float destinationAlpha = dst[x][3] / 255.0f;
                const float outputAlpha = sourceAlpha + destinationAlpha * (1.0f - sourceAlpha);

                for (int c = 0; c < 3; ++c)
                {
                    const float sourceColor = src[x][c] / 255.0f;
                    const float destinationColor = dst[x][c] / 255.0f;
                    const float blendedColor = BlendChannel(sourceColor, destinationColor, mode);

                    // Formule W3C de composition avec mode de fusion :
                    // Co = (1-as)*ad*Cd + (1-ad)*as*Cs + as*ad*B(Cs,Cd)
                    const float premultiplied =
                        (1.0f - sourceAlpha) * destinationAlpha * destinationColor +
                        (1.0f - destinationAlpha) * sourceAlpha * sourceColor +
                        sourceAlpha * destinationAlpha * blendedColor;

                    const float outputColor = outputAlpha > 0.0f
                        ? premultiplied / outputAlpha
                        : 0.0f;
                    dst[x][c] = cv::saturate_cast<uchar>(Clamp01(outputColor) * 255.0f);
                }

                dst[x][3] = cv::saturate_cast<uchar>(Clamp01(outputAlpha) * 255.0f);
            }
        }
    }

    bool CompositeOneLayer(cv::Mat& canvas, LayerElement* layer,
                           int width, int height)
    {
        if (!layer || !layer->isVisible || !layer->GetPicture() ||
            !layer->GetPicture()->IsOk())
            return false;

        const float opacity = Clamp01(layer->opacity / 100.0f);
        if (opacity <= 0.0f)
            return false;

        cv::Mat source = PrepareLayerImage(layer->GetPicture()->GetMatImage(), width, height);
        if (source.empty())
            return false;

        CompositeLayer(canvas, source, opacity, layer->blendMode);
        return true;
    }
}

CImageLoadingFormat* CLayerList::GetPictureToShow()
{
    if (m_layers.empty())
        return nullptr;

    if (m_layers.size() == 1 && m_layers.front())
        return m_layers.front()->GetPicture();

    if (canvasWidth <= 0 || canvasHeight <= 0)
        return nullptr;

    if (!finalImage)
        finalImage = new CImageLoadingFormat();

    if (isChanged)
        UpdatePictureToShow();

    return finalImage;
}

void CLayerList::UpdatePictureToShow()
{
    if (m_layers.empty() || canvasWidth <= 0 || canvasHeight <= 0)
        return;

    if (m_layers.size() == 1)
    {
        isChanged = false;
        return;
    }

    cv::Mat canvas = cv::Mat::zeros(canvasHeight, canvasWidth, CV_8UC4);

    // Le vecteur est ordonné du premier plan (index 0) vers l'arrière-plan.
    // La composition se fait donc du fond vers le premier plan.
    for (auto it = m_layers.rbegin(); it != m_layers.rend(); ++it)
        CompositeOneLayer(canvas, *it, canvasWidth, canvasHeight);

    if (!finalImage)
        finalImage = new CImageLoadingFormat();

    finalImage->SetPicture(canvas);
    isChanged = false;
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
        element->opacity = 100;
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
    delete finalImage;
    finalImage = nullptr;
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

int CLayerList::GetWidth() const
{
	return canvasWidth;
}

int CLayerList::GetHeight() const
{
	return canvasHeight;
}

void CLayerList::IsChanged(bool changed)
{
    this->isChanged = changed;
}

// ==================== Complément de LayerList.cpp ====================

/**
 * Supprime le calque à l'index spécifié, libère sa mémoire
 * et met à jour les indices 'numLayer' des calques restants.
 */
bool CLayerList::erase(size_t index)
{
    if (index >= m_layers.size())
        return false; // Index hors limites

    // 1. Libérer la mémoire du calque ciblé
    if (m_layers[index])
    {
        delete m_layers[index];
    }

    // 2. Retirer le pointeur du std::vector
    m_layers.erase(m_layers.begin() + index);

    // 3. Mettre à jour la variable 'numLayer' pour maintenir la cohérence des index
    for (size_t i = 0; i < m_layers.size(); ++i)
    {
        if (m_layers[i])
        {
            m_layers[i]->numLayer = i;
        }
    }

    // 4. Si la liste devient vide, réinitialiser les dimensions du canevas
    if (m_layers.empty())
    {
        canvasWidth = 0;
        canvasHeight = 0;
    }

    isChanged = true; // Demande de recalculer le rendu final de l'image
    return true;
}

/**
 * Duplique le calque situé à l'index spécifié et l'ajoute à la liste.
 */
bool CLayerList::copy(size_t index)
{
    if (index >= m_layers.size() || m_layers[index] == nullptr)
        return false; // Index hors limites ou calque invalide

    // 1. Utilise le constructeur de copie de LayerElement défini plus haut
    LayerElement* clonedLayer = new LayerElement(*m_layers[index]);

    // 2. Assigne le nouvel index au calque dupliqué
    clonedLayer->numLayer = m_layers.size();

    // 3. Ajoute le calque cloné à la liste
    m_layers.push_back(clonedLayer);

    isChanged = true; // Signale un changement pour reconstruire l'image finale
    return true;
}

#include <algorithm> // Pour std::sort et std::find

/**
 * Fusionne une liste d'indices de calques en un seul calque unique.
 * Le calque final est placé à la position du calque le plus haut de la sélection.
 */
bool CLayerList::FusionLayers(std::vector<int>* listLayer)
{
    if (!listLayer || listLayer->size() < 2 || canvasWidth <= 0 || canvasHeight <= 0)
        return false;

    std::vector<int> indices = *listLayer;
    std::sort(indices.begin(), indices.end());

    // Refuser les doublons et tous les indices invalides avant de modifier la liste.
    if (std::adjacent_find(indices.begin(), indices.end()) != indices.end())
        return false;

    for (const int index : indices)
    {
        if (index < 0 || static_cast<size_t>(index) >= m_layers.size() || !m_layers[index])
            return false;
    }

    cv::Mat merged = cv::Mat::zeros(canvasHeight, canvasWidth, CV_8UC4);

    // Même ordre que l'aperçu : fond vers premier plan, en conservant
    // visibilité, opacité et mode de fusion de chaque calque sélectionné.
    for (auto it = indices.rbegin(); it != indices.rend(); ++it)
        CompositeOneLayer(merged, m_layers[*it], canvasWidth, canvasHeight);

    auto* fusedPicture = new CImageLoadingFormat();
    fusedPicture->SetPicture(merged);

    const size_t targetIndex = static_cast<size_t>(indices.front());
    LayerElement* target = m_layers[targetIndex];
    target->SetPicture(fusedPicture);
    target->layerName = "Merged Layers";
    target->opacity = 100;
    target->blendMode = LayerBlendMode::Normal;
    target->isVisible = true;

    // Suppression en ordre décroissant pour ne pas décaler les indices restants.
    for (auto it = indices.rbegin(); it != indices.rend(); ++it)
    {
        if (static_cast<size_t>(*it) == targetIndex)
            continue;

        delete m_layers[static_cast<size_t>(*it)];
        m_layers.erase(m_layers.begin() + *it);
    }

    for (size_t i = 0; i < m_layers.size(); ++i)
        if (m_layers[i])
            m_layers[i]->numLayer = static_cast<int>(i);

    isChanged = true;
    return true;
}

/**
 * Déplace un calque vers le haut visuellement (diminue son index dans le vecteur).
 */
bool CLayerList::MoveUpLayer(size_t index)
{
    // Index 0 est déjà au sommet du vecteur (premier plan visuel)
    if (index == 0 || index >= m_layers.size())
        return false;

    // Échanger le calque avec celui du dessus (index - 1)
    std::swap(m_layers[index], m_layers[index - 1]);

    // Mettre à jour les numéros de calque internes
    m_layers[index]->numLayer = index;
    m_layers[index - 1]->numLayer = index - 1;

    isChanged = true;
    return true;
}

/**
 * Déplace un calque vers le bas visuellement (augmente son index dans le vecteur).
 */
bool CLayerList::MoveDownLayer(size_t index)
{
    // Le dernier index est déjà tout au fond
    if (m_layers.empty() || index >= m_layers.size() - 1)
        return false;

    // Échanger le calque avec celui du dessous (index + 1)
    std::swap(m_layers[index], m_layers[index + 1]);

    // Mettre à jour les numéros de calque internes
    m_layers[index]->numLayer = index;
    m_layers[index + 1]->numLayer = index + 1;

    isChanged = true;
    return true;
}

cv::Mat CLayerList::CreateCheckerboardBackground(int width, int height, int sizeSquare)
{
    if (width <= 0 || height <= 0 || sizeSquare <= 0)
        return {};

    // Crée une image de base blanche et opaque (Alpha = 255)
    cv::Mat checkerboard(height, width, CV_8UC4, cv::Scalar(255, 255, 255, 255));

    // Couleur des carrés gris (Gris clair en BGRA)
    cv::Scalar grayColor(220, 220, 220, 255);

    // Dessiner les carrés gris en alternance
    for (int y = 0; y < height; y += sizeSquare)
    {
        for (int x = 0; x < width; x += sizeSquare)
        {
            // Condition mathématique pour alterner les cases comme un échiquier
            if (((x / sizeSquare) + (y / sizeSquare)) % 2 == 1)
            {
                // Définir la zone du carré gris, en s'assurant de ne pas dépasser l'image
                int w = std::min(sizeSquare, width - x);
                int h = std::min(sizeSquare, height - y);

                cv::Rect roi(x, y, w, h);
                checkerboard(roi).setTo(grayColor);
            }
        }
    }

    return checkerboard;
}


void CLayerList::CreateLayer(wxString layerName)
{
    // 1. Instancier le nouveau calque avec ses paramètres par défaut
    LayerElement* newLayer = new LayerElement();
    newLayer->isVisible = true;
    newLayer->opacity = 100; // Initialisé à 100% d'opacité
    newLayer->layerName = layerName;
    newLayer->numLayer = m_layers.size();

    // 2. Si le canevas a déjà des dimensions définies, on lui crée une image transparente
    if (canvasWidth > 0 && canvasHeight > 0)
    {
        // Création d'une matrice OpenCV transparente (4 canaux : BGRA)
        //cv::Mat transparentMat = cv::Mat::zeros(canvasHeight, canvasWidth, CV_8UC4);

        cv::Mat transparentMat = CreateCheckerboardBackground(canvasHeight, canvasWidth);


        CImageLoadingFormat* newPicture = new CImageLoadingFormat();
        newPicture->SetPicture(transparentMat);

        newLayer->SetPicture(newPicture);
    }
    else
    {
        // Optionnel : Si c'est le tout premier calque et que les dimensions ne sont pas encore définies,
        // vous pouvez soit assigner des dimensions par défaut (ex: 800x600), soit laisser l'image à nullptr.
        // Ici, on initialise le calque sans image en attendant un appel à SetPicture ou push_back.
        newLayer->SetPicture(nullptr);
    }

    // 3. Ajouter le calque en haut de la pile (fin du vecteur)
    m_layers.push_back(newLayer);

    isChanged = true; // Signaler qu'un rafraîchissement du rendu est nécessaire
}


/**
 * Modifie les propriétés d'un calque spécifique et demande un rafraîchissement.
 * Retourne true si le calque a été mis à jour avec succès.
 */
bool CLayerList::SetPropertiesLayer(int numLayer, wxString layerName, LayerBlendMode blendMode, int opacity)
{
    // 1. Validation de l'index du calque
    if (numLayer < 0 || static_cast<size_t>(numLayer) >= m_layers.size())
        return false;

    LayerElement* layer = m_layers[numLayer];
    if (!layer)
        return false;

    // 2. Validation et encadrement de l'opacité (ex: entre 0 et 100)
    int validOpacity = opacity;
    if (validOpacity < 0) validOpacity = 0;
    if (validOpacity > 100) validOpacity = 100;

    // 3. Application des nouvelles propriétés
    layer->layerName = layerName;
    layer->blendMode = blendMode;
    layer->opacity = validOpacity;

    // 4. Signaler le changement pour recalculer le rendu final (dans GetPictureToShow)
    isChanged = true;

    return true;
}