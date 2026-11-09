#include <header.h>
#include "CriteriaTree.h"
#include <TreeElementTexte.h>
#include <TreeElementTriangle.h>
#include <TreeElementStar.h>
#include "TreeElementTexteClick.h"
#include <SqlPhotos.h>
#include "TreeDataLink.h"
#include "TreeDataStars.h"
#include <PhotoCriteria.h>
#include <wx/filename.h>
#include <libPicture.h>
#include <SqlFindFacePhoto.h>
#include <LibResource.h>
#include <appcontext.h>
#include <SqlPhotoCategorie.h>
#include <wx/tokenzr.h>
#if defined(__WXMSW__)
#include "../include/window_id.h"
#else
#include <window_id.h>
#endif
#include "TreeElementControlInterface.h"
using namespace Regards::Control;
using namespace Regards::Window;
using namespace Regards::Sqlite;
using namespace Regards::Picture;

#define TAILLEMAX 4096
extern AppContext application_context;
PhotoCategorieVector CCriteriaTree::photoCategorieVector;

CCriteriaTree::CCriteriaTree(CThemeTree* theme, CTreeElementControlInterface* interfaceControl) : yPos(0)
{
	rotation = 0;
	numPhotoId = 0;
	filename = L"";
	widthPosition = 0;
	rowWidth.push_back(0);
	rowWidth.push_back(0);
	themeTree = *theme;
	themeTree.themeTriangle.SetHeight(themeTree.GetRowHeight());
	themeTree.themeCheckbox.SetHeight(themeTree.GetRowHeight());
	themeTree.themeDelete.SetHeight(themeTree.GetRowHeight());
	themeTree.themeSlide.SetHeight(themeTree.GetRowHeight());
	themeTree.themeTexte.SetHeight(themeTree.GetRowHeight());
	eventControl = interfaceControl;

	EnableModernStyle();
}

wxString CCriteriaTree::GetFilename()
{
	return filename;
}

void CCriteriaTree::UpdateScreenRatio()
{
	RenderElement(RenderMode::Update);
}

void CCriteriaTree::LoadCategories()
{
	if (photoCategorieVector.size() == 0)
	{
		CSqlPhotoCategorie sqlPhotoCategorie;
		sqlPhotoCategorie.LoadPhotoCategorie(&photoCategorieVector, 1);
	}
}

void CCriteriaTree::SetFile(const wxString& picture, const int& numPhotoId)
{
	if (filename == picture)
		return;

	filename = picture;
	this->numPhotoId = numPhotoId;

	LoadCategories();

	top = tr.begin();
	child = top;

	auto treeDataPicture = new CTreeData();
	treeDataPicture->SetKey("Criteria");
	child = tr.insert(top, treeDataPicture);

	for (CPhotoCategorie& photoCategorie : photoCategorieVector)
	{
		ProcessCategory(picture, photoCategorie, child);
	}

	RenderElement(RenderMode::Create);
}

void CCriteriaTree::ProcessCategory(const wxString& picture, CPhotoCategorie& photoCategorie,
	tree<CTreeData*>::iterator& child)
{
	switch (photoCategorie.GetId())
	{
	case 4:
		AddFaces(picture, photoCategorie, child);
		break;
	case 2:
		AddFolder(picture, photoCategorie, child);
		break;
	case 5:
		CreateTextNode(child, "usenet.category", photoCategorie.GetLibelle());
		break;
	case 1:
		AddGPS(picture, photoCategorie, child);
		break;
	case 3:
		AddDate(picture, photoCategorie, child);
		break;
	case 6:
		AddStars(picture, photoCategorie, child);
		break;
	default:
		AddKeywords(picture, photoCategorie, child);
		break;
	}
}

void CCriteriaTree::AddFaces(const wxString& picture, CPhotoCategorie& photoCategorie,
	tree<CTreeData*>::iterator& child)
{
	CSqlFindFacePhoto sqlFindFacePhoto;
	std::vector<CFaceName> listFaceName = sqlFindFacePhoto.GetListFaceName(picture);
	wxString libelleCategorie = photoCategorie.GetLibelle();

	for (size_t i = 0; i < listFaceName.size(); i++)
	{
		CreateLinkNode(child, libelleCategorie, listFaceName[i].faceName, 0, listFaceName[i].numFace);
	}
}

void CCriteriaTree::AddFolder(const wxString& picture, CPhotoCategorie& photoCategorie,
	tree<CTreeData*>::iterator& child)
{
	wxFileName dirname = wxFileName::DirName(picture);
	CreateTextNode(child, photoCategorie.GetLibelle(), dirname.GetPath());
}

void CCriteriaTree::AddGPS(const wxString& picture, CPhotoCategorie& photoCategorie,
	tree<CTreeData*>::iterator& child)
{
	CriteriaVector criteriaVector;
	CSqlPhotos sqlPhotos;
	sqlPhotos.GetPhotoCriteriaByCategorie(&criteriaVector, picture, photoCategorie.GetId());

	wxString libelleCategorie = photoCategorie.GetLibelle();

	if (criteriaVector.size() == 0)
	{
		wxString value = CLibResource::LoadStringFromResource("LBLNOTGEO", 1);
		CreateLinkNode(child, libelleCategorie, value, 1, 0);
	}
	else
	{
		CCriteria criteria = criteriaVector.at(0);
		wxString value = criteria.GetLibelle();

		if (value.find(application_context.special_key) == 0)
			value = CLibResource::LoadStringFromResource("LBLNOTGEO", 1);

		CreateLinkNode(child, libelleCategorie, value, 1, criteria.GetId());
	}
}

void CCriteriaTree::AddDate(const wxString& picture, CPhotoCategorie& photoCategorie,
	tree<CTreeData*>::iterator& child)
{
	CriteriaVector criteriaVector;
	CSqlPhotos sqlPhotos;
	sqlPhotos.GetPhotoCriteriaByCategorie(&criteriaVector, picture, photoCategorie.GetId());

	wxString libelleCategorie = photoCategorie.GetLibelle();

	if (criteriaVector.size() == 0)
	{
		CreateLinkNode(child, libelleCategorie, "1970.01.01", 3, 0);
	}
	else
	{
		CCriteria& criteria = criteriaVector.at(0);
		CreateLinkNode(child, libelleCategorie, criteria.GetLibelle(), 3, criteria.GetId());
	}
}

void CCriteriaTree::AddStars(const wxString& picture, CPhotoCategorie& photoCategorie,
	tree<CTreeData*>::iterator& child)
{
	CriteriaVector criteriaVector;
	CSqlPhotos sqlPhotos;
	sqlPhotos.GetPhotoCriteriaByCategorie(&criteriaVector, picture, photoCategorie.GetId());

	wxString value = (criteriaVector.size() == 0) ? wxString("0") : criteriaVector[0].GetLibelle();
	CreateStarNode(child, photoCategorie.GetLibelle(), value);
}

void CCriteriaTree::AddKeywords(const wxString& picture, CPhotoCategorie& photoCategorie,
	tree<CTreeData*>::iterator& child)
{
	if (photoCategorie.GetId() == 7)
		return;

	CriteriaVector criteriaVector;
	CSqlPhotos sqlPhotos;
	sqlPhotos.GetPhotoCriteriaByCategorie(&criteriaVector, picture, photoCategorie.GetId());

	wxString libelleCategorie = photoCategorie.GetLibelle();

	if (criteriaVector.size() == 0)
	{
		CreateLinkNode(child, libelleCategorie, "", 0, 0);
	}
	else
	{
		for (CCriteria& criteria : criteriaVector)
		{
			CreateLinkNode(child, libelleCategorie, criteria.GetLibelle(), 0, criteria.GetId());
		}
	}
}

void CCriteriaTree::CreateLinkNode(tree<CTreeData*>::iterator& parent, const wxString& key, const wxString& value,
	const int& linkType, const int& id)
{
	auto treeDataFileName = new CTreeDataLink();
	treeDataFileName->SetIsParent(false);
	treeDataFileName->SetKey(key);
	treeDataFileName->SetValue(value);
	treeDataFileName->SetType(2);
	treeDataFileName->SetLinkType(linkType);
	treeDataFileName->SetId(id);
	treeDataFileName->SetLinkPath("");
	tr.append_child(parent, treeDataFileName);
}

void CCriteriaTree::CreateStarNode(tree<CTreeData*>::iterator& parent, const wxString& key, const wxString& value)
{
	auto treeDataFileName = new CTreeDataStars();
	treeDataFileName->SetNumPhotoId(numPhotoId);
	treeDataFileName->SetIsParent(false);
	treeDataFileName->SetKey(key);
	treeDataFileName->SetValue(value);
	treeDataFileName->SetType(6);
	tr.append_child(parent, treeDataFileName);
}

void CCriteriaTree::CreateTextNode(tree<CTreeData*>::iterator& parent, const wxString& key, const wxString& value)
{
	auto treeDataFileName = new CTreeData();
	treeDataFileName->SetIsParent(false);
	treeDataFileName->SetKey(key);
	treeDataFileName->SetValue(value);
	tr.append_child(parent, treeDataFileName);
}

void CCriteriaTree::CreateElement()
{
	RenderElement(RenderMode::Create);
}

void CCriteriaTree::OnModernRelayout()
{
	RenderElement(RenderMode::Update);
}

void CCriteriaTree::RenderElement(RenderMode mode)
{
	widthPosition = 0;

	if (mode == RenderMode::Create)
	{
		for (CPositionElement* posElement : vectorPosElement)
			delete posElement;

		vectorPosElement.clear();
		vectorPosElementDynamic.clear();
	}
	else if (mode == RenderMode::Update)
	{
		for (CPositionElement* value : vectorPosElement)
		{
			if (value != nullptr)
			{
				value->SetX(0);
				value->SetY(0);
				CTreeElement* treeElement = value->GetTreeElement();
				if (treeElement != nullptr)
				{
					treeElement->SetVisible(false);
					treeElement->SetElementPos(0, 0);
				}
			}
		}
	}

	ModernUpdateMetrics(true);

	tree<CTreeData*>::sibling_iterator it = tr.begin();
	auto itend = tr.end();
	yPos = 0;
	nbRow = 0;

	while (it != itend)
	{
		CTreeData* data = *it;
		int profondeur = tr.depth(it);
		if (profondeur == 0)
		{
			CTreeElementTriangle* treeElementTriangle = ModernRenderHeaderRow(data, mode, yPos, false);

			if (treeElementTriangle->GetOpen())
				RenderNode(it, mode);
		}
		++it;
	}
}

CPositionElement* CCriteriaTree::RenderTextLinkValue(
	CTreeData* data,
	int& xPos,
	int& yPos,
	bool visible,
	RenderMode mode)
{
	CPositionElement* posElement = nullptr;

	if (mode == RenderMode::Update)
		posElement = GetElement(data, ELEMENT_TEXTEVALUE);

	if (posElement == nullptr)
	{
		auto dataLink = static_cast<CTreeDataLink*>(data);
		CTreeElementTexte* treeElementTexte = CreateTexteLinkElement(
			themeTree.GetRowWidth(), themeTree.GetRowHeight(),
			data->GetValue(), dataLink->GetLinkPath(),
			dataLink->GetLinkType());
		treeElementTexte->SetVisible(visible);
		posElement = CreatePositionElement(
			xPos, yPos, nbRow, 1, treeElementTexte->GetWidth(),
			themeTree.GetRowHeight(), ELEMENT_TEXTEVALUE,
			treeElementTexte, data, (data->GetType() == 2));
	}
	else
	{
		auto* treeElementTexte = static_cast<CTreeElementTexte*>(posElement->GetTreeElement());
		treeElementTexte->SetVisible(visible);
		treeElementTexte->SetElementPos(xPos, yPos);
		posElement->SetX(xPos);
		posElement->SetY(yPos);
	}

	return posElement;
}

void CCriteriaTree::RenderNode(tree<CTreeData*>::sibling_iterator & parent, RenderMode mode)
{
	CPositionElement* pos_element;
	tree<CTreeData*>::sibling_iterator it = tr.begin(parent);
	bool isVisible = true;

	for (auto i = 0; i < parent.number_of_children(); i++)
	{
		CTreeData* data = *it;

		if (data->GetValue().size() > 0 || it.number_of_children() == 0)
		{
			int xPos = 0;
			if (data->GetType() == 2)
				pos_element = RenderTextLink(data, xPos, yPos, isVisible, mode);
			else
				pos_element = RenderText(data, xPos, yPos, isVisible, mode);

			ModernStyleText(pos_element, modernLabelWidth, modernMargin, modernMargin / 2, modernLabel, false, RENDERFONT_LEFT);

			xPos = 0;
			if (data->GetValue() != "")
			{
				if (data->GetType() == 6)
				{
					pos_element = RenderStar(data, xPos, yPos, isVisible, mode);
				}
				else if (data->GetType() == 2)
				{
					pos_element = RenderTextLinkValue(data, xPos, yPos, isVisible, mode);
				}
				else
				{
					pos_element = RenderTextValue(data, xPos, yPos, isVisible, mode);
					ModernColorText(pos_element, modernValue);
				}
			}
			else
			{
				pos_element = RenderTextValue(data, xPos, yPos, isVisible, mode);
				static_cast<CTreeElementTexte*>(pos_element->GetTreeElement())->SetLibelle(L"-");
				ModernColorText(pos_element, modernEmpty);
			}
			ModernPlaceValue(pos_element, yPos);

			if (data->GetType() == 6 && data->GetValue() != "")
			{
				const int starHeight = pos_element->GetTreeElement()->GetHeight();
				pos_element->SetY(yPos - starHeight / 2);
			}

			yPos += modernRowHeight;
			nbRow++;
		}
		else
		{
			CTreeElementTriangle* treeElementTriangle = ModernRenderHeaderRow(data, mode, yPos, false);
			if (treeElementTriangle->GetOpen())
				RenderNode(it, mode);
		}
		++it;
	}
}

void CCriteriaTree::MouseOver(wxDC * deviceContext, CPositionElement * element, const int& x, const int& y,
	const int& posLargeur, const int& posHauteur, bool& update)
{
	if (element != nullptr)
	{
		CTreeElement* treeElement = element->GetTreeElement();
		if (treeElement != nullptr)
			treeElement->MouseOver(deviceContext, x, y, update);
	}
}

void CCriteriaTree::ClickOnElement(CPositionElement * element, wxWindow * window,
	const int& x, const int& y,
	const int& posLargeur, const int& posHauteur)
{
	if (window == nullptr || element == nullptr)
		return;

	wxWindow* criteriaTreeWindow = window->FindWindowById(CRITERIATREEWINDOWID);
	CTreeElement* treeElement = element->GetTreeElement();
	if (element->GetType() == ELEMENT_TRIANGLE)
	{
		auto treeElementTriangle = static_cast<CTreeElementTriangle*>(treeElement);
		treeElementTriangle->ClickElement(window, (x + posLargeur) - element->GetX(),
			(y + posHauteur) - element->GetY());
		RenderElement(RenderMode::Update);
		eventControl->UpdateTreeControl();
		return;
	}
	else if (element->GetType() == ELEMENT_STAR)
	{
		auto treeElementStar = static_cast<CTreeElementStar*>(treeElement);
		treeElementStar->ClickElement(window, x, y);
		window->Refresh();

		wxWindow* criteriawindow = window->FindWindowById(CRITERIAFOLDERWINDOWID);

		if (criteriawindow)
		{
			wxCommandEvent evt(wxEVENT_UPDATECRITERIA);
			evt.SetExtraLong(2);
			criteriawindow->GetEventHandler()->AddPendingEvent(evt);
		}
	}
	else if (element->GetType() == ELEMENT_TEXTEVALUE)
	{
		auto treeElementTexte = static_cast<CTreeElementTexteClick*>(treeElement);
		if (treeElementTexte->GetTypeLink() == 3)
		{
			CLibPicture libpicture;
			if (libpicture.TestIsExifCompatible(filename))
			{
				auto treedata = static_cast<CTreeDataLink*>(element->GetTreeData());
				auto photoCriteria = new CPhotoCriteria();
				photoCriteria->SetCriteriaId(treedata->GetId());
				photoCriteria->SetPhotoId(numPhotoId);
				auto event = new wxCommandEvent(wxEVT_SHOWCALENDAR);
				event->SetClientData(photoCriteria);
				wxQueueEvent(criteriaTreeWindow, event);
			}
			else
			{
				wxString labelInformations = CLibResource::LoadStringFromResource(L"labelInformations", 1);
				wxString notCompatibleFormat = CLibResource::LoadStringFromResource(L"NotCompatibleFormat", 1);
				wxMessageBox(notCompatibleFormat, labelInformations, wxICON_INFORMATION);
			}
		}
		else if (treeElementTexte->GetTypeLink() == 1)
		{
			CLibPicture libpicture;
			if (libpicture.TestIsExifCompatible(filename))
			{
				auto treedata = static_cast<CTreeDataLink*>(element->GetTreeData());
				auto photoCriteria = new CPhotoCriteria();
				photoCriteria->SetCriteriaId(treedata->GetId());
				photoCriteria->SetPhotoId(numPhotoId);
				auto event = new wxCommandEvent(wxEVT_SHOWMAP);
				event->SetClientData(photoCriteria);
				wxQueueEvent(criteriaTreeWindow, event);
			}
			else
			{
				wxString labelInformations = CLibResource::LoadStringFromResource(L"labelInformations", 1);
				wxString notCompatibleFormat = CLibResource::LoadStringFromResource(L"NotCompatibleFormat", 1);
				wxMessageBox(notCompatibleFormat, labelInformations, wxICON_INFORMATION);
			}
		}
		else if (treeElementTexte->GetTypeLink() == 7)
		{
			auto treedata = static_cast<CTreeDataLink*>(element->GetTreeData());
			auto photoCriteria = new CPhotoCriteria();
			photoCriteria->SetCriteriaId(treedata->GetId());
			photoCriteria->SetPhotoId(numPhotoId);
			auto event = new wxCommandEvent(wxEVT_SHOWKEYWORD);
			event->SetClientData(photoCriteria);
			wxQueueEvent(criteriaTreeWindow, event);
		}
	}
}

void CCriteriaTree::AddTreeInfos(const wxString & exifKey,
	const wxString & exifValue,
	const int& index,
	tree<CTreeData*>::iterator & top,
	tree<CTreeData*>::iterator & child)
{
	AddTreeInfosImpl<CTreeData>(
		exifKey,
		index,
		top,
		child,
		[&](CTreeData* treeData)
		{
			treeData->SetIsParent(false);
			treeData->SetValue(exifValue);
			treeData->SetExifKey(exifKey);
		});
}
