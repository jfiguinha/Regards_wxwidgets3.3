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

private:
	void EraseData();

	// Structure représentant une action de l'historique
	struct ModificationStep {
		int filterId;
		std::unique_ptr<CEffectParameter> parameter; // Allocation dynamique ou copie des paramètres
		wxString libelle;
	};

	int nbModification;
	int numModification;
	int orientation;
	wxString folder;
	wxString filenameBitmap;
	CRgbaquad color_quad;
	CImageLoadingFormat* currentBitmap;
	// Conteneur de l'historique des filtres appliqués
	std::vector<ModificationStep> historySteps;
};
