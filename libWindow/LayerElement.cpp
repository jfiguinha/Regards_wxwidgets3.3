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

    UpdatePictureToShow();

    return finalImage;
}

void CLayerList::UpdatePictureToShow()
{
    // S'il n'y a aucun calque, on ne renvoie rien
    if (m_layers.empty())
        return;

    if (m_layers.size() == 1)
        return;

    if (!finalImage)
    {
        finalImage = new CImageLoadingFormat();
        // Initialiser le canevas avec une image transparente au format BGRA aux bonnes dimensions
        cv::Mat canvas = cv::Mat::zeros(canvasHeight, canvasWidth, CV_8UC4);
        finalImage->SetPicture(canvas);
        isChanged = true;
    }

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
bool CLayerList::FusionLayers(std::vector<int> * listLayer)
{
    // 1. Validations de base
    if (listLayer->size() < 2)
        return false; // Il faut au moins 2 calques pour fusionner

    // Trier les indices par ordre croissant pour faciliter la gestion
    std::vector<int> sortedIndices = *listLayer;
    std::sort(sortedIndices.begin(), sortedIndices.end());

    // Vérifier que tous les indices sont valides
    for (int idx : sortedIndices)
    {
        if (idx < 0 || static_cast<size_t>(idx) >= m_layers.size() || !m_layers[idx])
            return false;
    }

    // 2. Créer temporairement un mini-canevas pour fusionner uniquement les calques sélectionnés
    // On initialise une matrice transparente aux dimensions globales
    cv::Mat fusionCanvas = cv::Mat::zeros(canvasHeight, canvasWidth, CV_8UC4);

    // Parcourir à l'envers (du fond vers le haut) par rapport à l'affichage
    // Note : sortedIndices contient les indices du premier plan (petits index) vers l'arrière plan (grands index).
    // On parcourt donc sortedIndices de la fin vers le début.
    for (auto it = sortedIndices.rbegin(); it != sortedIndices.rend(); ++it)
    {
        int idx = *it;
        LayerElement* layer = m_layers[idx];

        if (layer->isVisible && layer->GetPicture() && layer->GetPicture()->IsOk())
        {
            float opacityFactor = layer->opacity / 100.0f;
            if (opacityFactor <= 0.0f) continue;
            if (opacityFactor > 1.0f) opacityFactor = 1.0f;

            cv::Mat srcImg = layer->GetPicture()->GetMatImage();
            if (srcImg.cols != canvasWidth || srcImg.rows != canvasHeight)
            {
                cv::resize(srcImg, srcImg, cv::Size(canvasWidth, canvasHeight));
            }

            // --- Algorithme d'Alpha Blending (Similaire à votre GetPictureToShow) ---
            cv::Mat srcF, dstF;
            srcImg.convertTo(srcF, CV_32FC4);
            fusionCanvas.convertTo(dstF, CV_32FC4);

            std::vector<cv::Mat> srcChannels(4), dstChannels(4);
            cv::split(srcF, srcChannels);
            cv::split(dstF, dstChannels);

            cv::Mat alphaSrc = (srcChannels[3] / 255.0f) * opacityFactor;
            cv::Mat alphaDst = dstChannels[3] / 255.0f;
            cv::Mat outAlpha = alphaSrc + alphaDst.mul(1.0f - alphaSrc);

            cv::Mat mask = (outAlpha > 0.0f);
            cv::Mat safeOutAlpha = cv::Mat::ones(outAlpha.size(), outAlpha.type());
            outAlpha.copyTo(safeOutAlpha, mask);

            std::vector<cv::Mat> outChannels(4);
            for (int i = 0; i < 3; ++i)
            {
                cv::Mat blendedColor = srcChannels[i].mul(alphaSrc) + dstChannels[i].mul(alphaDst).mul(1.0f - alphaSrc);
                cv::divide(blendedColor, safeOutAlpha, outChannels[i]);
            }
            outChannels[3] = outAlpha * 255.0f;

            cv::Mat blendedF;
            cv::merge(outChannels, blendedF);
            blendedF.convertTo(fusionCanvas, CV_8UC4);
        }
    }

    // 3. Remplacement dans la liste des calques
    // On conserve le calque le plus "haut" (le plus petit index de la sélection) pour y mettre le résultat
    int targetIndex = sortedIndices[0];

    // Créer le nouveau format d'image pour stocker la matrice fusionnée
    CImageLoadingFormat* fusedFormat = new CImageLoadingFormat();
    fusedFormat->SetPicture(fusionCanvas);

    // Configurer le calque cible qui va accueillir le résultat
    m_layers[targetIndex]->SetPicture(fusedFormat);
    m_layers[targetIndex]->layerName = "Merged Layers";
    m_layers[targetIndex]->opacity = 100; // La transparence est maintenant intégrée à la matrice
    m_layers[targetIndex]->isVisible = true;

    // 4. Supprimer les autres calques fusionnés (en partant de la fin pour ne pas décaler les indices restants)
    for (size_t i = sortedIndices.size() - 1; i > 0; --i)
    {
        int idxToDelete = sortedIndices[i];
        if (m_layers[idxToDelete])
        {
            delete m_layers[idxToDelete];
        }
        m_layers.erase(m_layers.begin() + idxToDelete);
    }

    // 5. Réindexer proprement le membre 'numLayer' de chaque calque restant
    for (size_t i = 0; i < m_layers.size(); ++i)
    {
        if (m_layers[i])
        {
            m_layers[i]->numLayer = i;
        }
    }

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
    if (index >= m_layers.size() - 1)
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