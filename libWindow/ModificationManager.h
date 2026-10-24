#pragma once
#include <vector>
#include <memory>
#include <wx/string.h>
#include <RGBAQuad.h>
#include "LayerElement.h" // Nécessaire pour manipuler CLayerList

class CImageLoadingFormat;
class CEffectParameter;

class CModificationManager
{
public:
    CModificationManager(const wxString& folder);
    ~CModificationManager();


    bool Undo();
    bool Redo();
    bool CanUndo() const;
    bool CanRedo() const;

    void SetNumModification(const unsigned int& numModification);
    unsigned int GetNbModification() const;
    unsigned int GetNumModification() const;

    wxString GetCurrentKey();

    // Récupère l'état des calques pour une modification donnée (pour Undo/Redo/Navigation dans l'historique).
    // Retourne une copie profonde allouée avec new : l'appelant en devient propriétaire.
    // Retourne nullptr si l'index est invalide (jamais une liste vide, qui effacerait l'affichage).
    CLayerList* GetLayersForModification(const unsigned int& targetModification);

    // Ajout d'une modification basée sur un effet (filtre)
    void AddModification(const int& numEffect, CEffectParameter* effectParameter, const wxString& libelle, CLayerList* currentLayers);

    // Ajout d'une modification globale directe de l'état des calques (ex: Collage, Suppression, Redimensionnement, etc.)
    // currentLayers est copié (profondeur) : l'appelant garde la propriété de son pointeur.
    void AddModificationState(const wxString& libelle, CLayerList* currentLayers);

    void Init(CImageLoadingFormat* bitmap, CLayerList* currentLayers);
    wxString GetModificationLibelle(const unsigned int& numModification) const;

    void EraseData();

private:
    struct ModificationStep {
        int filterId = -1;
        uint64_t targetLayerId = 0; // <--- Optionnel : pour savoir quel calque précis a subi l'effet
        std::unique_ptr<CEffectParameter> parameter;
        wxString libelle;
        std::unique_ptr<CLayerList> layers; // Propriétaire de l'instantané : libéré avec l'étape
        bool hasLayerState = false;
    };

    int nbModification;
    int numModification;
    int orientation;
    wxString folder;
    wxString filenameBitmap;
    CRgbaquad color_quad;

    CImageLoadingFormat* baseBitmap;    // Image originale décodée en RAM
    CImageLoadingFormat* currentBitmap; // Image courante en cache

    std::vector<ModificationStep> historySteps;
};