#include <header.h>
#include "InfosFile.h"
#include <libPicture.h>
#include <TreeData.h>
#ifdef EXIV2
#include <MetadataExiv2.h>
#else
#ifdef __APPLE__
#include <AppleReadExif.h>
#include <libPicture.h>
#include <MediaInfo.h>
#endif
#ifdef WIN32
#include <PictureMetadata.h>
#include <MetadataExiv2.h>
#include <libPicture.h>
#include <MediaInfo.h>
#endif
#endif
#include <TreeElementTexte.h>
#include <TreeElementTriangle.h>
#include <theme.h>
#include <TreeElementControlInterface.h>
#include <wx/tokenzr.h>
#include <algorithm>
using namespace Regards::Control;
using namespace Regards::Window;
using namespace Regards::Picture;
using namespace Regards::exiv2;
#define TAILLEMAX 4096

namespace
{
	// Parametres du style "moderne" (modifiables ici)
	constexpr int LABEL_COLUMN_PERCENT = 45; // part de la largeur reservee aux libelles
	constexpr int MIN_PANEL_WIDTH = 280;     // largeur minimale si SetPanelWidth n'a pas ete appele
	constexpr int INDENT_PER_LEVEL = 0;      // 0 = pas d'indentation ; mettre ex. 10 pour la retrouver
}

CInfosFile::CInfosFile(CThemeTree* theme, CTreeElementControlInterface* interfaceControl) : yPos(0), cxMax(0),
xMaxPos(0),
xMaxPosValue(0)
{
	rotation = 0;
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
}
;


const wxString CInfosFile::GetFilename()
{
	return filename;
}

void CInfosFile::MouseOver(wxDC* deviceContext, CPositionElement* element, const int& x, const int& y,
	const int& posLargeur, const int& posHauteur, bool& update)
{
	//

	if (element != nullptr)
	{
		CTreeElement* treeElement = element->GetTreeElement();
		if (treeElement != nullptr)
			treeElement->MouseOver(deviceContext, x, y, update);
	}
}


void CInfosFile::UpdateScreenRatio()
{
	//
	this->RenderElement(RenderMode::Update);
}

void CInfosFile::AddTreeInfos(const wxString& exifKey,
	const wxString& exifValue,
	const int& index,
	tree<CTreeData*>::iterator& top,
	tree<CTreeData*>::iterator& child)
{
	AddTreeInfosImpl<CTreeData>(
		exifKey,
		index,
		top,
		child,
		[&](CTreeData* treeData)
		{
			treeData->SetValue(exifValue);
			treeData->SetExifKey(exifKey);
		});
}

void CInfosFile::SetFile(const wxString& picture)
{
	//

	if (filename == picture)
		return;

	std::vector<CMetadata> listItem;
	CLibPicture libPicture;
	filename = picture;
	int width = 0;
	int height = 0;

	bool showOtherInformation = true;
	cxMax = 0;
	xMaxPos = 0;
	xMaxPosValue = 0;

#ifdef EXIV2
	CMetadataExiv2 metadata(picture);
	listItem = metadata.GetMetadata();
#else

#ifdef __APPLE__
	CAppleReadExif appleReadExif;
#elif WIN32
	WIC::CPictureMetadata metadata(picture);
#endif

	if (libPicture.TestIsVideo(picture))
	{
		CMediaInfo mediaInfo;
		listItem = mediaInfo.ReadMetadata(picture);
	}
	else if (libPicture.TestImageFormat(picture))
	{
#ifdef __APPLE__
		listItem = appleReadExif.GetPictureMetadata(picture);
#elif WIN32
		listItem = metadata.GetMetadataFromPicture();
		if (listItem.size() == 0)
		{
			EXIV2::CMetadataExiv2 meta(picture);
			listItem = meta.GetMetadata();
		}
#endif
	}
#endif

	if (listItem.size() > 0)
	{
		showOtherInformation = false;
		tree<CTreeData*>::iterator top;

		int index = 0;

		top = tr.begin();


		for (const CMetadata& meta : listItem)
		{
			auto child = top;
			AddTreeInfos(meta.key, meta.value, index, top, child);
			index++;
		}
	}

#ifdef WIN32
	wxString createDatatime = metadata.GetCreationDate();
	if (createDatatime == "")
		showOtherInformation = true;

#endif
	if (showOtherInformation)
	{
		CLibPicture lib_picture;
		lib_picture.GetPictureDimensions(picture, width, height, rotation);
		tree<CTreeData*>::iterator top;
		wxString exifinfos;
		//int item = 0;
		//int index = 0;

		top = tr.begin();

		auto child = top;

		auto treeDataPicture = new CTreeData();
		treeDataPicture->SetKey(L"Picture");
		child = tr.insert(top, treeDataPicture);

		auto treeDataFileName = new CTreeData();
		treeDataFileName->SetIsParent(false);
		treeDataFileName->SetKey(L"Filename");
		treeDataFileName->SetValue(picture);
		tr.append_child(child, treeDataFileName);

		auto treeDataWidth = new CTreeData();
		exifinfos = to_string(width);
		treeDataWidth->SetIsParent(false);
		treeDataWidth->SetKey(L"Width");
		treeDataWidth->SetValue(exifinfos);
		tr.append_child(child, treeDataWidth);

		auto treeDataHeight = new CTreeData();
		exifinfos = to_string(height);
		treeDataHeight->SetIsParent(false);
		treeDataHeight->SetKey(L"Height");
		treeDataHeight->SetValue(exifinfos);
		tr.append_child(child, treeDataHeight);

		wxStructStat strucStat;
		wxStat(picture, &strucStat);
		//wxFileOffset filelen=strucStat.st_size;
		wxDateTime last_modified_time(strucStat.st_mtime);
		wxString dateTimeInfos = last_modified_time.FormatISOCombined(' ');

		auto treeDataDateTime = new CTreeData();
		treeDataDateTime->SetIsParent(false);
		treeDataDateTime->SetKey(L"Date Time");
		treeDataDateTime->SetValue(dateTimeInfos);
		tr.append_child(child, treeDataDateTime);
	}

}

void CInfosFile::CreateElement()
{
	RenderElement(RenderMode::Create);
}


int CInfosFile::ToRawX(const int& pixels)
{
	// Les elements appliquent le ratio d'ecran dans GetWidth() : on le retire ici pour obtenir la largeur voulue
	const float ratio = themeTree.CalculRatioX();
	return ratio > 0.0f ? static_cast<int>(pixels / ratio + 0.5f) : pixels;
}

int CInfosFile::ToRawY(const int& pixels)
{
	const float ratio = themeTree.CalculRatioY();
	return ratio > 0.0f ? static_cast<int>(pixels / ratio + 0.5f) : pixels;
}

wxColour CInfosFile::GetBackgroundColour(const int& yPos)
{
	// Fond uni : plus de bandes alternees
	return themeTree.bgColorOne;
}

void CInfosFile::BeforeDrawBitmap(wxDC* deviceContext, const int& width, const int& height,
	const int& posLargeur, const int& posHauteur)
{
	// La largeur reelle de la zone dessinee devient la largeur du panneau
	if (width > 0 && width != panelWidth)
		SetPanelWidth(width);

	// Fond uni sur toute la zone, y compris sous la derniere ligne
	if (deviceContext != nullptr)
	{
		deviceContext->SetPen(*wxTRANSPARENT_PEN);
		deviceContext->SetBrush(wxBrush(themeTree.bgColorOne));
		deviceContext->DrawRectangle(0, 0, width, height);
		deviceContext->SetBrush(wxNullBrush);
		deviceContext->SetPen(wxNullPen);
	}
}

void CInfosFile::SetPanelWidth(const int& width)
{
	if (panelWidth == width)
		return;

	panelWidth = width;

	// Si l'arbre est deja construit, on recalcule les positions
	if (!tr.empty())
		RenderElement(RenderMode::Update);
}

void CInfosFile::UpdateLayoutMetrics()
{
	margin = themeTree.GetMargeX();
	rowHeight = themeTree.GetRowHeight();

	const int minWidth = themeTree.CalculElementSizeForScreen(MIN_PANEL_WIDTH);
	layoutWidth = std::max(panelWidth, minWidth);
	labelColumnWidth = layoutWidth * LABEL_COLUMN_PERCENT / 100;
	valueColumnWidth = layoutWidth - labelColumnWidth;
	chevronWidth = 2 * margin;

	// Colonne 0 = libelles, colonne 1 = valeurs (la colonne 1 demarre a rowWidth[0])
	rowWidth[0] = labelColumnWidth;
	rowWidth[1] = valueColumnWidth;

	// Palette derivee du theme existant (aucune couleur en dur)
	const wxColour font = themeTree.themeTexte.font.GetColorFont();
	colorRowBack = themeTree.bgColorOne;
	colorRowBorder = themeTree.bgColorTwo;//ChangeLightness(125);
	colorTitle = font;
	colorValue = font;

	auto blend = [&](const int& percentFont)
		{
			return wxColour((font.Red() * percentFont + colorRowBack.Red() * (100 - percentFont)) / 100,
				(font.Green() * percentFont + colorRowBack.Green() * (100 - percentFont)) / 100,
				(font.Blue() * percentFont + colorRowBack.Blue() * (100 - percentFont)) / 100);
		};
	colorLabel = blend(60);      // libelle attenue
	colorEmptyValue = blend(40); // "-" encore plus discret
}

void CInfosFile::StyleTextCell(CPositionElement* posElement, const int& zoneWidth, const int& insetLeft,
	const int& insetRight, const wxColour& textColor, const bool& bold,
	const int& alignment)
{
	auto* text = static_cast<CTreeElementTexte*>(posElement->GetTreeElement());
	text->SetZoneSize(ToRawX(zoneWidth), ToRawY(rowHeight));
	text->SetBold(bold);
	text->SetTextColor(textColor);
	text->SetInsets(insetLeft, insetRight);
	text->SetEllipsize(true);
	text->SetPosition(alignment);
	text->SetRowStyle(colorRowBack, colorRowBorder);
}

// Ligne de titre de section : TITRE en gras, chevron a droite, clic sur toute la ligne
CTreeElementTriangle* CInfosFile::RenderHeaderRow(CTreeData* data, const int& profondeur, RenderMode mode)
{
	bool isVisible = true;

	// Chevron (a droite)
	int xPos = layoutWidth - chevronWidth;
	CPositionElement* posTriangle = RenderTriangle(data, xPos, yPos, isVisible, mode);
	auto* triangle = static_cast<CTreeElementTriangle*>(posTriangle->GetTreeElement());
	triangle->SetChevronStyle(true);
	triangle->SetRowStyle(colorRowBack, colorRowBorder);
	triangle->SetZoneSize(ToRawX(chevronWidth), ToRawY(rowHeight));
	posTriangle->SetValue(xPos, yPos, nbRow, 0, chevronWidth, rowHeight, ELEMENT_TRIANGLE, triangle, data);

	// Titre (a gauche, en majuscules)
	const int titleWidth = layoutWidth - chevronWidth;
	xPos = 0;
	CPositionElement* posText = RenderText(data, xPos, yPos, isVisible, mode, true);
	auto* text = static_cast<CTreeElementTexte*>(posText->GetTreeElement());

	const wxString title = data->GetKey().Upper();
	if (text->GetLibelle() != title)
		text->SetLibelle(title);

	const int indent = themeTree.CalculElementSizeForScreen(INDENT_PER_LEVEL) * profondeur;
	StyleTextCell(posText, titleWidth, margin + indent, 0, colorTitle, true, RENDERFONT_LEFT);
	posText->SetValue(0, yPos, nbRow, 0, titleWidth, rowHeight, ELEMENT_TEXTE, text, data);

	yPos += rowHeight;
	nbRow++;

	return triangle;
}

void CInfosFile::RenderChildTree(tree<CTreeData*>::sibling_iterator& parent, RenderMode mode)
{
	CPositionElement* posElement = nullptr;
	tree<CTreeData*>::sibling_iterator it = tree<CTreeData*>::begin(parent);

	for (auto i = 0; i < parent.number_of_children(); i++)
	{
		bool isVisible = true;
		const int profondeur = tr.depth(it);
		CTreeData* data = *it;

		if (!data->GetValue().empty() || it.number_of_children() == 0)
		{
			const int indent = themeTree.CalculElementSizeForScreen(INDENT_PER_LEVEL) * profondeur;

			// Libelle (gris attenue, a gauche)
			int xPos = 0;
			posElement = RenderText(data, xPos, yPos, isVisible, mode);
			StyleTextCell(posElement, labelColumnWidth, margin + indent, margin / 2, colorLabel, false,
				RENDERFONT_LEFT);

			// Valeur (blanche, alignee a droite) ; "-" discret si vide
			xPos = 0;
			posElement = RenderTextValue(data, xPos, yPos, isVisible, mode);

			const bool emptyValue = data->GetValue().empty();
			if (emptyValue)
				static_cast<CTreeElementTexte*>(posElement->GetTreeElement())->SetLibelle(L"-");

			StyleTextCell(posElement, valueColumnWidth, margin / 2, margin,
				emptyValue ? colorEmptyValue : colorValue, false, RENDERFONT_RIGHT);

			if (isVisible)
			{
				yPos += rowHeight;
				nbRow++;
			}
		}
		else
		{
			CTreeElementTriangle* tree_element_triangle = RenderHeaderRow(data, profondeur, mode);

			if (mode == RenderMode::Create)
			{
				RenderChildTree(it, mode);
			}
			else
			{
				const bool isOpen = tree_element_triangle->GetOpen();
				if (isOpen)
					RenderChildTree(it, mode);
			}
		}
		++it;
	}
}

void CInfosFile::RenderElement(RenderMode mode)
{
	HideAll();
	UpdateLayoutMetrics();

	widthPosition = 0;

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
			CTreeElementTriangle* tree_element_triangle = RenderHeaderRow(data, profondeur, mode);

			if (mode == RenderMode::Create)
			{
				RenderChildTree(it, mode);
			}
			else
			{
				const bool isOpen = tree_element_triangle->GetOpen();
				if (isOpen)
					RenderChildTree(it, mode);
			}
		}
		++it;
	}
}

void CInfosFile::ClickOnElement(CPositionElement* element, wxWindow* window, const int& x, const int& y,
	const int& posLargeur, const int& posHauteur)
{
	//
	if (element != nullptr)
	{
		CTreeElement* treeElement = element->GetTreeElement();
		if (treeElement != nullptr)
		{
			CTreeElementTriangle* treeElementTriangle = nullptr;

			if (element->GetType() == ELEMENT_TRIANGLE)
			{
				treeElementTriangle = static_cast<CTreeElementTriangle*>(treeElement);
			}
			else if (element->GetType() == ELEMENT_TEXTE)
			{
				// Clic sur le titre d'une section : on bascule le chevron de la meme ligne
				CPositionElement* posTriangle = GetElement(element->GetTreeData(), ELEMENT_TRIANGLE);
				if (posTriangle != nullptr)
					treeElementTriangle = static_cast<CTreeElementTriangle*>(posTriangle->GetTreeElement());
			}

			if (treeElementTriangle != nullptr)
			{
				treeElementTriangle->ClickElement(window, x, y);
				RenderElement(RenderMode::Update);
				eventControl->UpdateTreeControl();
			}
		}
	}
}