#include <header.h>
#include "ThumbnailFace.h"
#include "MainWindow.h"
#include "ViewerParamInit.h"
#include "ViewerParam.h"
#include <ThumbnailDataFace.h>
#include <SqlFindFacePhoto.h>
#include <ScrollbarWnd.h>
#include <InfosSeparationBarFace.h>
#include <SqlFaceRecognition.h>
#include <SqlFaceLabel.h>
#include <libPicture.h>
#include <SqlFacePhoto.h>
using namespace Regards::Viewer;
using namespace Regards::Sqlite;
using namespace Regards::Picture;

CThumbnailFace::CThumbnailFace(wxWindow* parent, wxWindowID id, const CThemeThumbnail& themeThumbnail,
	const bool& testValidity)
	: CThumbnailVerticalSeparator(parent, id, themeThumbnail, testValidity)
{
	barseparationHeight = 40;
	widthThumbnail = 0;
	heightThumbnail = 0;
	flipHorizontal = false;
	flipVertical = true;
	enableDragAndDrop = true;
	moveOnPaint = false;
	Connect(wxEVENT_SELECTALLICONE, wxCommandEventHandler(CThumbnailFace::OnSelectIcon));
}


CThumbnailFace::~CThumbnailFace(void)
{
	listSeparator.clear();
}

void CThumbnailFace::OnPictureClick(const int& numPhotoId)
{
	wxString iconeFilename = "";
	CIcone* numSelect = GetIconeById(numPhotoId);
	if (numSelect)
		if (numSelect->GetPtData())
			iconeFilename = numSelect->GetPtData()->GetFilename();

	auto mainWindow = static_cast<CMainWindow*>(this->FindWindowById(MAINVIEWERWINDOWID));
	if (mainWindow != nullptr)
	{
		wxString* filename = new wxString(iconeFilename);
		wxCommandEvent evt(wxEVENT_ONPICTURECLICKBYFILENAME);
		evt.SetClientData(filename);
		mainWindow->GetEventHandler()->AddPendingEvent(evt);
	}
}

void CThumbnailFace::EnableModification(const bool& enable)
{
	this->enableModification = enable;

	for (int i =0;i < listSeparator.size();i++)
	{
		CInfosSeparationBarFace * separator = (CInfosSeparationBarFace*)listSeparator[i].get();
		separator->EnableModification(enable);
	}
}


void CThumbnailFace::AddSeparatorBar(
	CIconeList* iconeListLocal,
	const wxString& libelle,
	const CFaceName& faceName,
	const std::vector<CFaceFilePath>& listPhotoFace,
	int& nbElement,
	const std::unordered_map<FaceKey, CIcone*, FaceKeyHash>& iconIndex)
{
	auto infosSeparationBar = std::make_unique<CInfosSeparationBarFace>(themeThumbnail.themeSeparation);

	infosSeparationBar->SetTitle(libelle);
	infosSeparationBar->SetParentWindow(this);
	infosSeparationBar->SetWidth(GetWindowWidth());
	infosSeparationBar->SetNumFace(faceName);

	const int firstElement = nbElement;

	CLibPicture libPicture;
	CSqlFacePhoto facePhoto;

	for (size_t i = 0; i < listPhotoFace.size(); ++i)
	{
		const auto& photo = listPhotoFace[i];

		const int elementIndex =
			firstElement + static_cast<int>(i);

		infosSeparationBar->listElement.push_back(elementIndex);

		FaceKey key
		{
			photo.faceFilePath,
			photo.numFace
		};

		auto it = iconIndex.find(key);

		if (it != iconIndex.end())
		{
			auto* icone = it->second;

			auto* data =
				static_cast<CThumbnailDataFace*>(icone->GetPtData());

			data->SetNumElement(elementIndex);
			icone->SetNumElement(elementIndex);

			continue;
		}

		auto* thumbnailData =
			new CThumbnailDataFace(
				photo.faceFilePath,
				photo.numFace);

		thumbnailData->SetNumPhotoId(photo.numPhoto);
		thumbnailData->SetNumElement(elementIndex);

		if (libPicture.TestIsVideo(thumbnailData->GetFilename()))
		{
			thumbnailData->SetNumFrame(
				facePhoto.GetVideoFacePosition(photo.numFace));
		}

		auto* pBitmapIcone =
			new CIcone(thumbnailData);

		pBitmapIcone->ShowSelectButton(true);
		pBitmapIcone->SetNumElement(elementIndex);
		pBitmapIcone->SetTheme(themeThumbnail.themeIcone);
		pBitmapIcone->SetShowDelete(true);
		pBitmapIcone->SetFilename(photo.faceFilePath);

		iconeListLocal->AddElement(pBitmapIcone);
	}

	nbElement += static_cast<int>(listPhotoFace.size());

	if (!listPhotoFace.empty())
	{
		listSeparator.push_back(std::move(infosSeparationBar));
	}
}

bool CThumbnailFace::ItemCompFonctFindFaceElement(wxString filepath, int numFace, CIcone* icone)
/* Définit une fonction. */
{
	if (icone != nullptr)
	{
		auto data = static_cast<CThumbnailDataFace*>(icone->GetPtData());
		if (data->GetFilename() == filepath && numFace == data->GetNumFace())
		{
			return true;
		}
	}
	return false;
}

CIcone* CThumbnailFace::FindFaceElement(wxString filepath, int numFace)
{
	pItemCompFonctFace _pf = &ItemCompFonctFindFaceElement;
	return iconeList->FindFaceElement(filepath, numFace, &_pf);
}

void CThumbnailFace::EraseData()
{
	iconeList->EraseThumbnailList();
	listSeparator.clear();
}


void CThumbnailFace::InitListFace()
{	
	iconeList->EraseThumbnailListWithIcon();

	return;

	auto viewerParam = CMainParamInit::getInstance();
	double pertinence = 0.0;
    int nbElement = 0;
	if (viewerParam != nullptr)
		pertinence = viewerParam->GetPertinenceValue();

	CIconeList* newIconeList = new CIconeList();
	CSqlFindFacePhoto sqlFindFacePhoto;
	std::vector<CFaceFilePath> listPhotoFace = sqlFindFacePhoto.GetListAllPhotoFace(pertinence);
	std::unordered_set<FaceKey, FaceKeyHash> validFaces;

	for (const auto& face : listPhotoFace)
	{
		validFaces.insert(
			{
				face.faceFilePath,
				face.numFace
			});
	}

	

	for (int i = 0; i < iconeList->GetNbElement(); ++i)
	{
		auto* icone = iconeList->GetElement(i);
		if (icone == nullptr)
		{
			iconeList->RemoveElement(i);
			--i;
			continue;
		}
			

		auto* data =
			static_cast<CThumbnailDataFace*>(icone->GetPtData());

		FaceKey key
		{
			data->GetFilename(),
			data->GetNumFace()
		};

		if (validFaces.find(key) != validFaces.end())
		{
			newIconeList->AddElement(icone);
		}
		else
		{
			iconeList->RemoveElement(i);
			--i;
		}
	}
}

void CThumbnailFace::ReinitId()
{
	for (int i = 0; i < iconeList->GetNbElement(); ++i)
	{
		auto* icone = iconeList->GetElement(i);
		if (icone == nullptr)
			continue;

		if (auto* data = static_cast<CThumbnailDataFace*>(icone->GetPtData()))
		{
			data->SetNumElement(i);
		}
	}
}

void CThumbnailFace::init()
{
	auto viewerParam = CMainParamInit::getInstance();
	threadDataProcess = false;
	double pertinence = 0.0;
	if (viewerParam != nullptr)
		pertinence = viewerParam->GetPertinenceValue();


	InitListFace();

	std::unordered_map<FaceKey, CIcone*, FaceKeyHash> iconIndex;
	
	/*
	for (int i = 0; i < iconeList->GetNbElement(); ++i)
	{
		auto* icone = iconeList->GetElement(i);
		if (icone == nullptr)
			continue;

		icone->SetNumElement(i);

		if (auto* data =
			static_cast<CThumbnailDataFace*>(icone->GetPtData()))
		{
			data->SetNumElement(i);

			iconIndex.insert(
				{
					{ data->GetFilename(), data->GetNumFace() },
					icone
				});
		}
	}
	*/


	listSeparator.clear();

	nbElement = 0;

	CSqlFindFacePhoto sqlFindFacePhoto;
	std::vector<CFaceName> listFace = sqlFindFacePhoto.GetListFaceName();
	for (int i = 0; i < listFace.size(); i++)
	{
		std::vector<CFaceFilePath> listPhotoFace = sqlFindFacePhoto.GetListPhotoFace(listFace.at(i).numFace, pertinence);
		if(listPhotoFace.size() > 0)
			AddSeparatorBar(iconeList.get(), listFace.at(i).faceName, listFace.at(i), listPhotoFace, nbElement, iconIndex);
	}


	nbElementInIconeList = iconeList->GetNbElement();

	AfterSetList();

	thumbnailPos = 0;

	threadDataProcess = true;

	widthThumbnail = 0;
	heightThumbnail = 0;
	ResizeThumbnail();

	CIcone* actif = iconeList->FindElementByPhotoId(numSelectPhotoId);
	if (actif)
		actif->SetSelected(true);

	needToRefresh = true;
}

void CThumbnailFace::SyncWithDatabase()
{
	auto viewerParam = CMainParamInit::getInstance();
	double pertinence = 0.0;
	if (viewerParam != nullptr)
		pertinence = viewerParam->GetPertinenceValue();

	CSqlFindFacePhoto sqlFindFacePhoto;
	std::vector<CFaceName> listFaceDB = sqlFindFacePhoto.GetListFaceName();

	// Bloquer temporairement le rafraîchissement UI pendant la synchronisation
	threadDataProcess = false;

	// -------------------------------------------------------------------------
	// ÉTAPE 1 : Extraire et conserver TOUTES les icônes valides actuellement en mémoire
	// -------------------------------------------------------------------------
	// On crée une map pour pouvoir récupérer instantanément une icône existante (avec son image chargée)
	// à partir de sa clé unique {filepath, numFace}.
	std::unordered_map<FaceKey, CIcone*, FaceKeyHash> memoryIconMap;

	for (int i = 0; i < iconeList->GetNbElement(); ++i)
	{
		CIcone* icone = iconeList->GetElement(i);
		if (icone == nullptr) continue;

		auto* data = static_cast<CThumbnailDataFace*>(icone->GetPtData());
		if (data != nullptr)
		{
			FaceKey key{ data->GetFilename(), data->GetNumFace() };
			memoryIconMap[key] = icone;
		}
	}

	// -------------------------------------------------------------------------
	// ÉTAPE 2 : Préparer les nouveaux conteneurs propres
	// -------------------------------------------------------------------------
	// On détache les icônes de la liste actuelle SANS détruire les objets CIcone sous-jacents
	iconeList->EraseThumbnailList();
	listSeparator.clear();

	CLibPicture libPicture;
	CSqlFacePhoto facePhoto;
	int globalElementIndex = 0; // Compteur unique et séquentiel global pour la grille

	// -------------------------------------------------------------------------
	// ÉTAPE 3 : Reconstruction synchronisée (Séparateurs + Icônes dans le MÊME ordre)
	// -------------------------------------------------------------------------
	for (int i = 0; i < listFaceDB.size(); i++)
	{
		const auto& currentFace = listFaceDB.at(i);
		std::vector<CFaceFilePath> photosInDB = sqlFindFacePhoto.GetListPhotoFace(currentFace.numFace, pertinence);

		if (photosInDB.empty()) continue;

		// Création du bloc de séparation (En-tête du groupe de visage)
		auto infosSeparationBar = std::make_unique<CInfosSeparationBarFace>(themeThumbnail.themeSeparation);
		infosSeparationBar->SetTitle(currentFace.faceName);
		infosSeparationBar->SetParentWindow(this);
		infosSeparationBar->SetWidth(GetWindowWidth());
		infosSeparationBar->SetNumFace(currentFace);
		infosSeparationBar->EnableModification(this->enableModification);

		for (const auto& photo : photosInDB)
		{
			FaceKey key{ photo.faceFilePath, photo.numFace };
			auto it = memoryIconMap.find(key);

			CIcone* pBitmapIcone = nullptr;

			if (it != memoryIconMap.end())
			{
				// CAS 1 : L'image existait déjà en mémoire. On la RÉUTILISE (Pas de clignotement / rechargement)
				pBitmapIcone = it->second;

				// On met à jour ses index internes pour correspondre à sa nouvelle position ordonnée
				pBitmapIcone->SetNumElement(globalElementIndex);
				if (auto* data = static_cast<CThumbnailDataFace*>(pBitmapIcone->GetPtData()))
				{
					data->SetNumElement(globalElementIndex);
				}

				// On la retire de la map temporaire pour marquer qu'elle est réutilisée
				memoryIconMap.erase(it);
			}
			else
			{
				// CAS 2 : C'est un NOUVEAU visage détecté -> instanciation de l'icône
				auto* thumbnailData = new CThumbnailDataFace(photo.faceFilePath, photo.numFace);
				thumbnailData->SetNumPhotoId(photo.numPhoto);
				thumbnailData->SetNumElement(globalElementIndex);

				if (libPicture.TestIsVideo(thumbnailData->GetFilename()))
				{
					thumbnailData->SetNumFrame(facePhoto.GetVideoFacePosition(photo.numFace));
				}

				pBitmapIcone = new CIcone(thumbnailData);
				pBitmapIcone->ShowSelectButton(this->check);
				pBitmapIcone->SetNumElement(globalElementIndex);
				pBitmapIcone->SetTheme(themeThumbnail.themeIcone);
				pBitmapIcone->SetShowDelete(true);
				pBitmapIcone->SetFilename(photo.faceFilePath);
			}

			// On insère l'icône dans la liste globale à sa position exacte de tri
			iconeList->AddElement(pBitmapIcone);

			// On lie cette icône au bloc séparateur visuel actuel
			infosSeparationBar->listElement.push_back(globalElementIndex);

			globalElementIndex++;
		}

		if (!infosSeparationBar->listElement.empty())
		{
			listSeparator.push_back(std::move(infosSeparationBar));
		}
	}

	// -------------------------------------------------------------------------
	// ÉTAPE 4 : Nettoyage de la mémoire obsolète
	// -------------------------------------------------------------------------
	// Toutes les icônes restant dans la map n'existent plus du tout dans la base de données.
	// On doit explicitement détruire ces objets pour éviter les fuites de mémoire.
	for (auto& pair : memoryIconMap)
	{
		if (pair.second != nullptr)
		{
			delete pair.second; // Suppression propre de l'objet CIcone obsolète
		}
	}

	// -------------------------------------------------------------------------
	// ÉTAPE 5 : Finalisation de l'affichage
	// -------------------------------------------------------------------------
	nbElement = globalElementIndex;
	nbElementInIconeList = iconeList->GetNbElement();

	AfterSetList();

	widthThumbnail = 0;
	heightThumbnail = 0;
	ResizeThumbnail();

	threadDataProcess = true;
	needToRefresh = true;
}



void CThumbnailFace::UpdateNewFacesFromDatabase()
{
	auto viewerParam = CMainParamInit::getInstance();
	double pertinence = 0.0;
	if (viewerParam != nullptr)
		pertinence = viewerParam->GetPertinenceValue();

	CSqlFindFacePhoto sqlFindFacePhoto;
	std::vector<CFaceName> listFace = sqlFindFacePhoto.GetListFaceName();

	// Bloquer temporairement les événements de dessin pendant la mise à jour
	threadDataProcess = false;

	bool elementsAdded = false;

	for (int i = 0; i < listFace.size(); i++)
	{
		std::vector<CFaceFilePath> listPhotoFace = sqlFindFacePhoto.GetListPhotoFace(listFace.at(i).numFace, pertinence);
		if (listPhotoFace.empty())
			continue;

		// 1. Rechercher si le séparateur (la catégorie de visage) existe déjà
		CInfosSeparationBarFace* existingSeparator = nullptr;
		for (auto& separatorBar : listSeparator)
		{
			auto* faceBar = static_cast<CInfosSeparationBarFace*>(separatorBar.get());
			if (faceBar != nullptr && faceBar->GetNumFace() == listFace.at(i).numFace)
			{
				existingSeparator = faceBar;
				break;
			}
		}

		// 2. Filtrer uniquement les photos de ce visage qui ne sont pas encore affichées
		std::vector<CFaceFilePath> newPhotosForThisFace;
		for (const auto& photo : listPhotoFace)
		{
			// Utilise votre méthode native FindFaceElement pour vérifier la présence
			if (FindFaceElement(photo.faceFilePath, photo.numFace) == nullptr)
			{
				newPhotosForThisFace.push_back(photo);
			}
		}

		// 3. S'il y a des nouveautés, on les injecte de manière incrémentale
		if (!newPhotosForThisFace.empty())
		{
			elementsAdded = true;

			// Si la catégorie n'existait pas du tout, on utilise votre méthode standard
			if (existingSeparator == nullptr)
			{
				std::unordered_map<FaceKey, CIcone*, FaceKeyHash> emptyIndex;
				AddSeparatorBar(iconeList.get(), listFace.at(i).faceName, listFace.at(i), newPhotosForThisFace, nbElement, emptyIndex);
			}
			else
			{
				// Si elle existe, on ajoute les icônes à la fin d'iconeList globale, 
				// et on lie leurs nouveaux index à ce séparateur précis
				CLibPicture libPicture;
				CSqlFacePhoto facePhoto;

				for (const auto& photo : newPhotosForThisFace)
				{
					// Calcul du nouvel index dans le tableau global d'icones
					int elementIndex = iconeList->GetNbElement();

					auto* thumbnailData = new CThumbnailDataFace(photo.faceFilePath, photo.numFace);
					thumbnailData->SetNumPhotoId(photo.numPhoto);
					thumbnailData->SetNumElement(elementIndex);

					if (libPicture.TestIsVideo(thumbnailData->GetFilename()))
					{
						thumbnailData->SetNumFrame(facePhoto.GetVideoFacePosition(photo.numFace));
					}

					auto* pBitmapIcone = new CIcone(thumbnailData);
					pBitmapIcone->ShowSelectButton(this->check);
					pBitmapIcone->SetNumElement(elementIndex);
					pBitmapIcone->SetTheme(themeThumbnail.themeIcone);
					pBitmapIcone->SetShowDelete(true);
					pBitmapIcone->SetFilename(photo.faceFilePath);

					// Insertion physique dans la grille globale sans rien détruire
					iconeList->AddElement(pBitmapIcone);

					// Liaison de l'index de l'icône au séparateur graphique concerné
					existingSeparator->listElement.push_back(elementIndex);
					nbElement++;
				}
			}
		}
	}

	if (elementsAdded)
	{
		nbElementInIconeList = iconeList->GetNbElement();

		// Recalculer les dimensions de la grille de vignettes sans tout réinitialiser
		widthThumbnail = 0;
		heightThumbnail = 0;
		ResizeThumbnail();
	}

	threadDataProcess = true;
	needToRefresh = true;
}


bool CThumbnailFace::ItemCompFonctWithVScroll(int x, int y, CIcone* icone, CWindowMain* parent)
/* Définit une fonction. */
{
	if (icone != nullptr && parent != nullptr)
	{
		wxRect rc = icone->GetPos();
		if ((rc.x < x && x < (rc.x + rc.width)) && (rc.y < y && y < (rc.height + rc.y)))
		{
			return true;
		}
	}
	return false;
}

CIcone* CThumbnailFace::FindElementWithVScroll(const int& xPos, const int& yPos)
{
	pItemCompFonct _pf = &ItemCompFonctWithVScroll;
	return iconeList->FindElementByPosition(xPos, yPos, &_pf, this);
}




void CThumbnailFace::MoveIcone(const int& numElement, const int& numFace)
{
	for (auto& separatorBar : listSeparator)
	{
		auto infosSeparationBarFace = static_cast<CInfosSeparationBarFace*>(separatorBar.get());
		if (numFace == infosSeparationBarFace->GetNumFace())
		{
			infosSeparationBarFace->listElement.push_back(numElement);
			return;
		}
	}
}

void CThumbnailFace::DeleteEmptyFace()
{
	CSqlFaceLabel sqlfaceLabel;

	auto it = listSeparator.begin();

	while (it != listSeparator.end())
	{
		auto* faceBar = static_cast<CInfosSeparationBarFace*>(it->get());

		if (faceBar != nullptr &&
			faceBar->listElement.empty())
		{
			sqlfaceLabel.DeleteFaceLabelDatabase(
				faceBar->GetNumFace());

			it = listSeparator.erase(it);
		}
		else
		{
			++it;
		}
	}

	CSqlFacePhoto facePhoto;
	facePhoto.RebuildLink();
}

//-----------------------------------------------------------------
//
//-----------------------------------------------------------------
void CThumbnailFace::MoveFace(const wxString& faceName)
{
	CSqlFaceRecognition faceRecognition;
	CSqlFaceLabel sqlfaceLabel;
	int numFace = sqlfaceLabel.GetNumFace(faceName);

	for (auto& separatorBar : listSeparator)
	{
		if (separatorBar != nullptr)
		{
			vector<int> numElementToDelete;
			for (int i = 0; i < separatorBar->listElement.size(); i++)
			{
				int numElement = separatorBar->listElement.at(i);
				CIcone* icone = iconeList->GetElement(numElement);
				if (icone != nullptr)
				{
					if (icone->IsChecked())
					{
						auto thumbnailData = static_cast<CThumbnailDataFace*>(icone->GetPtData());
						int numFaceCompatible = faceRecognition.GetCompatibleFace(thumbnailData->GetNumFace());
						if (numFaceCompatible != numFace)
						{
							MoveIcone(numElement, numFace);
							faceRecognition.MoveFaceRecognition(thumbnailData->GetNumFace(), numFace);
							separatorBar->listElement.erase(separatorBar->listElement.begin() + i);
							i--;
						}
					}
					icone->SetChecked(false);
				}
			}
		}
	}

	DeleteEmptyFace();


	widthThumbnail = 0;
	heightThumbnail = 0;
	ResizeThumbnail();

	wxWindow* mainWnd = this->FindWindowById(MAINVIEWERWINDOWID);
	auto eventChange = new wxCommandEvent(wxEVT_CRITERIACHANGE);
	wxQueueEvent(mainWnd, eventChange);

	init();
}

vector<int> CThumbnailFace::GetFaceSelectID()
{
	vector<int> listFace;
	for (auto& separatorBar : listSeparator)
	{
		if (separatorBar != nullptr)
		{
			vector<int> numElementToDelete;
			for (int i = 0; i < separatorBar->listElement.size(); i++)
			{
				int numElement = separatorBar->listElement.at(i);
				CIcone* icone = iconeList->GetElement(numElement);
				if (icone != nullptr)
				{
					bool needToMove = false;
					if (icone->IsChecked())
					{
						auto thumbnailData = static_cast<CThumbnailDataFace*>(icone->GetPtData());
						listFace.push_back(thumbnailData->GetNumFace());
					}
				}
			}
		}
	}
	return listFace;
}

//-----------------------------------------------------------------
//
//-----------------------------------------------------------------
CInfosSeparationBar* CThumbnailFace::FindSeparatorElement(const int& xPos, const int& yPos)
{
	int x = xPos + posLargeur;
	int y = yPos + posHauteur;

	for (auto& separatorBar : listSeparator)
	{
		if (separatorBar != nullptr)
		{
			wxRect rc = separatorBar->GetPos();
			if ((rc.x < x && x < (rc.x + rc.width)) && (rc.y < y && y < (rc.height + rc.y)))
			{
				return separatorBar.get();
			}
		}
	}
	return nullptr;
}


int CThumbnailFace::FindSeparatorFace(const int& xPos, const int& yPos)
{
	//int x = xPos + posLargeur;
	int y = yPos + posHauteur;
	int numFace = 0;
	for (int i = 0; i < listSeparator.size(); i++)
	{
		if (i == listSeparator.size() - 1)
		{
			auto faceSeparator = static_cast<CInfosSeparationBarFace*>(listSeparator[i].get()); // static_cast<CInfosSeparationBarFace*>(listSeparator[i]);
			numFace = faceSeparator->GetNumFace();
		}
		else
		{
			auto separatorBarFirst = static_cast<CInfosSeparationBarFace*>(listSeparator[i].get());
			auto separatorBarSecond = static_cast<CInfosSeparationBarFace*>(listSeparator[i + 1].get());
			wxRect rcFirst = separatorBarFirst->GetPos();
			wxRect rcSecond = separatorBarSecond->GetPos();
			if (y > (rcFirst.y + rcFirst.height) && y < rcSecond.y)
			{
				numFace = separatorBarFirst->GetNumFace();
				break;
			}
		}
	}
	return numFace;
}

void CThumbnailFace::OnMouseRelease(const int& x, const int& y)
{
	bool faceMove = false;
	int numFace = FindSeparatorFace(x, y);
	CSqlFaceRecognition faceRecognition;

	if (numFace != 0)
	{
		for (auto& separatorBar : listSeparator)
		{
			if (separatorBar != nullptr)
			{
				vector<int> numElementToDelete;
				for (int i = 0; i < separatorBar->listElement.size(); i++)
				{
					int numElement = separatorBar->listElement.at(i);
					CIcone* icone = iconeList->GetElement(numElement);
					if (icone != nullptr)
					{
						bool needToMove = false;
						if (icone->IsChecked())
						{
							auto thumbnailData = static_cast<CThumbnailDataFace*>(icone->GetPtData());
							int numFaceCompatible = faceRecognition.GetCompatibleFace(thumbnailData->GetNumFace());
							if (numFaceCompatible != numFace)
							{
								faceMove = true;
								needToMove = true;
								MoveIcone(numElement, numFace);
								faceRecognition.MoveFaceRecognition(thumbnailData->GetNumFace(), numFace);
								separatorBar->listElement.erase(separatorBar->listElement.begin() + i);
								i--;
							}
						}
						if (needToMove)
							icone->SetChecked(false);
					}
				}
			}
		}
	}

	if (faceMove)
	{
		DeleteEmptyFace();
		widthThumbnail = 0;
		heightThumbnail = 0;
		ResizeThumbnail();

		wxWindow* mainWnd = this->FindWindowById(MAINVIEWERWINDOWID);
		auto eventChange = new wxCommandEvent(wxEVT_CRITERIACHANGE);
		wxQueueEvent(mainWnd, eventChange);
	}
}

void CThumbnailFace::FindOtherElement(wxDC* dc, const int& x, const int& y)
{
	CInfosSeparationBar* separator = FindSeparatorElement(x, y);
	if (separator != nullptr)
	{
		auto faceSeparator = static_cast<CInfosSeparationBarFace*>(separator);
		if (faceSeparator != nullptr)
		{
			bool select = faceSeparator->GetSelected();
			faceSeparator->OnClick(x, y);
			if (select != faceSeparator->GetSelected())
			{
				auto eventChange = new wxCommandEvent(wxEVENT_SELECTALLICONE);
				eventChange->SetClientData(faceSeparator);
				wxQueueEvent(this, eventChange);
			}
			mouseClickBlock = false;
		}
	}
}

void CThumbnailFace::OnSelectIcon(wxCommandEvent& event)
{
	if (!TestIfEnable())
		return;
	auto faceSeparator = static_cast<CInfosSeparationBarFace*>(event.GetClientData());
	if (faceSeparator != nullptr)
	{
		for (auto numElement : faceSeparator->listElement)
		{
			CIcone* icone = iconeList->GetElement(numElement);
			if (icone != nullptr)
			{
				if (faceSeparator->GetSelected())
					icone->SetChecked(true);
				else
					icone->SetChecked(false);
			}
		}
	}
}


bool CThumbnailFace::TestIfEnable()
{
	if (!enableModification)
	{
		wxMessageBox("Face detection is working. Please wait", "Informations");
		return false;
	}
	return true;
}


void CThumbnailFace::DeleteIcone(CIcone* numSelect)
{
	if (!TestIfEnable())
		return;

	auto face_thumbnail = static_cast<CThumbnailDataFace*>(numSelect->GetPtData());
	if (face_thumbnail != nullptr)
	{
		CSqlFacePhoto facePhoto;
		facePhoto.DeleteNumFace(face_thumbnail->GetNumFace());

		DeleteEmptyFace();

		wxWindow* mainWnd = this->FindWindowById(MAINVIEWERWINDOWID);
		auto eventChange = new wxCommandEvent(wxEVT_CRITERIACHANGE);
		wxQueueEvent(mainWnd, eventChange);

		init();
	}
}

bool CThumbnailFace::ItemCompFonct(int xPos, int yPos, CIcone* icone, CWindowMain* parent) /* Définit une fonction. */
{
	if (icone != nullptr && parent != nullptr)
	{
		auto face = static_cast<CThumbnailFace*>(parent);
		wxRect rc = icone->GetPos();
		int left = rc.x - face->posLargeur;
		int right = rc.x + rc.width - face->posLargeur;
		int top = rc.y - face->posHauteur;
		int bottom = rc.y + rc.height - face->posHauteur;
		if ((left < xPos && xPos < right) && (top < yPos && yPos < bottom))
		{
			return true;
		}
	}
	return false;
}

CIcone* CThumbnailFace::FindElement(const int& xPos, const int& yPos)
{
	if (!threadDataProcess)
		return nullptr;

	pItemCompFonct _pf = &ItemCompFonct;
	return iconeList->FindElementByPosition(xPos, yPos, &_pf, this);
}
