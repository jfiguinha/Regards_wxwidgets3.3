// ==================== LayerElement.cpp ====================
#include <header.h>
#include "LayerElement.h"
#include <GLTexture.h>
#include <opencv2/imgproc.hpp>
#include <algorithm>
#include <atomic>
#include <LayerFonction.h>
#include <fstream>

// ============================================================================
// Implémentation : LayerElement (Classe Mère)
// ============================================================================

std::atomic<uint64_t> LayerElement::counter{ 0 };

LayerElement::LayerElement() {}

LayerElement::LayerElement(const LayerElement& other)
{
    // 1. Propriétés graphiques et d'affichage
    numLayer = other.numLayer;
    isVisible = other.isVisible;
    blendMode = other.blendMode;
    opacity = other.opacity;
    layerName = other.layerName + " _copy";
    typeLayer = other.typeLayer;

    // 2. Gestion de l'invalidation et des versions
    needTextureUpdate = true; // Forcé à true pour reconstruire la texture GPU sur la nouvelle liste
    pictureVersion = other.pictureVersion;

    // 3. Dimensions et Positionnement géométrique
    textureWidth = other.textureWidth;
    textureHeight = other.textureHeight;
    x = other.x;
    y = other.y;

    // 4. Copie stricte de l'identifiant unique du calque (indispensable pour FindElementByLayerId)
    layerId = other.layerId;

    // 5. Copie profonde de l'image de travail CPU (buffer de pixels indépendant)
    picture = CloneImage(other.picture);

}

CImageLoadingFormat* LayerElement::CloneImage(CImageLoadingFormat* source)
{
    if (source == nullptr)
        return nullptr;

    auto* copy = new CImageLoadingFormat();

    // clone() alloue un nouveau buffer : plus aucun partage de pixels avec la source
    cv::Mat mat = source->GetMatrix().getMat().clone();
    copy->SetPicture(mat);
    copy->SetOrientation(source->GetOrientation());
    copy->SetFilename(source->GetFilename());
    return copy;
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

    filepath = picture->GetFilename();

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
    return ++counter;
}

uint64_t LayerElement::GetLayerId()
{
    if (counter == 0)
        return 1;
    return counter - 1;
}

// ============================================================================
// Implémentation : LayerElementUMat (Classe Dérivée OpenCV)
// ============================================================================
LayerElementUMat::LayerElementUMat(const LayerElementUMat& other) : LayerElement(other)
{
    // Partage sécurisé de la texture UMat via l'incrémentation du compteur de référence d'OpenCV
    if (!other.m_umatTexture.empty())
    {
        m_umatTexture = other.m_umatTexture;
        needTextureUpdate = false;
    }
    else
    {
        m_umatTexture = cv::UMat();
        needTextureUpdate = true;
    }
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
// Implémentation : LayerElementGL (Classe Dérivée OpenGL)
// ============================================================================
LayerElementGL::LayerElementGL(const LayerElementGL& other) : LayerElement(other)
{
    // Une texture OpenGL ne peut pas être partagée par simple copie de pointeur.
    // On met à plat le pointeur pour forcer le thread graphique à recréer sa ressource.
    m_glTexture = nullptr;
    needTextureUpdate = true;
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
// Implémentation : CLayerList (Gestionnaire Polymorphique de Calques)
// ============================================================================

CLayerList::CLayerList(const CLayerList& other)
{
    canvasWidth = other.canvasWidth;
    canvasHeight = other.canvasHeight;
    isChanged = true;
    finalImage = nullptr;

    // Le clonage polymorphique évite l'Object Slicing et conserve les types dérivés réels
    for (const LayerElement* layer : other.m_layers)
    {
        if (layer != nullptr)
        {
            m_layers.push_back(layer->Clone());
        }
    }
}

CLayerList::~CLayerList()
{
    delete finalImage;
    for (LayerElement* layer : m_layers)
        delete layer; // Appelle le destructeur virtuel de la classe dérivée appropriée
}

bool CLayerList::erase(size_t index)
{
    if (index >= m_layers.size()) return false;

    if (m_layers[index])
    {
        delete m_layers[index];
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

    LayerElement* clonedLayer = m_layers[index]->Clone();
    clonedLayer->numLayer = m_layers.size();
    clonedLayer->layerId = clonedLayer->NextLayerId();
    m_layers.push_back(clonedLayer);

    isChanged = true;
    return true;
}

void CLayerList::CreateLayer(wxString layerName, BackendType type)
{
    LayerElement* newLayer = nullptr;

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

void CLayerList::UpdatePictureToShow()
{
    if (m_layers.empty() || canvasWidth <= 0 || canvasHeight <= 0)
        return;

    if (!finalImage)
        finalImage = new CImageLoadingFormat();

    // SÉCURITÉ : Gestion propre du cas avec un seul calque
    if (m_layers.size() == 1)
    {
        if (m_layers.front() && m_layers.front()->GetPicture())
        {
            *finalImage = *(m_layers.front()->GetPicture());
        }
        isChanged = false;
        return;
    }

    cv::Mat canvas(canvasHeight, canvasWidth, CV_8UC4, cv::Scalar(0, 0, 0, 0));

    // Composition du fond vers le premier plan.
    for (auto it = m_layers.rbegin(); it != m_layers.rend(); ++it)
    {
        if (*it == nullptr)
            continue;

        LayerFonction::CompositeOneLayer(canvas, *it, canvasWidth, canvasHeight);
    }

    finalImage->SetPicture(canvas);
    isChanged = false;
}

bool CLayerList::SetPropertiesLayer(int numLayer, wxString layerName, LayerBlendMode blendMode, int opacity)
{
    if (numLayer < 0 || static_cast<size_t>(numLayer) >= m_layers.size())
        return false;

    LayerElement* layer = m_layers[numLayer];
    if (!layer)
        return false;

    int validOpacity = opacity;
    if (validOpacity < 0) validOpacity = 0;
    if (validOpacity > 100) validOpacity = 100;

    layer->layerName = layerName;
    layer->blendMode = blendMode;
    layer->opacity = validOpacity;

    isChanged = true;
    return true;
}

bool CLayerList::FusionLayers(std::vector<int>* listLayer)
{
    if (!listLayer || listLayer->size() < 2 || canvasWidth <= 0 || canvasHeight <= 0)
        return false;

    std::vector<int> indices = *listLayer;
    std::sort(indices.begin(), indices.end());

    if (std::adjacent_find(indices.begin(), indices.end()) != indices.end())
        return false;

    for (const int index : indices)
    {
        if (index < 0 || static_cast<size_t>(index) >= m_layers.size() || !m_layers[index])
            return false;
    }

    cv::Mat merged = cv::Mat::zeros(canvasHeight, canvasWidth, CV_8UC4);

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


// 1. Ajoutez ces méthodes virtuelles dans la classe 'LayerElement' (Section public) :
void LayerElement::Write(cv::FileStorage& fs) const {
    fs << "typeLayer" << typeLayer;
    fs << "numLayer" << numLayer;
    fs << "isVisible" << (isVisible ? 1 : 0);
    fs << "opacity" << opacity;
    fs << "layerName" << layerName.ToStdString(); // Conversion wxString -> std::string
    fs << "x" << x;
    fs << "y" << y;
    fs << "textureWidth" << textureWidth;
    fs << "textureHeight" << textureHeight;

    // Sérialisation des matrices OpenCV via CImageLoadingFormat
    if (picture)
        fs << "picture" << picture->GetMatrix().getMat();
    else
        fs << "picture" << cv::Mat();
}

void LayerElement::Read(const cv::FileNode& node) {
    typeLayer = (int)node["typeLayer"];
    numLayer = (int)node["numLayer"];
    isVisible = ((int)node["isVisible"] == 1);
    opacity = (int)node["opacity"];

    std::string name;
    node["layerName"] >> name;
    layerName = wxString::FromUTF8(name.c_str()); // Recréation de la wxString

    x = (int)node["x"];
    y = (int)node["y"];
    textureWidth = (int)node["textureWidth"];
    textureHeight = (int)node["textureHeight"];

    // Restauration des images
    cv::Mat matPicture, matOriginal;
    node["picture"] >> matPicture;
    node["originalPicture"] >> matOriginal;

    if (!matPicture.empty()) {
        picture = new CImageLoadingFormat();
        picture->SetPicture(matPicture);
    }

    pictureVersion = 1;
    needTextureUpdate = true;
}

// ============================================================================
// Nouvelles méthodes de sérialisation binaire pour LayerElement
// ============================================================================

// Fonction pour sérialiser une cv::Mat compressée en WebP sans perte
// Sérialisation binaire avec compression PNG sans perte
static void WriteMatBinary(std::ofstream& fs, const cv::Mat& mat)
{
    bool empty = mat.empty();
    fs.write(reinterpret_cast<const char*>(&empty), sizeof(empty));

    if (!empty)
    {
        std::vector<uchar> buffer;
        // L'extension ".png" active nativement la compression sans perte avec canal Alpha
        cv::imencode(".png", mat, buffer);

        size_t bufferSize = buffer.size();
        fs.write(reinterpret_cast<const char*>(&bufferSize), sizeof(bufferSize));
        fs.write(reinterpret_cast<const char*>(buffer.data()), bufferSize);
    }
}

// Désérialisation binaire et décompression du PNG
static void ReadMatBinary(std::ifstream& fs, cv::Mat& mat)
{
    bool empty = true;
    fs.read(reinterpret_cast<char*>(&empty), sizeof(empty));

    if (!empty)
    {
        size_t bufferSize = 0;
        fs.read(reinterpret_cast<char*>(&bufferSize), sizeof(bufferSize));

        std::vector<uchar> buffer(bufferSize);
        fs.read(reinterpret_cast<char*>(buffer.data()), bufferSize);

        // IMREAD_UNCHANGED est indispensable pour récupérer les 4 canaux (RGB + Alpha) du calque
        mat = cv::imdecode(buffer, cv::IMREAD_UNCHANGED);
    }
    else
    {
        mat = cv::Mat();
    }
}

bool CLayerList::SaveToFile(const wxString& filename) const
{
    std::ofstream fs(filename.fn_str(), std::ios::binary);
    if (!fs.is_open())
        return false;

    // 1. Écriture des métadonnées globales de la liste
    fs.write(reinterpret_cast<const char*>(&canvasWidth), sizeof(canvasWidth));
    fs.write(reinterpret_cast<const char*>(&canvasHeight), sizeof(canvasHeight));

    // 2. Écriture du nombre de calques
    size_t layerCount = m_layers.size();
    fs.write(reinterpret_cast<const char*>(&layerCount), sizeof(layerCount));

    // 3. Écriture séquentielle de chaque calque polymorphe
    for (const LayerElement* layer : m_layers)
    {
        if (layer == nullptr)
            continue;

        // On sauvegarde impérativement le type concret en premier pour le polymorphisme
        int typeLayer = const_cast<LayerElement*>(layer)->TypeLayer();
        fs.write(reinterpret_cast<const char*>(&typeLayer), sizeof(typeLayer));

        // Propriétés primitives scalaires et géométriques
        fs.write(reinterpret_cast<const char*>(&layer->layerId), sizeof(layer->layerId));
        fs.write(reinterpret_cast<const char*>(&layer->numLayer), sizeof(layer->numLayer));
        fs.write(reinterpret_cast<const char*>(&layer->isVisible), sizeof(layer->isVisible));

        int blendModeInt = static_cast<int>(layer->blendMode);
        fs.write(reinterpret_cast<const char*>(&blendModeInt), sizeof(blendModeInt));

        fs.write(reinterpret_cast<const char*>(&layer->opacity), sizeof(layer->opacity));
        fs.write(reinterpret_cast<const char*>(&layer->x), sizeof(layer->x));
        fs.write(reinterpret_cast<const char*>(&layer->y), sizeof(layer->y));
        fs.write(reinterpret_cast<const char*>(&layer->textureWidth), sizeof(layer->textureWidth));
        fs.write(reinterpret_cast<const char*>(&layer->textureHeight), sizeof(layer->textureHeight));

        // Sérialisation de la chaîne de caractères (wxString -> std::string UTF-8)
        std::string nameStr = layer->layerName.ToStdString();
        size_t nameLength = nameStr.size();
        fs.write(reinterpret_cast<const char*>(&nameLength), sizeof(nameLength));
        if (nameLength > 0)
        {
            fs.write(nameStr.data(), nameLength);
        }

        // Sérialisation des matrices images
        cv::Mat pictureMat = layer->GetPicture() ? layer->GetPicture()->GetMatrix().getMat() : cv::Mat();

        WriteMatBinary(fs, pictureMat);
    }

    fs.close();
    return true;
}

bool CLayerList::LoadFromFile(const wxString& filename)
{
    std::ifstream fs(filename.fn_str(), std::ios::binary);
    if (!fs.is_open())
        return false;

    // Nettoyage de la liste mémoire existante avant réimportation
    if (finalImage)
    {
        delete finalImage;
        finalImage = nullptr;
    }
    for (LayerElement* layer : m_layers)
    {
        delete layer;
    }
    m_layers.clear();

    // 1. Lecture des propriétés globales
    fs.read(reinterpret_cast<char*>(&canvasWidth), sizeof(canvasWidth));
    fs.read(reinterpret_cast<char*>(&canvasHeight), sizeof(canvasHeight));

    // 2. Lecture du nombre de calques
    size_t layerCount = 0;
    fs.read(reinterpret_cast<char*>(&layerCount), sizeof(layerCount));

    // 3. Restauration des instances dérivées abstraites
    for (size_t i = 0; i < layerCount; ++i)
    {
        int typeLayer = 0;
        fs.read(reinterpret_cast<char*>(&typeLayer), sizeof(typeLayer));

        LayerElement* newLayer = nullptr;

        // Résolution de l'instanciation de la classe abstraite (Pattern Factory via typeLayer)
        if (typeLayer == TYPE_OPENCL)
        {
            newLayer = new LayerElementUMat();
        }
        else if (typeLayer == TYPE_OPENGL)
        {
            newLayer = new LayerElementGL();
        }

        if (newLayer == nullptr)
            continue;

        // Lecture des primitives scalaires
        fs.read(reinterpret_cast<char*>(&newLayer->layerId), sizeof(newLayer->layerId));
        fs.read(reinterpret_cast<char*>(&newLayer->numLayer), sizeof(newLayer->numLayer));
        fs.read(reinterpret_cast<char*>(&newLayer->isVisible), sizeof(newLayer->isVisible));

        int blendModeInt = 0;
        fs.read(reinterpret_cast<char*>(&blendModeInt), sizeof(blendModeInt));
        newLayer->blendMode = static_cast<LayerBlendMode>(blendModeInt);

        fs.read(reinterpret_cast<char*>(&newLayer->opacity), sizeof(newLayer->opacity));
        fs.read(reinterpret_cast<char*>(&newLayer->x), sizeof(newLayer->x));
        fs.read(reinterpret_cast<char*>(&newLayer->y), sizeof(newLayer->y));
        fs.read(reinterpret_cast<char*>(&newLayer->textureWidth), sizeof(newLayer->textureWidth));
        fs.read(reinterpret_cast<char*>(&newLayer->textureHeight), sizeof(newLayer->textureHeight));

        // Restauration de la wxString
        size_t nameLength = 0;
        fs.read(reinterpret_cast<char*>(&nameLength), sizeof(nameLength));
        if (nameLength > 0)
        {
            std::string nameStr(nameLength, '\0');
            fs.read(&nameStr[0], nameLength);
            newLayer->layerName = wxString::FromUTF8(nameStr.c_str());
        }

        // Restauration des matrices images
        cv::Mat pictureMat, originalMat;
        ReadMatBinary(fs, pictureMat);
        ReadMatBinary(fs, originalMat);

        if (!pictureMat.empty())
        {
            CImageLoadingFormat* pictureFormat = new CImageLoadingFormat();
            pictureFormat->SetPicture(pictureMat);
            newLayer->SetPicture(pictureFormat);
        }

        // On initialise l'originalPicture si elle était sauvegardée
        if (!originalMat.empty())
        {
            // Libération de la mémoire allouée par défaut par le Reset/SetPicture si existant
            CImageLoadingFormat* originalFormat = new CImageLoadingFormat();
            originalFormat->SetPicture(originalMat);
        }

        newLayer->pictureVersion = 1;
        newLayer->needTextureUpdate = true;

        m_layers.push_back(newLayer);
    }

    fs.close();
    isChanged = true;
    return true;
}


LayerElement* CLayerList::FindElementByLayerId(const uint64_t& layerId)
{
    for (LayerElement* element : m_layers)
    {
        if (element->layerId == layerId)
            return element;
    }
    return nullptr;
}

bool CLayerList::MoveDownLayer(size_t index)
{
    if (index == 0 || index >= m_layers.size())
        return false;

    std::swap(m_layers[index], m_layers[index - 1]);
    m_layers[index]->numLayer = index;
    m_layers[index - 1]->numLayer = index - 1;

    isChanged = true;
    return true;
}

bool CLayerList::MoveUpLayer(size_t index) {
    if (m_layers.empty() || index >= m_layers.size() - 1) return false;
    std::swap(m_layers[index], m_layers[index + 1]);
    m_layers[index]->numLayer = index;
    m_layers[index + 1]->numLayer = index + 1;
    isChanged = true;
    return true;
}
void CLayerList::push_back(LayerElement* element) {
    if (!element || !element->GetPicture()) return;
    if (m_layers.size() == 0) {
        canvasWidth = element->GetPicture()->GetWidth();
        canvasHeight = element->GetPicture()->GetHeight();
    }
    isChanged = true;
    m_layers.push_back(element);
}
CImageLoadingFormat* CLayerList::GetPictureToShow() {
    if (m_layers.empty()) return nullptr;
    if (m_layers.size() == 1 && m_layers.front())
        return m_layers.front()->GetPicture();
    if (canvasWidth <= 0 || canvasHeight <= 0) return nullptr;
    if (finalImage == nullptr) {
        finalImage = new CImageLoadingFormat();
        isChanged = true;
    }
    if (isChanged) {
        UpdatePictureToShow();
    }
    return finalImage;
}
CImageLoadingFormat* CLayerList::ClonePictureToShow() {
    if (isChanged) {
        UpdatePictureToShow();
    }
    CImageLoadingFormat* currentPicture = GetPictureToShow();
    if (currentPicture == nullptr) return nullptr;
    return new CImageLoadingFormat(*currentPicture);
}