#include <header.h>
#include "KeywordWnd.h"
#include <Criteria.h>
#include <SqlFindCriteria.h>
#include <SqlCriteria.h>
#include <SqlPhotos.h>
#include "TreeDataCategory.h"
#include <TreeElementTexte.h>
#include <TreeElementTriangle.h>
#include <TreeElementDelete.h>
#include <TreeElementCheckBox.h>
#include "ViewerParam.h"
#include <ThumbnailData.h>
#include <SqlFindCatalog.h>
#include <theme.h>
#include <SqlPhotoCriteria.h>
#include <TreeWindow.h>
#include <ScrollbarWnd.h>
#include <algorithm>
using namespace Regards::Sqlite;
using namespace Regards::Viewer;
using namespace Regards::Window;

namespace
{
	//Identifiant de la catégorie "Keyword" dans la base de critères
	constexpr int KEYWORD_CRITERIA_CATEGORY = 7;

	//Identifiant (factice) de la ligne "+" qui permet d'ajouter un mot-clé
	constexpr int ADD_KEYWORD_ROW = -1;

	//Lit le flag ('1' / '0') qui suit localkey dans states, defaultValue si la clé est absente
	bool ReadFlag(const wxString& states, const wxString& localkey, const bool defaultValue)
	{
		const size_t pos = states.find(localkey);
		if (pos == wxString::npos)
			return defaultValue;

		const size_t flagPos = pos + localkey.size();
		if (flagPos >= states.size())
			return false;

		return states[flagPos] == '1';
	}
}

CKeywordWnd::CKeywordWnd(CWindowMain* windowMain, CThemeTree* theme, CTreeElementControlInterface* interfaceControl)
	: windowMain(windowMain)
{
	if (theme != nullptr)
	{
		themeTree = *theme;
		const int rowHeight = themeTree.GetRowHeight();
		themeTree.themeTriangle.SetHeight(rowHeight);
		themeTree.themeCheckbox.SetHeight(rowHeight);
		themeTree.themeDelete.SetHeight(rowHeight);
		themeTree.themeSlide.SetHeight(rowHeight);
		themeTree.themeTexte.SetHeight(rowHeight);
	}

	eventControl = interfaceControl;
	rowWidth.push_back(0);
	rowWidth.push_back(0);

	// Style moderne : fond uni, filets, libelle a gauche / valeur a droite
	EnableModernStyle();
}

CKeywordWnd::~CKeywordWnd()
{
	CloseEditor();
}


void CKeywordWnd::Init(const wxString& filename)
{
	CancelEdit();

	CSqlFindCatalog sqlCatalog;
	CatalogVector catalogVector;
	CriteriaVector criteriaVector;
	CSqlPhotos photos;
	auto top = tr.begin();

	numPhotoId = photos.GetPhotoId(filename);

	idElement = 0;

	sqlCatalog.GetCatalog(&catalogVector);

	for (auto& catalog : catalogVector)
	{
		auto* treeData = new CTreeDataCategory();
		numCatalog = catalog.GetNumCatalog();
		treeData->SetNumElement(idElement);
		treeData->SetIdElement(-1);
		treeData->SetNumParent(0);
		treeData->SetIsParent(true);
		treeData->SetNumLevel(0);
		treeData->SetNumCatalog(numCatalog);
		treeData->SetKey("Keyword");
		treeData->SetExifKey("Keyword");
		treeData->SetTypeElement(TYPECATALOG);
		auto child = tr.insert(top, treeData);
		treeData->child = child;
		InitKeyWordCategorie(child, idElement);
	}

	photos.GetPhotoCriteriaByCategorie(&criteriaVector, filename, KEYWORD_CRITERIA_CATEGORY);

	for (auto& criteria : criteriaVector)
	{
		stateValue += std::to_string(criteria.GetId()) + ":" + criteria.GetLibelle() + ":1:";
	}

	RenderTree();
}


//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
void CKeywordWnd::InitKeyWordCategorie(tree<CTreeData*>::iterator parent, int numParent)
{
	//Récupération des catégories principales
	CSqlFindCriteria sqlCriteria;
	CriteriaVector criteriaVector;
	sqlCriteria.SearchCriteria(&criteriaVector, KEYWORD_CRITERIA_CATEGORY, 1);

	auto appendRow = [&](const int numCategorie, const wxString& exifKey, const wxString& key)
		{
			idElement++;
			auto* treeData = new CTreeDataCategory();
			treeData->SetNumElement(idElement);
			treeData->SetNumCatalog(numCatalog);
			treeData->SetIdElement(-1);
			treeData->SetNumCategorie(numCategorie);
			treeData->SetNumParent(numParent);
			treeData->SetIsParent(false);
			treeData->SetNumLevel(0);
			treeData->SetExifKey(exifKey);
			treeData->SetKey(key);
			treeData->SetTypeElement(TYPECATEGORIE);
			auto child = tr.append_child(parent, treeData);
			treeData->child = child;
		};

	for (auto& criteria : criteriaVector)
		appendRow(criteria.GetId(), std::to_string(criteria.GetId()), criteria.GetLibelle());

	//Dernière ligne : "+" pour ajouter un nouveau mot-clé
	appendRow(ADD_KEYWORD_ROW, "+", "+");
}

//------------------------------------------------------------------------------
// Prévient la fenêtre principale et la fenêtre des mots-clés qu'un critère a changé
//------------------------------------------------------------------------------
void CKeywordWnd::NotifyCriteriaChange(wxWindow* window)
{
	if (window == nullptr)
		return;

	if (wxWindow* mainWnd = window->FindWindowById(MAINVIEWERWINDOWID); mainWnd != nullptr)
		wxQueueEvent(mainWnd, new wxCommandEvent(wxEVT_CRITERIACHANGE));

	if (wxWindow* keyword = window->FindWindowById(KEYWORDCRITERIAWINDOWID); keyword != nullptr)
		wxQueueEvent(keyword, new wxCommandEvent(wxEVENT_REFRESHDATA));
}

//------------------------------------------------------------------------------
// Création d'un nouveau mot-clé ("New Keyword N") - ex traitement de WM_ADDKEYWORD
//------------------------------------------------------------------------------
void CKeywordWnd::AddKeyword(wxWindow* window)
{
	bool isNew = false;
	CSqlCriteria sqlCriteria;
	const int lastId = sqlCriteria.GetCriteriaId() + 1;
	const wxString value = wxString::Format("New Keyword %d", lastId);
	sqlCriteria.GetOrInsertCriteriaId(1, KEYWORD_CRITERIA_CATEGORY, value, isNew);

	if (isNew)
		NotifyCriteriaChange(window);
}

//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
void CKeywordWnd::ClickOnElement(CPositionElement* element, wxWindow* window, const int& x, const int& y,
	const int& posLargeur, const int& posHauteur)
{
	if (element == nullptr)
		return;

	if (element->GetType() == ELEMENT_CHECKBOX)
	{
		CSqlPhotoCriteria photoCriteria;
		auto* treeData = static_cast<CTreeDataCategory*>(element->GetTreeData());
		auto* checkBox = static_cast<CTreeElementCheckBox*>(element->GetTreeElement());

		checkBox->SetCheckState(!checkBox->GetCheckState());
		StoreCheckState(treeData->GetExifKey(), treeData->GetKey(), checkBox->GetCheckState());

		const int numCategorie = treeData->GetNumCategorie();
		if (checkBox->GetCheckState())
			photoCriteria.InsertPhotoCriteria(numPhotoId, numCategorie);
		else
			photoCriteria.DeletePhotoCriteria(numPhotoId, numCategorie);

		eventControl->UpdateTreeControl();
	}
	else if (element->GetType() == ELEMENT_DELETE)
	{
		auto* treeData = static_cast<CTreeDataCategory*>(element->GetTreeData());

		wxMessageDialog dialog(nullptr, _("Do you want to delete this criteria ?"), _("Confirmation"), wxOK | wxCANCEL);

		if (dialog.ShowModal() == wxID_OK)
		{
			CSqlCriteria sqlCriteria;
			CSqlPhotoCriteria sqlPhotoCriteria;

			const int numCriteria = wxAtoi(treeData->GetExifKey());
			sqlCriteria.DeletePhotoCriteria(1, numCriteria);
			sqlPhotoCriteria.DeleteCriteria(numCriteria);
			sqlCriteria.DeleteCriteria(numCriteria, KEYWORD_CRITERIA_CATEGORY);

			NotifyCriteriaChange(window);

			eventControl->UpdateTreeControl();
		}
	}
	else if (element->GetType() == ELEMENT_TEXTE)
	{
		auto* treeData = static_cast<CTreeDataCategory*>(element->GetTreeData());

		if (treeData->GetNumCategorie() == ADD_KEYWORD_ROW)
			AddKeyword(window);
		else
			//Le renommage se fait par édition inline, directement sur le libellé
			BeginEditKeyword(element, window, posLargeur, posHauteur);
	}
}


void CKeywordWnd::UpdateScreenRatio()
{
	CancelEdit();
	RenderTree();
	eventControl->UpdateTreeControl();
}

//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
bool CKeywordWnd::GetTriangleState(const wxString& exifKey, const wxString& key) const
{
	return ReadFlag(stateTriangleValue, exifKey + ":" + key + ":", true);
}

//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
bool CKeywordWnd::GetCheckState(const wxString& exifKey, const wxString& key) const
{
	return ReadFlag(stateValue, exifKey + ":" + key + ":", false);
}

//------------------------------------------------------------------------------
// Memorise l'etat d'une case a cocher (l'arbre est reconstruit a chaque relayout)
//------------------------------------------------------------------------------
void CKeywordWnd::StoreCheckState(const wxString& exifKey, const wxString& key, const bool state)
{
	const wxString localkey = exifKey + ":" + key + ":";
	const size_t pos = stateValue.find(localkey);
	if (pos == wxString::npos)
	{
		stateValue += localkey + (state ? "1:" : "0:");
		return;
	}

	const size_t flagPos = pos + localkey.size();
	if (flagPos < stateValue.size())
		stateValue.replace(flagPos, 1, state ? "1" : "0");
	else
		stateValue += (state ? "1:" : "0:");
}

void CKeywordWnd::Resize()
{
	CancelEdit();
	RenderTree();
}


CPositionElement* CKeywordWnd::RenderDelete(
	CTreeDataCategory* data,
	int& xPos,
	int& yPos,
	bool visible)
{
	auto* treeElementDelete = CreateDeleteElement(themeTree.GetRowWidth(), themeTree.GetRowHeight());
	treeElementDelete->SetVisible(visible);
	return CreatePositionElement(xPos, yPos, nbRow, 1, treeElementDelete->GetWidth(),
		treeElementDelete->GetHeight(), ELEMENT_DELETE, treeElementDelete,
		data);
}

void CKeywordWnd::OnModernRelayout()
{
	RenderTree();
}

void CKeywordWnd::RenderTree()
{
	// Les anciens elements sont liberes avant la reconstruction (RenderTree peut etre rappele a chaque redimensionnement)
	for (CPositionElement* posElement : vectorPosElement)
		delete posElement;

	vectorPosElement.clear();
	vectorPosElementDynamic.clear();

	ModernUpdateMetrics(true);

	tree<CTreeData*>::sibling_iterator it = tr.begin();
	const tree<CTreeData*>::iterator itend = tr.end();
	yPos = 0;
	nbRow = 0;

	while (it != itend)
	{
		CTreeData* data = *it;
		if (tr.depth(it) == 0)
		{
			CTreeElementTriangle* treeElementTriangle = ModernRenderHeaderRow(data, RenderMode::Create, yPos, false);

			if (treeElementTriangle->GetOpen())
				RenderChildTree(it);
		}
		++it;
	}
}

void CKeywordWnd::RenderChildTree(tree<CTreeData*>::sibling_iterator& parent)
{
	tree<CTreeData*>::sibling_iterator it = tr.begin(parent);
	const bool isVisible = true;
	const auto nbChildren = parent.number_of_children();

	for (unsigned int i = 0; i < nbChildren; i++)
	{
		auto* data = static_cast<CTreeDataCategory*>(*it);
		if (tr.depth(it) == 1 && (data->GetValue().size() > 0 || it.number_of_children() == 0))
		{
			int xPos = modernMargin;

			if (data->GetNumCategorie() == ADD_KEYWORD_ROW)
			{
				//Ligne "+" : pas de case a cocher ni de bouton de suppression
				CPositionElement* posElement = RenderText(data, xPos, yPos, isVisible, RenderMode::Create, true);
				ModernColorText(posElement, modernLabel);
			}
			else
			{
				//Case a cocher a gauche, puis le libelle
				const bool check = GetCheckState(data->GetExifKey(), data->GetKey());
				CPositionElement* posElement = RenderCheckBox(data, xPos, yPos, check, isVisible,
					RenderMode::Create);

				xPos = modernMargin + posElement->GetWidth() + modernMargin;
				posElement = RenderText(data, xPos, yPos, isVisible, RenderMode::Create, true);
				ModernColorText(posElement, modernValue);

				//Bouton de suppression aligne a droite (comme une valeur)
				xPos = 0;
				posElement = RenderDelete(data, xPos, yPos, isVisible);
				ModernPlaceValue(posElement, yPos);
			}

			yPos += modernRowHeight;
			nbRow++;
		}
		++it;
	}
}

//------------------------------------------------------------------------------
// Pose un champ de saisie sur le libellé du critère
//------------------------------------------------------------------------------
void CKeywordWnd::BeginEditKeyword(CPositionElement* element, wxWindow* window, const int scrollX, const int scrollY)
{
	CancelEdit();

	if (element == nullptr || window == nullptr)
		return;

	auto* treeData = static_cast<CTreeDataCategory*>(element->GetTreeData());

	const wxSize clientSize = window->GetClientSize();
	const int editX = element->GetX() - scrollX;
	const int editY = element->GetY() - scrollY;
	int editWidth = std::max(element->GetWidth(), 150);
	editWidth = std::max(std::min(editWidth, clientSize.x - editX - themeTree.GetMargeX()), 50);

	auto* ctrl = new wxTextCtrl(window, wxID_ANY, treeData->GetKey(),
		wxPoint(editX, editY), wxSize(editWidth, modernRowHeight),
		wxTE_PROCESS_ENTER);

	editedCategorie = treeData->GetNumCategorie();
	editedKey = treeData->GetKey();
	editCtrl = ctrl;

	ctrl->Bind(wxEVT_TEXT_ENTER, &CKeywordWnd::OnEditEnter, this);
	ctrl->Bind(wxEVT_KILL_FOCUS, &CKeywordWnd::OnEditKillFocus, this);
	ctrl->Bind(wxEVT_KEY_DOWN, &CKeywordWnd::OnEditKeyDown, this);

	ctrl->SelectAll();
	ctrl->SetFocus();
}

void CKeywordWnd::OnEditEnter(wxCommandEvent& event)
{
	CommitEdit();
}

void CKeywordWnd::OnEditKillFocus(wxFocusEvent& event)
{
	event.Skip();
	CommitEdit();
}

void CKeywordWnd::OnEditKeyDown(wxKeyEvent& event)
{
	if (event.GetKeyCode() == WXK_ESCAPE)
		CancelEdit();
	else
		event.Skip();
}

//------------------------------------------------------------------------------
// Valide la saisie : mise à jour en base uniquement si le libellé est non vide et modifié
//------------------------------------------------------------------------------
void CKeywordWnd::CommitEdit()
{
	wxTextCtrl* ctrl = editCtrl.get();
	if (ctrl == nullptr)
		return;

	wxWindow* parent = ctrl->GetParent();
	wxString value = ctrl->GetValue();
	value.Trim(true).Trim(false);

	const int numCategorie = editedCategorie;
	const bool changed = !value.empty() && value != editedKey;

	CloseEditor();

	if (changed)
	{
		CSqlCriteria sqlCriteria;
		sqlCriteria.UpdateCriteria(1, numCategorie, value);
		NotifyCriteriaChange(parent);
	}

	eventControl->UpdateTreeControl();
}

//------------------------------------------------------------------------------
// Abandonne la saisie sans toucher à la base
//------------------------------------------------------------------------------
void CKeywordWnd::CancelEdit()
{
	CloseEditor();
}

void CKeywordWnd::CloseEditor()
{
	wxTextCtrl* ctrl = editCtrl.get();
	if (ctrl == nullptr)
		return;

	editCtrl.Release();
	ctrl->Unbind(wxEVT_TEXT_ENTER, &CKeywordWnd::OnEditEnter, this);
	ctrl->Unbind(wxEVT_KILL_FOCUS, &CKeywordWnd::OnEditKillFocus, this);
	ctrl->Unbind(wxEVT_KEY_DOWN, &CKeywordWnd::OnEditKeyDown, this);
	ctrl->Hide();
	ctrl->Destroy();
}