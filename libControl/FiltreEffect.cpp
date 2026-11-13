#include <header.h>
#include "FiltreEffect.h"
#include <TreeDataEffect.h>
#include <FilterData.h>
#include <wx/tokenzr.h>
#include <algorithm>
#include "MainTheme.h"
#include "MainThemeInit.h"
#include "BitmapWndViewer.h"
#include "BitmapWnd3d.h"
#include <TreeElementTexte.h>
#include <TreeElementSlide.h>
#include <TreeElementTriangle.h>
#include <TreeElementCheckBox.h>
#include <TreeElementListBox.h>
#include <TreeElementColor.h>
#include <TreeElementComboBox.h>
#include <PositionElement.h>
#include <TreeElementControlInterface.h>
#include <ImageLoadingFormat.h>
#include <appcontext.h>
extern AppContext application_context;
#define TAILLEMAX 1024
using namespace Regards::Control;
using namespace Regards::Window;

namespace
{
	// Parametres du style "moderne" (identiques a CInfosFile)
	constexpr int LABEL_COLUMN_PERCENT = 45; // part de la largeur reservee aux libelles
	constexpr int MIN_PANEL_WIDTH = 280;     // largeur minimale si SetPanelWidth n'a pas ete appele
	constexpr int INDENT_PER_LEVEL = 0;      // 0 = pas d'indentation ; mettre ex. 10 pour la retrouver
}

CFiltreEffect::CFiltreEffect(IFiltreUpdate* bitmapViewer, CTreeElementControlInterface* interfaceControl, bool isVideo,
	int bitmapWindowId) : yPos(0), cxMax(0), cyMax(0), xMaxPos(0), xMaxPosValue(0), xMinPos(0),
	effectParameter(nullptr)
{
	this->bitmapWindowId = bitmapWindowId;
	filtre = 0;
	widthPosition = 0;
	filterEffect = nullptr;

	index = 0;
	this->isVideo = isVideo;

	CMainTheme* viewerTheme = CMainThemeInit::getInstance();
	if (viewerTheme)
		viewerTheme->GetTreeTheme(&themeTree);
	this->bitmapViewer = bitmapViewer;

	themeTree.themeTriangle.SetHeight(themeTree.GetRowHeight());
	themeTree.themeCheckbox.SetHeight(themeTree.GetRowHeight());
	themeTree.themeDelete.SetHeight(themeTree.GetRowHeight());
	themeTree.themeSlide.SetHeight(themeTree.GetRowHeight());
	themeTree.themeTexte.SetHeight(themeTree.GetRowHeight());
	eventControl = interfaceControl;
	rowWidth.push_back(0);
	rowWidth.push_back(0);
}

void CFiltreEffect::AddTreeInfos(const wxString& exifKey, CTreeElementValue* position, void* value,
	const int& typeValue, const int& type)
{
	AddTreeInfos(exifKey, position, value, typeValue, index++, top, child, type);
}

CPositionElement* CFiltreEffect::RenderColor(
	CTreeData* dataEffect,
	int& xPos,
	int& yPos,
	bool visible,
	RenderMode mode)
{
	CPositionElement* posElement = nullptr;
	auto data = static_cast<CTreeDataEffect*>(dataEffect);

	// On vérifie si l'élément de positionnement existe déjà dans le vecteur dynamique
	if (mode == RenderMode::Update)
		posElement = GetElement(data, ELEMENT_COLOR);

	if (posElement == nullptr)
	{
		// Premier affichage : extraction de la wxColour via l'objet initial Value
		wxColour initialColor(255, 0, 0); // Couleur de repli
		if (data->GetInitValue() != nullptr && data->GetInitValue()->GetType() == TYPE_ELEMENT_COLOR)
		{
			auto colorValue = static_cast<CTreeElementValueColor*>(data->GetInitValue());
			initialColor = colorValue->GetValue();
		}

		// Appel de la méthode de création héritée de CTreeControl
		CTreeElementColor* treeElementColor = CreateColorElement(
			themeTree.GetRowWidth(), themeTree.GetRowHeight(), initialColor);
		treeElementColor->SetVisible(visible);

		// Enregistrement de la position sur la colonne de droite (colonne 1)
		posElement = CreatePositionElement(xPos, yPos, nbRow, 1, themeTree.GetRowWidth(),
			themeTree.GetRowHeight(), ELEMENT_COLOR, treeElementColor, data);
	}
	else
	{
		// Mise à jour de l'interface : repositionnement XY standard
		auto* colorElement = static_cast<CTreeElementColor*>(posElement->GetTreeElement());
		colorElement->SetVisible(visible);
		colorElement->SetElementPos(xPos, yPos);

		posElement->SetX(xPos);
		posElement->SetY(yPos);
	}

	return posElement;
}

void CFiltreEffect::UpdateSourceBitmap(cv::Mat& source)
{
	this->source = source.clone();
}

void CFiltreEffect::Init(CEffectParameter* effectParameter, cv::Mat source, const wxString& filename,
	const int& filtre)
{
	CBitmapWndViewer* bitmapViewer = nullptr;
	auto bitmapWindow = dynamic_cast<IBitmapWnd*>(wxWindow::FindWindowById(bitmapWindowId));
	if (!isVideo)
	{
		if (bitmapWindow != nullptr)
			bitmapViewer = static_cast<CBitmapWndViewer*>(bitmapWindow->GetWndPt());
	}

	this->filtre = filtre;
	this->effectParameter = effectParameter;
	this->source = source;

	index = 0;
	top = tr.begin();
	child = top;

	filterEffect = CFiltreData::CreateEffectPointer(filtre);
	if (filterEffect != nullptr)
	{
		if (!source.empty())
		{
			filterEffect->Filter(effectParameter, source, filename, this);
		}
		else
			filterEffect->Filter(effectParameter, filename, this);
	}

	if (bitmapViewer != nullptr && CFiltreData::NeedPreview(filtre))
		bitmapViewer->SetListener(filterEffect.get());

	if (bitmapViewer != nullptr)
		bitmapViewer->UpdateFiltre(effectParameter);

	RenderElement(RenderMode::Create);
}

CEffectParameter* CFiltreEffect::GetEffectParameter()
{
	return effectParameter;
}

void CFiltreEffect::UpdateScreenRatio()
{
	RenderElement(RenderMode::Update);
}



void CFiltreEffect::UpdateBitmapToViewer(CImageLoadingFormat* bitmap)
{
	auto bitmapWindow = wxWindow::FindWindowById(bitmapWindowId);
	auto event = new wxCommandEvent(wxEVENT_SETBITMAP);
	event->SetClientData(bitmap);
	wxQueueEvent(bitmapWindow, event);
}

void CFiltreEffect::UpdateMousePosition()
{
	CBitmapWndViewer* bitmapViewer = nullptr;
	auto bitmapWindow = dynamic_cast<IBitmapWnd*>(wxWindow::FindWindowById(bitmapWindowId));
	if (bitmapWindow != nullptr)
	{
		bitmapViewer = static_cast<CBitmapWndViewer*>(bitmapWindow->GetWndPt());
	}

	if (bitmapViewer != nullptr)
	{
		CImageLoadingFormat* imageLoad = filterEffect->ApplyEffect(effectParameter, bitmapViewer);
		if (imageLoad != nullptr)
			UpdateBitmapToViewer(imageLoad);
		else
		{
			//Default picture if no effect is applied
			CImageLoadingFormat* imageLoad = new CImageLoadingFormat();
			cv::Mat mat = application_context.GetDefaultPictureThumbnail();
			imageLoad->SetPicture(mat);
			UpdateBitmapToViewer(imageLoad);
		}
	}
}

CImageLoadingFormat* CFiltreEffect::ApplyEffect()
{
	CBitmapWndViewer* bitmapViewer = nullptr;
	auto bitmapWindow = dynamic_cast<IBitmapWnd*>(wxWindow::FindWindowById(bitmapWindowId));
	if (bitmapWindow != nullptr)
	{
		bitmapViewer = static_cast<CBitmapWndViewer*>(bitmapWindow->GetWndPt());
	}

	if (filterEffect != nullptr)
	{
		CImageLoadingFormat* picture = filterEffect->ApplyEffect(effectParameter, bitmapViewer);
		picture->SetFilename(bitmapViewer->GetFilename());
		return picture;
	}
	return nullptr;
}

void CFiltreEffect::SlidePosChange(CTreeElement* treeElement, const int& position, CTreeElementValue* value,
	const wxString& key)
{
	if (filterEffect != nullptr)
		filterEffect->FilterChangeParam(effectParameter, value, key);

	if (bitmapViewer != nullptr)
	{
		bitmapViewer->UpdateFiltre(effectParameter);
	}

	// --- AJOUT REQUIS POUR LA COMBOBOX ET LE SLIDER ---
	// Force l'arbre entier à recalculer l'agencement et la visibilité des lignes au repos
	this->RenderElement(RenderMode::Update);

	// Demande au conteneur de fenêtre global de rafraîchir le contrôle de l'arbre
	if (eventControl != nullptr)
	{
		eventControl->UpdateTreeControl();
	}
}


void CFiltreEffect::AddTreeInfos(const wxString& exifKey,
	CTreeElementValue* position,
	void* value,
	int typeValue,
	const int& index,
	tree<CTreeData*>::iterator& top,
	tree<CTreeData*>::iterator& child,
	const int& type)
{
	AddTreeInfosImpl<CTreeDataEffect>(
		exifKey,
		index,
		top,
		child,
		[&](CTreeDataEffect* treeData)
		{
			treeData->SetInitValue(position);
			treeData->SetValue(value, typeValue);
			treeData->SetExifKey(exifKey);
			treeData->SetType(type);
		});
}

CPositionElement* CFiltreEffect::RenderSlide(
	CTreeData* dataEffect,
	int& xPos,
	int& yPos,
	bool visible,
	RenderMode mode)
{
	CPositionElement* posElement = nullptr;
	auto data = static_cast<CTreeDataEffect*>(dataEffect);

	if (mode == RenderMode::Update)
		posElement = GetElement(data, ELEMENT_SLIDE);

	if (posElement == nullptr)
	{
		CTreeElementSlide* treeElementSlide = CreateSlideElement(
			themeTree.GetRowWidth(), themeTree.GetRowHeight(),
			data->GetInitValue(), data->GetVectorValue(),
			data->GetExifKey());
		treeElementSlide->SetVisible(visible);
		posElement = CreatePositionElement(xPos, yPos, nbRow, 1, themeTree.GetRowWidth(),
			themeTree.GetRowHeight(), ELEMENT_SLIDE, treeElementSlide,
			data);
	}
	else
	{
		auto* slide = static_cast<CTreeElementSlide*>(posElement->GetTreeElement());

		slide->SetVisible(visible);
		slide->SetElementPos(xPos, yPos);

		posElement->SetX(xPos);
		posElement->SetY(yPos);
	}

	return posElement;
}

CPositionElement* CFiltreEffect::RenderCheckbox(
	CTreeData* dataEffect,
	int& xPos,
	int& yPos,
	bool visible,
	RenderMode mode)
{
	CPositionElement* posElement = nullptr;
	auto data = static_cast<CTreeDataEffect*>(dataEffect);

	if (mode == RenderMode::Update)
		posElement = GetElement(data, ELEMENT_CHECKBOX);

	if (posElement == nullptr)
	{
		CTreeElementCheckBox* treeElementCheckbox = CreateCheckBoxElement(
			themeTree.GetRowWidth(), themeTree.GetRowHeight(),
			data->GetBoolValue());
		treeElementCheckbox->SetVisible(visible);
		posElement = CreatePositionElement(xPos, yPos, nbRow, 1, themeTree.GetRowWidth(),
			themeTree.GetRowHeight(), ELEMENT_CHECKBOX, treeElementCheckbox,
			data);
	}
	else
	{
		auto* checkbox = static_cast<CTreeElementCheckBox*>(posElement->GetTreeElement());

		checkbox->SetVisible(visible);
		checkbox->SetElementPos(xPos, yPos);

		posElement->SetX(xPos);
		posElement->SetY(yPos);
	}

	return posElement;
}


CPositionElement* CFiltreEffect::RenderList(
	CTreeData* dataEffect,
	int& xPos,
	int& yPos,
	bool visible,
	RenderMode mode)
{
	CPositionElement* posElement = nullptr;
	auto data = static_cast<CTreeDataEffect*>(dataEffect);

	if (mode == RenderMode::Update)
		posElement = GetElement(data, ELEMENT_LISTBOX);

	if (posElement == nullptr)
	{
		CTreeElementListBox* treeElementListBox = CreateListBoxElement(
			themeTree.GetRowWidth(), themeTree.GetRowHeight(),
			data->GetMetadataValue(), data->GetIndex(),
			data->GetExifKey());
		treeElementListBox->SetVisible(visible);
		posElement = CreatePositionElement(xPos, yPos, nbRow, 1, themeTree.GetRowWidth(),
			themeTree.GetRowHeight(), ELEMENT_LISTBOX, treeElementListBox,
			data);
	}
	else
	{
		auto* checkbox = static_cast<CTreeElementListBox*>(posElement->GetTreeElement());

		checkbox->SetVisible(visible);
		checkbox->SetElementPos(xPos, yPos);

		posElement->SetX(xPos);
		posElement->SetY(yPos);
	}

	return posElement;
}

/**
 * Gère la création, la mise à jour et le positionnement d'une ComboBox dans l'arbre.
 */
CPositionElement* CFiltreEffect::RenderComboBox(
	CTreeData* data,
	int& xPos,
	int& yPos,
	bool visible,
	RenderMode mode)
{
	CPositionElement* posElement = nullptr;
	auto dataEffect = static_cast<CTreeDataEffect*>(data);

	// 1. Recherche d'un élément existant si nous rafraîchissons l'interface
	if (mode == RenderMode::Update)
		posElement = GetElement(data, ELEMENT_COMBOBOX);

	if (posElement == nullptr)
	{
		// 2. Extraction des données textuelles depuis CTreeData
		std::vector<wxString> items;
		int defaultSelection = 0;

		if (dataEffect != nullptr)
		{
			// Récupération du vecteur de CMetadata (comme pour la ListBox)
			const auto& metadataVector = dataEffect->GetMetadataValue();
			for (const auto& meta : metadataVector)
			{
				items.push_back(meta.value); // Extraction de la chaîne localisée
			}
			defaultSelection = dataEffect->GetIndex();
		}

		// 3. Création de l'élément graphique via la méthode héritée
		CTreeElementComboBox* treeElementComboBox = CreateComboBoxElement(
			themeTree.GetRowWidth(), themeTree.GetRowHeight(), items, dataEffect->GetExifKey(), defaultSelection);

		treeElementComboBox->SetVisible(visible);

		// 4. Enregistrement dans le gestionnaire de positions (Colonne de droite = 1)
		posElement = CreatePositionElement(xPos, yPos, nbRow, 1, themeTree.GetRowWidth(),
			themeTree.GetRowHeight(), ELEMENT_COMBOBOX, treeElementComboBox, data);
	}
	else
	{
		// 5. Mode Mise à jour : Simple réalignement des coordonnées graphiques XY
		auto* comboElement = static_cast<CTreeElementComboBox*>(posElement->GetTreeElement());
		comboElement->SetVisible(visible);
		comboElement->SetElementPos(xPos, yPos);

		posElement->SetX(xPos);
		posElement->SetY(yPos);
	}

	return posElement;
}

void CFiltreEffect::SetPanelWidth(const int& width)
{
	if (panelWidth == width)
		return;

	panelWidth = width;

	// Si l'arbre est deja construit, on recalcule les positions
	if (!tr.empty())
		RenderElement(RenderMode::Update);
}

int CFiltreEffect::ToRawX(const int& pixels)
{
	// Les elements appliquent le ratio d'ecran dans GetWidth() : on le retire ici pour obtenir la largeur voulue
	const float ratio = themeTree.CalculRatioX();
	return ratio > 0.0f ? static_cast<int>(pixels / ratio + 0.5f) : pixels;
}

int CFiltreEffect::ToRawY(const int& pixels)
{
	const float ratio = themeTree.CalculRatioY();
	return ratio > 0.0f ? static_cast<int>(pixels / ratio + 0.5f) : pixels;
}

wxColour CFiltreEffect::GetBackgroundColour(const int& yPos)
{
	// Fond uni : plus de bandes alternees
	return themeTree.bgColorOne;
}

void CFiltreEffect::BeforeDrawBitmap(wxDC* deviceContext, const int& width, const int& height,
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

void CFiltreEffect::UpdateLayoutMetrics()
{
	margin = themeTree.GetMargeX();
	rowHeight = themeTree.GetRowHeight();

	const int minWidth = themeTree.CalculElementSizeForScreen(MIN_PANEL_WIDTH);
	layoutWidth = std::max(panelWidth, minWidth);
	labelColumnWidth = layoutWidth * LABEL_COLUMN_PERCENT / 100;
	valueColumnWidth = layoutWidth - labelColumnWidth;
	chevronWidth = 2 * margin;

	// Largeur des controles (slide, liste, couleur, combo) : colonne de droite moins les marges
	controlWidth = std::max(valueColumnWidth - margin / 2 - margin, 1);

	// Colonne 0 = libelles, colonne 1 = controles (la colonne 1 demarre a rowWidth[0])
	rowWidth[0] = labelColumnWidth;
	rowWidth[1] = valueColumnWidth;

	// Palette derivee du theme existant (aucune couleur en dur)
	const wxColour font = themeTree.themeTexte.font.GetColorFont();
	colorRowBack = themeTree.bgColorOne;
	colorRowBorder = themeTree.bgColorTwo;
	colorTitle = font;

	auto blend = [&](const int& percentFont)
		{
			return wxColour((font.Red() * percentFont + colorRowBack.Red() * (100 - percentFont)) / 100,
				(font.Green() * percentFont + colorRowBack.Green() * (100 - percentFont)) / 100,
				(font.Blue() * percentFont + colorRowBack.Blue() * (100 - percentFont)) / 100);
		};
	colorLabel = blend(60); // libelle attenue
}

void CFiltreEffect::StyleTextCell(CPositionElement* posElement, const int& zoneWidth, const int& insetLeft,
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

// Controle de la colonne de droite : il occupe toute la colonne (moins les marges)
// fillColumn = false : l'element garde sa taille propre (case a cocher)
// scaledByRatio = true : l'element applique le ratio d'ecran dans GetWidth() (comme CTreeElementTexte)
void CFiltreEffect::StyleControlCell(CPositionElement* posElement, const int& xPos, const bool& fillColumn,
	const bool& scaledByRatio)
{
	if (posElement == nullptr || !fillColumn)
		return;

	CTreeElement* element = posElement->GetTreeElement();
	if (element == nullptr)
		return;

	if (scaledByRatio)
		element->SetZoneSize(ToRawX(controlWidth), ToRawY(rowHeight));
	else
		element->SetZoneSize(controlWidth, rowHeight);

	posElement->SetValue(xPos, yPos, nbRow, 1, controlWidth, rowHeight, posElement->GetType(), element,
		posElement->GetTreeData());
}

// Ligne de titre de section : TITRE en gras, chevron a droite, clic sur toute la ligne
CTreeElementTriangle* CFiltreEffect::RenderHeaderRow(CTreeData* data, const int& profondeur, RenderMode mode)
{
	const bool isVisible = true;

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
	CPositionElement* posText = RenderText(data, xPos, yPos, isVisible, mode);
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

// Ligne de parametre : libelle attenue a gauche, controle (slide, case, liste, couleur, combo) a droite
void CFiltreEffect::RenderValueRow(CTreeDataEffect* data, const int& profondeur, RenderMode mode)
{
	const bool isVisible = true;
	const int indent = themeTree.CalculElementSizeForScreen(INDENT_PER_LEVEL) * profondeur;

	// Libelle (gris attenue, a gauche)
	int xPos = 0;
	CPositionElement* pos_element = RenderText(data, xPos, yPos, isVisible, mode, false);
	StyleTextCell(pos_element, labelColumnWidth, margin + indent, margin / 2, colorLabel, false, RENDERFONT_LEFT);

	if (data->HasValue())
	{
		xPos = margin / 2;
		pos_element = nullptr;

		if (data->GetType() == TYPE_SLIDE)
		{
			pos_element = RenderSlide(data, xPos, yPos, isVisible, mode);
			StyleControlCell(pos_element, xPos, true, true);
		}
		else if (data->GetType() == TYPE_CHECKBOX)
		{
			pos_element = RenderCheckbox(data, xPos, yPos, isVisible, mode);
			StyleControlCell(pos_element, xPos, false, true);
		}
		else if (data->GetType() == TYPE_LISTBOX)
		{
			pos_element = RenderList(data, xPos, yPos, isVisible, mode);
			StyleControlCell(pos_element, xPos, true, true);
		}
		else if (data->GetType() == TYPE_COLOR)
		{
			pos_element = RenderColor(data, xPos, yPos, isVisible, mode);
			StyleControlCell(pos_element, xPos, true, false);
		}
		else if (data->GetType() == TYPE_COMBOBOX)
		{
			pos_element = RenderComboBox(data, xPos, yPos, isVisible, mode);
			StyleControlCell(pos_element, xPos, true, false);
		}
	}

	yPos += rowHeight;
	nbRow++;
}

void CFiltreEffect::UpdateElement(tree<CTreeData*>::sibling_iterator& parent, RenderMode mode)
{
	tree<CTreeData*>::sibling_iterator it = tr.begin(parent);

	for (int i = 0; i < parent.number_of_children(); i++)
	{
		const int profondeur = tr.depth(it);
		auto data = static_cast<CTreeDataEffect*>(*it);

		if (data->HasValue() || it.number_of_children() == 0)
		{
			RenderValueRow(data, profondeur, mode);
		}
		else
		{
			CTreeElementTriangle* tree_element_triangle = RenderHeaderRow(data, profondeur, mode);

			if (mode == RenderMode::Create)
				UpdateElement(it, mode);
			else if (tree_element_triangle->GetOpen())
				UpdateElement(it, mode);
		}
		++it;
	}
}

void CFiltreEffect::RenderElement(RenderMode mode)
{
	tree<CTreeData*>::sibling_iterator it = tr.begin();
	auto itend = tr.end();

	UpdateLayoutMetrics();
	yPos = 0;
	nbRow = 0;

	if (mode == RenderMode::Create)
	{
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

	// --- À insérer au tout début de votre méthode CFiltreEffect::RenderElement ---
	for (CPositionElement* value : vectorPosElement)
	{
		if (value != nullptr && value->GetType() == ELEMENT_COMBOBOX)
		{
			CTreeElement* treeElement = value->GetTreeElement();
			if (treeElement != nullptr)
			{
				// On va forcer l'effacement visuel du widget natif associé
				// Pour cela, nous pouvons ajouter une petite méthode de commodité dans TreeElementComboBox
				// ou appeler une surcharge. Le plus simple est de mettre à jour le flag à false :
				treeElement->SetVisible(false);
			}
		}
	}


	while (it != itend)
	{
		CTreeData* data = *it;
		const int profondeur = tr.depth(it);
		if (profondeur == 0)
		{
			CTreeElementTriangle* tree_element_triangle = RenderHeaderRow(data, profondeur, mode);

			if (mode == RenderMode::Create)
				UpdateElement(it, mode);
			else if (tree_element_triangle->GetOpen())
				UpdateElement(it, mode);
		}
		++it;
	}
}

void CFiltreEffect::UnclickOnElement(CPositionElement* element, wxWindow* window, const int& x, const int& y,
	const int& posLargeur, const int& posHauteur)
{
	CTreeElement* treeElement = element->GetTreeElement();
	if (element->GetType() == ELEMENT_SLIDE)
	{
		auto treeElementSlide = static_cast<CTreeElementSlide*>(treeElement);

		int xPos = 0;
		if (treeElementSlide->GetRow() > 0)
			xPos = GetWidthRow(treeElementSlide->GetRow() - 1);

		treeElementSlide->UnclickElement(window, (x + posLargeur) - (element->GetX() + xPos),
			(y + posHauteur) - element->GetY());
	}
}

void CFiltreEffect::ClickOnElement(CPositionElement* element, wxWindow* window, const int& x, const int& y,
	const int& posLargeur, const int& posHauteur)
{
	bool update = false;
	CTreeElement* treeElement = element->GetTreeElement();
	if (element->GetType() == ELEMENT_SLIDE)
	{
		auto treeElementSlide = static_cast<CTreeElementSlide*>(treeElement);

		int xPos = 0;
		if (treeElementSlide->GetRow() > 0)
			xPos = GetWidthRow(treeElementSlide->GetRow() - 1);

		treeElementSlide->ClickElement(window, (x + posLargeur) - (element->GetX() + xPos),
			(y + posHauteur) - element->GetY());

		update = true;
	}
	else if (element->GetType() == ELEMENT_TRIANGLE)
	{
		auto treeElementTriangle = static_cast<CTreeElementTriangle*>(treeElement);
		treeElementTriangle->ClickElement(window, (x + posLargeur) - element->GetX(),
			(y + posHauteur) - element->GetY());
		update = true;
	}
	else if (element->GetType() == ELEMENT_TEXTE)
	{
		// Clic sur le titre d'une section : on bascule le chevron de la meme ligne
		CPositionElement* posTriangle = GetElement(element->GetTreeData(), ELEMENT_TRIANGLE);
		if (posTriangle != nullptr)
		{
			auto treeElementTriangle = static_cast<CTreeElementTriangle*>(posTriangle->GetTreeElement());
			treeElementTriangle->ClickElement(window, x, y);
			update = true;
		}
	}
	else if (element->GetType() == ELEMENT_CHECKBOX)
	{
		auto treeElementCheckBox = static_cast<CTreeElementCheckBox*>(treeElement);
		treeElementCheckBox->ClickElement(window, (x + posLargeur) - element->GetX(),
			(y + posHauteur) - element->GetY());

		if (filterEffect != nullptr)
		{
			auto data = static_cast<CTreeDataEffect*>(element->GetTreeData());
			CTreeElementValueBool tree_element_value_bool(treeElementCheckBox->GetCheckState());
			filterEffect->FilterChangeParam(effectParameter, &tree_element_value_bool, data->GetExifKey());
		}

		update = true;

		if (bitmapViewer != nullptr)
			bitmapViewer->UpdateFiltre(effectParameter);
	}
	else if (element->GetType() == ELEMENT_LISTBOX)
	{
		auto treeElementListbox = static_cast<CTreeElementListBox*>(treeElement);

		int xPos = 0;
		if (treeElementListbox->GetRow() > 0)
			xPos = GetWidthRow(treeElementListbox->GetRow() - 1);

		treeElementListbox->ClickElement(window, (x + posLargeur) - (element->GetX() + xPos),
			(y + posHauteur) - element->GetY());

		update = true;
	}
	// --- BRANCHEMENT DU COMPOSANT COULEUR ---
	else if (element->GetType() == ELEMENT_COLOR)
	{
		auto treeElementColor = static_cast<CTreeElementColor*>(treeElement);
		treeElementColor->ClickElement(window, (x + posLargeur) - element->GetX(),
			(y + posHauteur) - element->GetY());

		if (filterEffect != nullptr)
		{
			auto data = static_cast<CTreeDataEffect*>(element->GetTreeData());
			wxColour selectedColor = treeElementColor->GetColor();
			CTreeElementValueColor tree_element_value_color(selectedColor);
			filterEffect->FilterChangeParam(effectParameter, &tree_element_value_color, data->GetExifKey());
		}

		update = true;

		if (bitmapViewer != nullptr)
			bitmapViewer->UpdateFiltre(effectParameter);
	}
	// --- ENTRÉE AJOUTÉE POUR LA COMBOBOX ---
	else if (element->GetType() == ELEMENT_COMBOBOX)
	{
		auto treeElementComboBox = static_cast<CTreeElementComboBox*>(treeElement);

		int xPos = 0;
		if (treeElementComboBox->GetRow() > 0)
			xPos = GetWidthRow(treeElementComboBox->GetRow() - 1);

		// CORRECTION : On passe l'emplacement absolu exact calculé sur le canvas pour le composant wxWidgets
		int comboAbsoluteX = (element->GetX() + xPos) - posLargeur;
		int comboAbsoluteY = element->GetY() - posHauteur;

		// Déclenche l'ouverture au pixel près avec la bonne largeur m_width préservée
		treeElementComboBox->ClickElement(window, comboAbsoluteX, comboAbsoluteY);

	}


	if (update)
	{
		RenderElement(RenderMode::Update);
		eventControl->UpdateTreeControl();
	}
}


void CFiltreEffect::MouseOver(wxDC* deviceContext, CPositionElement* element, const int& x, const int& y,
	const int& posLargeur, const int& posHauteur, bool& update)
{
	CTreeElement* treeElement = element->GetTreeElement();
	if (element->GetType() == ELEMENT_SLIDE)
	{
		auto treeElementSlide = static_cast<CTreeElementSlide*>(treeElement);

		int xPos = 0;
		if (treeElementSlide->GetRow() > 0)
			xPos = GetWidthRow(treeElementSlide->GetRow() - 1);

		//
		if (treeElementSlide->MouseBlock())
		{
			treeElementSlide->MouseOver(deviceContext, (x + posLargeur) - (element->GetX() + xPos),
				(y + posHauteur) - element->GetY(), update);
			treeElementSlide->DrawElement(deviceContext, (element->GetX() + xPos) - posLargeur,
				element->GetY() - posHauteur);
		}
	}
}