// ==================== LayerElement.cpp ====================
#include <header.h>
#include "LayerElement.h"
#include <GLTexture.h>
#include <opencv2/imgproc.hpp>
#include <algorithm>
#include <atomic>
#include <LayerFonction.h>
// ============================================================================
// Implémentation : LayerElement (Mère)
// ============================================================================
LayerElement::LayerElement() {}

LayerElement::LayerElement(const LayerElement& other)
{
    numLayer = other.numLayer;
    isVisible = other.isVisible;
    blendMode = other.blendMode;
    opacity = other.opacity;
    layerName = other.layerName + " _copy";

    if (other.picture)
        picture = new CImageLoadingFormat(*other.picture);

    needTextureUpdate = true;
}

LayerElement::~LayerElement()
{
    if (picture)
        delete picture;
}

void LayerElement::SetPicture(CImageLoadingFormat* source)
{
    if (picture)
        delete picture;

    picture = source;
    pictureVersion++;
    needTextureUpdate = true;
}

void LayerElement::InvalidatePicture()
{
    pictureVersion++;
    needTextureUpdate = true;
}

uint64_t LayerElement::NextLayerId()
{
    static std::atomic<uint64_t> counter{ 0 };
    return ++counter;
}

// ============================================================================
// Implémentation : LayerElementUMat
// ============================================================================
LayerElementUMat::LayerElementUMat(const LayerElementUMat& other) : LayerElement(other)
{
    // Réinitialisation de la texture spécifique
    m_umatTexture = cv::UMat();
}

void LayerElementUMat::SetUMatTexture(const cv::UMat& texture)
{
    if (m_umatTexture.handle(cv::ACCESS_READ) == texture.handle(cv::ACCESS_READ))
        return;
    m_umatTexture = texture;
}

cv::UMat LayerElementUMat::DetachUMatTexture()
{
    cv::UMat texture = m_umatTexture;
    m_umatTexture.release();
    needTextureUpdate = true;
    return texture;
}

// ============================================================================
// Implémentation : LayerElementGL
// ============================================================================
LayerElementGL::LayerElementGL(const LayerElementGL& other) : LayerElement(other)
{
    // Réinitialisation du pointeur spécifique OpenGL pour éviter le double-free
    m_glTexture = nullptr;
}

void LayerElementGL::DeleteTexture()
{
    if (m_glTexture)
    {
        delete m_glTexture;
        m_glTexture = nullptr;
    }
}

void LayerElementGL::SetOpenGLTexture(Regards::OpenGL::GLTexture* texture)
{
    if (m_glTexture == texture) return;
    DeleteTexture();
    m_glTexture = texture;
}

Regards::OpenGL::GLTexture* LayerElementGL::DetachOpenGLTexture()
{
    Regards::OpenGL::GLTexture* texture = m_glTexture;
    m_glTexture = nullptr;
    needTextureUpdate = true;
    return texture;
}

// ============================================================================
// Implémentation : CLayerList (Utilisation polymorphique)
// ============================================================================

CLayerList::~CLayerList()
{
    delete finalImage;
    for (LayerElement* layer : m_layers)
        delete layer; // Appelle le destructeur virtuel de la classe fille appropriée
}


bool CLayerList::erase(size_t index)
{
    if (index >= m_layers.size()) return false;

    if (m_layers[index])
    {
        delete m_layers[index]; // Destructeur virtuel appelé
    }
    m_layers.erase(m_layers.begin() + index);

    for (size_t i = 0; i < m_layers.size(); ++i)
        if (m_layers[i]) m_layers[i]->numLayer = i;

    if (m_layers.empty()) { canvasWidth = 0; canvasHeight = 0; }
    isChanged = true;
    return true;
}

bool CLayerList::copy(size_t index)
{
    if (index >= m_layers.size() || m_layers[index] == nullptr) return false;

    // Utilisation du clonage polymorphique à la place du constructeur de copie direct
    LayerElement* clonedLayer = m_layers[index]->Clone();
    clonedLayer->numLayer = m_layers.size();
    m_layers.push_back(clonedLayer);

    isChanged = true;
    return true;
}

void CLayerList::CreateLayer(wxString layerName, BackendType type)
{
    LayerElement* newLayer = nullptr;

    // Instanciation de la bonne classe fille selon le besoin de votre application
    if (type == BackendType::UMat)
        newLayer = new LayerElementUMat();
    else
        newLayer = new LayerElementGL();

    newLayer->isVisible = true;
    newLayer->opacity = 100;
    newLayer->layerName = layerName;
    newLayer->numLayer = m_layers.size();

    if (canvasWidth > 0 && canvasHeight > 0)
    {
        cv::Mat transparentMat = cv::Mat::zeros(canvasHeight, canvasWidth, CV_8UC4);
        transparentMat.setTo(cv::Scalar(255, 255, 255, 0));

        CImageLoadingFormat* newPicture = new CImageLoadingFormat();
        newPicture->SetPicture(transparentMat);
        newLayer->SetPicture(newPicture);
    }
    m_layers.push_back(newLayer);
    isChanged = true;
}

// Les fonctions UpdatePictureToShow, FusionLayers, MoveUp/Down, SetPropertiesLayer et SetPicture
// restent identiques à vos implémentations précédentes, car elles n'appellent que des propriétés
// de la classe de base ou délèguent le traitement matriciel (cv::Mat) à LayerFonction.


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
        LayerFonction::CompositeOneLayer(canvas, *it, canvasWidth, canvasHeight);

    if (!finalImage)
        finalImage = new CImageLoadingFormat();

    finalImage->SetPicture(canvas);
    isChanged = false;
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
        LayerFonction::CompositeOneLayer(merged, m_layers[*it], canvasWidth, canvasHeight);

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
