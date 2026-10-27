#pragma once
#include <vector>
#include <memory>
#include <cstdint>
#include <wx/string.h>
#include <RGBAQuad.h>
#include "LayerElement.h"

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

    CLayerList* GetLayersForModification(const unsigned int& targetModification);

    void AddModification(const int& numEffect, CEffectParameter* effectParameter, const wxString& libelle, CLayerList* currentLayers, int64_t targetLayerId);
    void AddModificationState(const wxString& libelle, CLayerList* currentLayers, int64_t targetLayerId);

    void Init(CImageLoadingFormat* bitmap, CLayerList* currentLayers);
    wxString GetModificationLibelle(const unsigned int& numModification) const;

    void EraseData();

private:
    struct ModificationStep {
        int filterId = -1;
        int64_t targetLayerId = 0;
        wxString libelle;

        // Stockage des chemins de fichiers au lieu des objets
        wxString layersFilePath;
        wxString parameterFilePath;
    };

    void AddStep(int filterId, CEffectParameter* parameter, size_t parameterSize, const wxString& libelle,
        CLayerList* currentLayers, int64_t targetLayerId);

    void DeleteStepFiles(const ModificationStep& step);

    unsigned int numModification = 0;
    CRgbaquad color_quad;
    wxString m_sessionFolder; // Dossier unique pour cette session d'image
    std::vector<ModificationStep> historySteps;
};
