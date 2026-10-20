// ReSharper disable All
#include <header.h>
#include "InfoEffect.h"
#include "TreeElementTexte.h"
#include "TreeElementTriangle.h"
#include "ModificationManager.h"
#include <ImageLoadingFormat.h>
#include "MainTheme.h"
#include "MainThemeInit.h"
#include "BitmapWnd3d.h"
#include "BitmapWndViewer.h"
#include <libPicture.h>
#include <TreeData.h>
#include <TreeElementControlInterface.h>
#include <ConvertUtility.h>
using namespace Regards::Window;
using namespace Regards::Control;
using namespace Regards::Picture;

CInfoEffect::CInfoEffect(CTreeElementControlInterface* interfaceControl, CModificationManager* modificationManager,
	int bitmapWindowId, wxString filename) : numEvent(0), yPos(0), index(0)
{
	this->filename = filename;
	this->bitmapWindowId = bitmapWindowId;
	widthPosition = 0;
	CMainTheme* viewerTheme = CMainThemeInit::getInstance();

	if (viewerTheme != nullptr)
		viewerTheme->GetTreeTheme(&themeTree);

	themeTree.themeTriangle.SetHeight(themeTree.GetRowHeight());
	themeTree.themeCheckbox.SetHeight(themeTree.GetRowHeight());
	themeTree.themeDelete.SetHeight(themeTree.GetRowHeight());
	themeTree.themeSlide.SetHeight(themeTree.GetRowHeight());
	themeTree.themeTexte.SetHeight(themeTree.GetRowHeight());
	eventControl = interfaceControl;
	themeTree.SetMargeX(5);
	this->modificationManager = modificationManager;
	rowWidth.push_back(0);
}

void CInfoEffect::SetTreeInterface(CTreeElementControlInterface* interfaceControl)
{
	eventControl = interfaceControl;
}

void CInfoEffect::Init(CImageLoadingFormat* bitmap, CLayerList * initialLayers, const int64_t& layerId, const wxString& libelle, const wxString& key)
{
	initLayerId = layerId;
	if (modificationManager != nullptr)
	{
		modificationManager->Init(bitmap, initialLayers);
	}
	InitTree(libelle, key);
}

void CInfoEffect::AddModification(const int& numEffect, CEffectParameter* effectParameter, const wxString& libelle, CLayerList* currentLayers)
{
	if (modificationManager == nullptr)
		return;

	const unsigned int numModification = modificationManager->GetNumModification();
	const unsigned int nbModification = modificationManager->GetNbModification();

	// Si l'utilisateur était revenu en arrière (Undo), on nettoie l'arbre graphique
	if (numModification < nbModification)
	{
		ClearData();

		InitTree(modificationManager->GetModificationLibelle(0), to_string(0));

		for (unsigned int i = 1; i <= numModification; i++)
		{
			wxString modificationString = modificationManager->GetModificationLibelle(i);
			modificationString.Replace(".", "@99");
			AddEvent("Effect." + modificationString, to_string(i));
			index++;
		}
	}

	wxString localLibelle = libelle;
	localLibelle.Replace(".", "@99");

	// Enregistrement de la modification avec le nouveau ModificationManager
	modificationManager->AddModification(numEffect, effectParameter, libelle, currentLayers);

	// Récupération du nouvel index généré par le manager
	wxString currentModifKey = GetNumModification();

	AddEvent("Effect." + localLibelle, currentModifKey);
	index++;
	SetActifElement(currentModifKey);
}

void CInfoEffect::ClickOnElement(
	CPositionElement* element,
	wxWindow* window,
	const int& x,
	const int& y,
	const int& posLargeur,
	const int& posHauteur)
{
	if (element == nullptr)
		return;

	CTreeElement* treeElement = element->GetTreeElement();
	auto treeData = element->GetTreeData();

	if (element->GetType() == ELEMENT_TEXTE)
	{
		if (treeData == nullptr)
			return;

		const wxString key = treeData->GetExifKey();

		if (modificationManager != nullptr)
		{
			unsigned int modif = 0;

			if (key != "History")
			{
				const long parsedIndex = wxAtol(key);

				if (parsedIndex < 0 ||
					static_cast<unsigned long>(parsedIndex) >=
					modificationManager->GetNbModification())
				{
					return;
				}

				modif = static_cast<unsigned int>(parsedIndex);
			}

			CLayerList* layers =
				modificationManager->GetLayersForModification(modif);

			if (layers == nullptr)
				return;

			modificationManager->SetNumModification(modif);
			RestoreLayersToViewer(layers, initLayerId);
		}

		SetActifElement(key);
	}
	else if (element->GetType() == ELEMENT_TRIANGLE)
	{
		auto treeElementTriangle =
			dynamic_cast<CTreeElementTriangle*>(treeElement);

		if (treeElementTriangle == nullptr)
			return;

		treeElementTriangle->ClickElement(
			window,
			(x + posLargeur) - element->GetX(),
			(y + posHauteur) - element->GetY());

		CreateElement(RenderMode::Update);

		if (eventControl)
			eventControl->UpdateTreeControl();
	}
}

void CInfoEffect::InitTree(const wxString& libelle, const wxString& key)
{
	numEvent = 1;
	index = 0;
	wxString localLibelle = libelle;
	localLibelle.Replace(".", "@99");
	AddEvent("Source." + localLibelle, key);
	index++;
	CreateElement(RenderMode::Create);
}

wxString CInfoEffect::GetNumModification()
{
	if (modificationManager != nullptr)
		return to_string(modificationManager->GetNumModification());
	return "0";
}

void CInfoEffect::RestoreLayersToViewer(CLayerList* layers, const int64_t& layerId)
{
	auto bitmapWindow = wxWindow::FindWindowById(bitmapWindowId);
	if (bitmapWindow != nullptr && layers != nullptr)
	{
		auto event = new wxCommandEvent(wxEVENT_UPDATELAYERMODIFICATION);

		// 1. On s'assure que l'image finale composite de cette étape est calculée et à jour
		layers->UpdatePictureToShow();

		// 2. On transmet la CLayerList complète à la fenêtre d'affichage (Viewer/Editor)
		// L'appelant (CBitmapEditor) pourra ainsi manipuler à nouveau chaque calque individuellement.
		event->SetClientData(layers);
		event->SetInt(layerId);
		wxQueueEvent(bitmapWindow, event);
	}
}


void CInfoEffect::MouseOver(wxDC* deviceContext, CPositionElement* element, const int& x, const int& y,
	const int& posLargeur, const int& posHauteur, bool& update)
{
	int xPos = 0;
	if (element->GetRow() > 0)
		xPos = GetWidthRow(element->GetRow() - 1);

	element->GetTreeElement()->MouseOver(deviceContext, element->GetX() + xPos, element->GetY(), update);
}

wxString CInfoEffect::GetFilename()
{
	return filename;
}

void CInfoEffect::SetActifElement(const wxString& key)
{
	for (auto posElement : vectorPosElement)
	{
		if (posElement != nullptr)
		{
			CTreeData* data = posElement->GetTreeData();
			if (data->GetExifKey() == key)
				data->SetActif(true);
			else
				data->SetActif(false);
		}
	}
	if (eventControl)
		eventControl->UpdateTreeControl();
}

void CInfoEffect::AddEvent(const wxString& libelle, const wxString& key)
{
	numEvent += 2;
	wxString localLibelle = libelle;
	localLibelle.Replace("@99", ".");
	int level = 0;
	wxStringTokenizer tokenizer(libelle, ".");

	while (tokenizer.HasMoreTokens())
	{
		wxString value = tokenizer.GetNextToken();
		value.Replace("@99", ".");
		const bool hasChildren = tokenizer.HasMoreTokens();
		auto* treeData = new CTreeData();
		treeData->SetKey(value);

		if (hasChildren)
		{
			treeData->SetIsParent(true);
			if (index > 0)
			{
				tree<CTreeData*>::iterator it;
				if (level == 0)
					it = FindKey(treeData->GetKey());
				else
					it = FindKey(treeData->GetKey(), child);

				if (it != nullptr)
				{
					child = it;
					++level;
					delete treeData;
					continue;
				}
			}

			if (level > 0)
				child = tr.append_child(child, treeData);
			else
			{
				treeData->SetExifKey("-1");
				child = tr.insert(top, treeData);
			}
		}
		else
		{
			treeData->SetIsParent(false);
			treeData->SetValue(localLibelle);
			treeData->SetExifKey(key);
			tr.append_child(child, treeData);
		}
		++level;
	}
	CreateElement(RenderMode::Update);
	if (eventControl)
		eventControl->UpdateTreeControl();
}

void CInfoEffect::CreateElement(RenderMode mode)
{
	if (mode == RenderMode::Create)
	{
		vectorPosElement.clear();
		vectorPosElementDynamic.clear();
	}
	else
	{
		for (CPositionElement* value : vectorPosElement)
		{
			if (value != nullptr)
			{
				CTreeElement* treeElement = value->GetTreeElement();
				if (treeElement != nullptr)
					treeElement->SetVisible(false);
			}
		}
	}

	tree<CTreeData*>::sibling_iterator it = tr.begin();
	auto itend = tr.end();
	yPos = 0;
	nbRow = 0;

	while (it != itend)
	{
		CTreeData* data = *it;
		const int profondeur = tr.depth(it);
		if (profondeur == 0)
		{
			int xPos = themeTree.GetMargeX();
			int widthElement = 0;
			bool isVisible = true;
			CTreeElementTriangle* tree_element_triangle = nullptr;
			CPositionElement* posElement = nullptr;

			posElement = RenderTriangle(data, xPos, yPos, isVisible, mode);
			xPos += posElement->GetWidth() + themeTree.GetMargeX();
			widthPosition = posElement->GetWidth() + themeTree.GetMargeX();
			tree_element_triangle = dynamic_cast<CTreeElementTriangle*>(posElement->GetTreeElement());

			posElement = RenderText(data, xPos, yPos, isVisible, mode);
			widthElement += xPos + posElement->GetWidth() + themeTree.GetMargeX();
			yPos += themeTree.GetRowHeight();
			nbRow++;
			if (rowWidth[0] < widthElement)
				rowWidth[0] = widthElement;

			if (mode == RenderMode::Update)
			{
				bool isOpen = false;
				if (tree_element_triangle != nullptr)
					isOpen = tree_element_triangle->GetOpen();

				if (isOpen)
					UpdateChildTree(it);
			}
		}
		++it;
	}
}

void CInfoEffect::UpdateScreenRatio()
{
	CreateElement(RenderMode::Update);
}

void CInfoEffect::UpdateChildTree(tree<CTreeData*>::sibling_iterator& parent)
{
	tree<CTreeData*>::sibling_iterator it = tree<CTreeData*>::begin(parent);

	for (auto i = 0; i < parent.number_of_children(); i++)
	{
		const bool isVisible = true;
		const int profondeur = tr.depth(it);
		CTreeData* data = *it;
		int xPos = widthPosition * (profondeur);

		CPositionElement* posElement = RenderText(data, xPos, yPos, isVisible, RenderMode::Update, true);
		const int widthElement = xPos + posElement->GetWidth() + themeTree.GetMargeX();
		yPos += themeTree.GetRowHeight();

		nbRow++;
		if (rowWidth[0] < widthElement)
			rowWidth[0] = widthElement;

		++it;
	}
}