#pragma once
#include "Photos.h"
#include <mutex>
#include <shared_mutex>
#include <unordered_set>
#include <unordered_map>
#include <list>
#include <atomic>

class CThumbnailBuffer
{
public:
    static cv::Mat    GetPicture(const wxString& filename);
    static void       RemovePicture(const wxString& filename);
    static void       InitVectorList(PhotosVector* newVector);
    static CPhotos    GetVectorValue(int i);
    static int        GetVectorSize();
    static bool       FindValidFile(const wxString& localFilename);
    static wxString   FindPhotoById(int id);
    static wxString   FindPhotoByPath(const wxString& path);
    static std::shared_ptr<const PhotosVector> GetVectorList();

private:
    // ── LRU cache ──────────────────────────────────────────────────────────
    // list conserve l'ordre LRU (front = le plus ancien)
    // unordered_map donne accès O(1) à l'itérateur dans la list
    struct LruCache
    {
        using ListIt = std::list<wxString>::iterator;

        std::unordered_map<wxString, std::pair<cv::Mat, ListIt>> map;
        std::list<wxString>                                      order;

        // CORRECTION : Utilisation d'un std::mutex standard. L'écriture LRU ayant lieu
        // à CHAQUE lecture (get), le verrou partagé (shared_mutex) provoquait un interblocage.
        mutable std::mutex                                       mutex;
        int                                                      maxSize = 100;

        // Retourne l'image décodée si présente, sinon Mat vide
        // Promotionne l'entrée en "most recently used"
        cv::Mat get(const wxString& key);

        // Insère ou met à jour ; évince le plus ancien si dépassement
        void put(const wxString& key, cv::Mat data);

        void remove(const wxString& key);
    };

    // ── PhotosVector ───────────────────────────────────────────────────────
    struct VectorStore
    {
        std::shared_ptr<PhotosVector>    data;
        mutable std::shared_mutex        mutex;
        std::atomic<int>                 size{ 0 };
        std::unordered_set<wxString>     pathIndex; // Lookup par chemin en O(1)
        std::unordered_map<int, wxString> idIndex;  // NOUVEAU : Lookup par ID en O(1) pour accélérer FindPhotoById
    };

    static LruCache    s_cache;
    static VectorStore s_store;
};
