#include "header.h"
#include "ThumbnailBuffer.h"
#include <ParamInit.h>
#include "RegardsConfigParam.h"
#include <ConvertUtility.h>
#include <iostream>
#include <wx/file.h>

CThumbnailBuffer::LruCache    CThumbnailBuffer::s_cache;
CThumbnailBuffer::VectorStore CThumbnailBuffer::s_store;

// ── LruCache ────────────────────────────────────────────────────────────────

cv::Mat CThumbnailBuffer::LruCache::get(const wxString& key)
{
    std::unique_lock lock(mutex); // Utilisation d'un lock unique pour la mise à jour sûre du LRU

    auto it = map.find(key);
    if (it == map.end())
        return {};

    // Promotion LRU : Déplacement en queue de liste (élément le plus récent)
    order.erase(it->second.second);
    order.push_back(key);
    it->second.second = std::prev(order.end());

    // Retourne une copie de la matrice (Partage le compteur de référence de la texture OpenCV de manière thread-safe)
    return it->second.first;
}

void CThumbnailBuffer::LruCache::put(const wxString& key, cv::Mat data)
{
    std::unique_lock lock(mutex);

    auto it = map.find(key);
    if (it != map.end())
    {
        order.erase(it->second.second);
        order.push_back(key);
        it->second = { data, std::prev(order.end()) };
        return;
    }

    // Éviction de l'élément le plus ancien si le cache est saturé
    if (static_cast<int>(map.size()) >= maxSize)
    {
        auto oldest = order.front();
        order.pop_front();
        map.erase(oldest);
    }

    order.push_back(key);
    map[key] = { data, std::prev(order.end()) };
}

void CThumbnailBuffer::LruCache::remove(const wxString& key)
{
    std::unique_lock lock(mutex);
    auto it = map.find(key);
    if (it == map.end()) return;

    order.erase(it->second.second);
    map.erase(it);
}

// ── GetPicture ───────────────────────────────────────────────────────────────

cv::Mat CThumbnailBuffer::GetPicture(const wxString& filename)
{
    int sizeBuffer = 100;
    if (auto* param = CParamInit::getInstance())
        sizeBuffer = param->GetBufferSize();

    if (sizeBuffer <= 0)
        return cv::imread(CConvertUtility::ConvertToStdString(filename).c_str(), cv::IMREAD_COLOR);

    s_cache.maxSize = sizeBuffer;

    // 1. Recherche de la matrice déjà décodée en mémoire cache O(1)
    cv::Mat cachedMat = s_cache.get(filename);
    if (!cachedMat.empty())
    {
        return cachedMat;
    }

    // 2. Si absent du cache : Chargement et décodage complet depuis le disque
    cv::Mat decoded;
    if (wxFile::Exists(filename))
    {
        wxFile file(filename);
        if (file.IsOpened())
        {
            size_t fileSize = file.Length();
            cv::Mat rawBuffer(1, static_cast<int>(fileSize), CV_8UC1);
            file.Read(rawBuffer.data, fileSize);
            file.Close();

            if (!rawBuffer.empty())
            {
                // Décodage immédiat en mémoire
                decoded = cv::imdecode(rawBuffer, cv::IMREAD_COLOR);
            }
        }
    }

    // Fallback de sécurité si la lecture par buffer échoue
    if (decoded.empty())
    {
        decoded = cv::imread(CConvertUtility::ConvertToStdString(filename).c_str(), cv::IMREAD_COLOR);
    }

    // 3. Stockage de l'image décodée dans le cache LRU pour les prochains affichages
    if (!decoded.empty())
    {
        s_cache.put(filename, decoded);
    }

    return decoded;
}

void CThumbnailBuffer::RemovePicture(const wxString& filename)
{
    s_cache.remove(filename);
}

void CThumbnailBuffer::InitVectorList(PhotosVector* newVector)
{
    std::shared_ptr<PhotosVector> incoming(newVector);
    std::unique_lock write(s_store.mutex);

    std::unordered_set<wxString> newIndex;
    std::unordered_map<int, wxString> newIdIndex; // Double indexation pour optimiser FindPhotoById

    newIndex.reserve(incoming->size());
    newIdIndex.reserve(incoming->size());

    for (CPhotos& photo : *incoming)
    {
        newIndex.insert(photo.GetPath());
        newIdIndex[photo.GetId()] = photo.GetPath();
    }

    s_store.data = std::move(incoming);
    s_store.size = static_cast<int>(s_store.data->size());
    s_store.pathIndex = std::move(newIndex);
    s_store.idIndex = std::move(newIdIndex); // Stockage de l'index d'identifiants
}

std::shared_ptr<const PhotosVector> CThumbnailBuffer::GetVectorList()
{
    std::shared_lock read(s_store.mutex);
    return s_store.data;
}

int CThumbnailBuffer::GetVectorSize()
{
    return s_store.size.load();
}

CPhotos CThumbnailBuffer::GetVectorValue(int i)
{
    std::shared_lock read(s_store.mutex);
    if (!s_store.data || i < 0 || i >= static_cast<int>(s_store.data->size()))
        throw std::out_of_range("GetVectorValue: index hors limites");
    return s_store.data->at(i);
}

wxString CThumbnailBuffer::FindPhotoByPath(const wxString& path)
{
    std::shared_lock read(s_store.mutex);
    if (!s_store.data) return {};
    return s_store.pathIndex.find(path) != s_store.pathIndex.end() ? path : wxString{};
}

bool CThumbnailBuffer::FindValidFile(const wxString& localFilename)
{
    std::shared_lock read(s_store.mutex);
    if (!s_store.data) return false;
    return s_store.pathIndex.find(localFilename) != s_store.pathIndex.end();
}

wxString CThumbnailBuffer::FindPhotoById(int id)
{
    std::shared_lock read(s_store.mutex);
    if (!s_store.data) return {};

    // Optimisation : Recherche instantanée O(1) au lieu du parcours de tableau de recherche O(N)
    auto it = s_store.idIndex.find(id);
    return (it != s_store.idIndex.end()) ? it->second : wxString{};
}
