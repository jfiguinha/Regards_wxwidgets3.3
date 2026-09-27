#pragma once
#include <vector>
#include <memory>
#include <wx/string.h>
#include <RGBAQuad.h>
class CImageLoadingFormat;
class CEffectParameter;

class CModificationManager
{
public:
	CModificationManager(const wxString& folder);
	~CModificationManager();

	void SetNumModification(const unsigned int& numModification);
	unsigned int GetNbModification();
	unsigned int GetNumModification();

	// Prend désormais l'image de base en paramètre pour rejouer les filtres dessus
	CImageLoadingFormat* GetModification(const unsigned int& numModification);

	// Enregistre l'ID du filtre et ses paramètres au lieu d'écrire un fichier PNG
	void AddModification(const int& numEffect, CEffectParameter* effectParameter, const wxString& libelle);

	void Init(CImageLoadingFormat* bitmap);
	wxString GetModificationLibelle(const unsigned int& numModification);

	// Dans ModificationManager.h [2]
private:
	void EraseData();

	struct ModificationStep {
		int filterId;
		std::unique_ptr<CEffectParameter> parameter;
		wxString libelle;
	};

	int nbModification;
	int numModification;
	int orientation;
	wxString folder;
	wxString filenameBitmap;
	CRgbaquad color_quad;

	CImageLoadingFormat* baseBitmap;    // AJOUT : Stocke l'image originale décodée en RAM
	CImageLoadingFormat* currentBitmap; // MODIFICATION : Stockera l'image à l'étape 'numModification' [1, 2]

	std::vector<ModificationStep> historySteps;

};
