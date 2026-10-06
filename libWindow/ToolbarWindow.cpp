#include "header.h"
#include "ToolbarWindow.h"
#include <wx/dcbuffer.h>
#include <LibResource.h>
using namespace Regards::Window;

namespace
{
	constexpr int TIMER_PUSHID = 1;
	constexpr int TIMER_PUSH_DELAY = 500; // ms before repeat fires
}



CToolbarWindow::CToolbarWindow(wxWindow* parent, wxWindowID id, const CThemeToolbar& theme, const bool& vertical) :
	CWindowMain("CToolbarWindow", parent, id)
{
	isVertical = vertical;
	m_bMouseOver = false;
	m_bMouseMove = false;
	saveLastPush = false;
	m_bIconeOn = false;
	numButtonActif = -1;
	navPush = nullptr;
	pushButton = std::make_unique<wxTimer>(this, TIMER_PUSHID);
	themeToolbar = theme;


	Connect(wxEVT_PAINT, wxPaintEventHandler(CToolbarWindow::on_paint));
	Connect(wxEVT_MOTION, wxMouseEventHandler(CToolbarWindow::OnMouseMove));
	Connect(wxEVT_LEFT_DOWN, wxMouseEventHandler(CToolbarWindow::OnLButtonDown));
	Connect(wxEVT_LEFT_UP, wxMouseEventHandler(CToolbarWindow::OnLButtonUp));
	Connect(wxEVT_ENTER_WINDOW, wxMouseEventHandler(CToolbarWindow::OnMouseHover));
	Connect(wxEVT_LEAVE_WINDOW, wxMouseEventHandler(CToolbarWindow::OnMouseLeave));
	Connect(wxEVT_MOUSE_CAPTURE_LOST, wxMouseEventHandler(CToolbarWindow::OnMouseCaptureLost));
	Connect(TIMER_PUSHID, wxEVT_TIMER, wxTimerEventHandler(CToolbarWindow::OnTimerPushButton), nullptr, this);
	Connect(wxEVT_ERASE_BACKGROUND, wxEraseEventHandler(CToolbarWindow::OnEraseBackground));
}


void CToolbarWindow::OnMouseCaptureLost(wxMouseEvent& event)
{}


int CToolbarWindow::GetHeight()
{
	return themeToolbar.GetHeight();
}

int CToolbarWindow::GetWidth()
{
	return themeToolbar.GetWidth();
}

CToolbarWindow::~CToolbarWindow()
{
	if (pushButton->IsRunning())
	{
		pushButton->Stop();
	}
	EmptyNavigator();
}


void CToolbarWindow::OnTimerPushButton(wxTimerEvent& event)
{
	if (navPush != nullptr)
	{
		EventManager(navPush->GetCommandId());
	}
	//pushButton->Stop();
}

void CToolbarWindow::EmptyNavigator()
{
	navElement.clear();
}

void CToolbarWindow::UpdateScreenRatio()
{
	needToRefresh = true;
}

int CToolbarWindow::GetNavigatorHeight()
{
	int size = 0;
	if (isVertical)
	{
		for (CToolbarElement* nav : navElement)
		{
			if (nav->IsVisible())
				size += themeToolbar.GetMargeX() + nav->GetHeight();
		}
	}
	else
	{
		for (CToolbarElement* nav : navElement)
		{
			if (nav->IsVisible())
				if (size < nav->GetHeight())
					size = nav->GetHeight();
		}
	}
	return size;
}

int CToolbarWindow::GetNavigatorWidth()
{
	int size = 0;
	if (isVertical)
	{
		for (CToolbarElement* nav : navElement)
		{
			if (nav->IsVisible())
				if (size < nav->GetWidth())
					size = nav->GetWidth();
		}
	}
	else
	{
		for (CToolbarElement* nav : navElement)
		{
			if (nav->IsVisible())
				size += themeToolbar.GetMargeX() + nav->GetWidth();
		}
	}
	return size;
}


void CToolbarWindow::OnMouseMove(wxMouseEvent& event)
{
	const int xPos = event.GetX();
	const int yPos = event.GetY();
	bool needToRedraw = false;
	int  i = 0;

	for (auto& nav : navElement)
	{
		if (!nav || !nav->IsVisible()) { ++i; continue; }

		if (nav->FindElement(xPos, yPos))
		{
			// Only create DC when we actually need to draw
			wxClientDC dc(this);
			if (nav->MouseOver(&dc, xPos, yPos)) needToRedraw = true;
			if (nav->SetActif())                 needToRedraw = true;

			if (numButtonActif != i)
			{
				SetToolTip(nav->GetLibelleTooltip());
				numButtonActif = i;
			}
		}
		else
		{
			if (nav->SetInactif()) needToRedraw = true;
		}
		++i;
	}

	if (needToRedraw)
		needToRefresh = true;
}


void CToolbarWindow::RedrawElement(wxDC* dc, CToolbarElement* nav)
{
	DrawButton(dc, nav);
}

void CToolbarWindow::DrawButton(wxDC* dc, CToolbarElement* nav)
{
	//nav->DrawButton(dc, nav->GetXPos(), nav->GetYPos());

	
	if (!pictureBuffer.IsOk() || pictureBuffer.GetWidth() != nav->GetWidth() || pictureBuffer.GetHeight() != nav->GetHeight())
		pictureBuffer.Create(nav->GetWidth(), nav->GetHeight());

	wxMemoryDC memDC(pictureBuffer);

	wxRect rc;
	rc.x = 0;
	rc.y = 0;
	rc.width = nav->GetWidth();
	rc.height = nav->GetHeight();
	DrawBackground(&memDC, rc);
	nav->DrawButton(&memDC, 0, 0);
	memDC.SelectObject(wxNullBitmap);

	if (pictureBuffer.IsOk())
	{
		dc->DrawBitmap(pictureBuffer, nav->GetXPos(), nav->GetYPos());
	}
}

void CToolbarWindow::OnLButtonDown(wxMouseEvent& event)
{
	SetFocus();
	const int xPos = event.GetX();
	const int yPos = event.GetY();
	bool repeatable = false;
	bool needToRedraw = false;

	for (auto& nav : navElement)
	{
		if (!nav->IsVisible()) continue;

		if (nav->FindElement(xPos, yPos))
		{
			nav->ClickElement(this, xPos, yPos);
			nav->SetPush(true);
			needToRedraw = true;
			navPush = nav;

			if (navPush->GetRepeatable())
			{
				if (pushButton->IsRunning())
					pushButton->Stop();
				pushButton->Start(TIMER_PUSH_DELAY);
				repeatable = true;
			}
		}
		else
		{
			nav->SetPush(false);
			needToRedraw = true;
		}
	}

	if (!repeatable && navPush)
		EventManager(navPush->GetCommandId());

	if (needToRedraw)
		needToRefresh = true;
}

void CToolbarWindow::OnLButtonUp(wxMouseEvent& event)
{
	const int xPos = event.GetX();
	const int yPos = event.GetY();

	for (auto& nav : navElement)
	{
		if (!nav->IsVisible()) continue;

		if (nav->FindElement(xPos, yPos))
		{
			nav->UnclickElement(this, xPos, yPos);
			if (!saveLastPush)
				nav->SetPush(false);
			break;
		}
	}

	if (pushButton && pushButton->IsRunning())
		pushButton->Stop();

	if (navPush && navPush->GetRepeatable())
	{
		EventManager(navPush->GetCommandId());
		navPush = nullptr;
	}

	needToRefresh = true;
}

std::unique_ptr<CToolbarTexte> CToolbarWindow::CreateTexte(
	const wxString& label,
	int commandId)
{
	auto btn = std::make_unique<CToolbarTexte>(themeToolbar.texte);
	const wxString libelle = CLibResource::LoadStringFromResource(label, 1);

	btn->SetCommandId(commandId);
	btn->SetLibelle(libelle);
	btn->SetLibelleTooltip(libelle);
	navElement.push_back(btn.get());
	return btn;
}

std::unique_ptr<CToolbarButton> CToolbarWindow::CreateButton(
	const wxString& icon,
	const wxString& label,
	int commandId,
	bool showLibelle)
{
	auto btn = std::make_unique<CToolbarButton>(themeToolbar.button);
	const wxString libelle = CLibResource::LoadStringFromResource(label, 1);
	btn->SetButtonResourceId(icon);
	if (showLibelle)
		btn->SetLibelle(libelle);
	btn->SetCommandId(commandId);
	btn->SetLibelleTooltip(libelle);
	navElement.push_back(btn.get());

	return btn;
}

void CToolbarWindow::OnMouseLeave(wxMouseEvent& event)
{
	m_bMouseOver = false;
	if (HasCapture())
		ReleaseMouse();

	wxClientDC dc(this);
	for (auto& nav : navElement)
	{
		if (nav->SetInactif())
			RedrawElement(&dc, nav);
	}
}

void CToolbarWindow::OnMouseHover(wxMouseEvent& event)
{
	wxSetCursor(wxCursor(wxCURSOR_ARROW));
	m_bMouseOver = true;
}



bool CToolbarWindow::IsMouseOver()
{
	return m_bMouseOver;
}


void CToolbarWindow::SetAllDisable()
{
	for (CToolbarElement* nav : navElement)
	{
		nav->SetPush(false);
		nav->SetInactif();
	}
}


void CToolbarWindow::DrawBackground(wxDC* dc, const wxRect& rc)
{
	const wxDirection dir = isVertical ? wxEAST : wxNORTH;
	dc->GradientFillLinear(rc, themeToolbar.colorTop, themeToolbar.colorBottom, dir);

	if (showLine)
	{
		wxPen penTop(themeTexte.rectTop, themeTexte.GetRectangleSize(), wxPENSTYLE_SOLID);
		dc->SetPen(penTop);
		dc->DrawLine(0, GetWindowHeight(), GetWindowWidth(), GetWindowHeight());
		dc->SetPen(wxNullPen);
	}
}


void CToolbarWindow::DrawBackground(wxDC* dc)
{
	const int winW = GetWindowWidth();
	const int winH = GetWindowHeight();
	if (winW <= 0 || winH <= 0) return;

	// Only reallocate the off-screen bitmap when the window is resized
	if (!background.IsOk() || background.GetWidth() != winW || background.GetHeight() != winH)
		background.Create(winW, winH);

	{
		wxMemoryDC memDC(background);
		DrawBackground(&memDC, GetWindowRect());
		memDC.SelectObject(wxNullBitmap);
	}

	backPicture = background.ConvertToImage();
	dc->DrawBitmap(background, 0, 0);
}



void CToolbarWindow::GenerateNavigatorButton(wxDC* deviceContext)
{
	for (CToolbarElement* nav : navElement)
	{
		if (nav->IsVisible())
		{
			DrawButton(deviceContext, nav);
		}
	}
}


// ---------------------------------------------------------------------------
// Layout recalculation (separated from paint so it runs only when needed)
// ---------------------------------------------------------------------------
void CToolbarWindow::RecalcLayout()
{
	int maxRows = themeToolbar.GetNbRow();
	int maxCols = themeToolbar.GetNbCol();

	// Filtrer la liste pour n'avoir que les éléments visibles
	std::vector<CToolbarElement*> visibleElements;
	for (auto& nav : navElement)
	{
		if (nav && nav->IsVisible())
			visibleElements.push_back(nav);
	}

	if (visibleElements.empty()) return;

	// Détermination du mode de rendu (Grille vs Linéaire classique)
	bool useGrid = (maxRows != -1 || maxCols != -1);

	if (useGrid)
	{
		int totalElements = static_cast<int>(visibleElements.size());

		// 1. Ajustement automatique des dimensions manquantes (-1)
		if (maxRows == -1 && maxCols != -1)
		{
			maxRows = (totalElements + maxCols - 1) / maxCols;
		}
		else if (maxCols == -1 && maxRows != -1)
		{
			maxCols = (totalElements + maxRows - 1) / maxRows;
		}

		// Sécurité : éviter les divisions par zéro
		if (maxRows <= 0) maxRows = 1;
		if (maxCols <= 0) maxCols = 1;

		// 2. Calcul des dimensions maximales d'une cellule de la grille
		int cellWidth = 0;
		int cellHeight = 0;
		for (auto* nav : visibleElements)
		{
			if (nav->GetWidth() > cellWidth)   cellWidth = nav->GetWidth();
			if (nav->GetHeight() > cellHeight) cellHeight = nav->GetHeight();
		}

		// Taille totale occupée par la grille complète
		int gridWidth = maxCols * cellWidth + (maxCols - 1) * themeToolbar.GetMargeX();
		int gridHeight = maxRows * cellHeight + (maxRows - 1) * themeToolbar.GetMargeY();

		// 3. Alignement global de la grille dans la fenêtre
		int gridXStart = 0;
		int gridYStart = 0;

		switch (themeToolbar.position)
		{
		case NAVIGATOR_CENTER:
			gridXStart = (GetWindowWidth() - gridWidth) / 2;
			gridYStart = (GetWindowHeight() - gridHeight) / 2;
			break;
		case NAVIGATOR_RIGHT:
			gridXStart = GetWindowWidth() - gridWidth;
			gridYStart = GetWindowHeight() - gridHeight;
			break;
		default: // NAVIGATOR_LEFT
			gridXStart = 0;
			gridYStart = 0;
			break;
		}

		// 4. Placement des éléments selon l'axe principal (isVertical)
		for (int i = 0; i < totalElements; ++i)
		{
			int row = 0;
			int col = 0;

			if (isVertical)
			{
				// Remplissage par colonne : on remplit d'abord l'axe Y (les lignes)
				row = i % maxRows;
				col = i / maxRows;

				// Si le nombre d'éléments dépasse la grille maximale allouée
				if (col >= maxCols) break;
			}
			else
			{
				// Remplissage par ligne : on remplit d'abord l'axe X (les colonnes)
				row = i / maxCols;
				col = i % maxCols;

				// Si le nombre d'éléments dépasse la grille maximale allouée
				if (row >= maxRows) break;
			}

			// Calcul des coordonnées de la cellule
			int xPos = gridXStart + col * (cellWidth + themeToolbar.GetMargeX());
			int yPos = gridYStart + row * (cellHeight + themeToolbar.GetMargeY());

			// Centrage de l'élément à l'intérieur de sa cellule individuelle
			xPos += (cellWidth - visibleElements[i]->GetWidth()) / 2;
			yPos += (cellHeight - visibleElements[i]->GetHeight()) / 2;

			visibleElements[i]->SetPosition(xPos, yPos);
		}
	}
	else
	{
		// 5. Comportement linéaire d'origine (si nbRow == -1 && nbCol == -1)
		const int navW = GetNavigatorWidth();
		const int navH = GetNavigatorHeight();

		if (isVertical)
		{
			int xStart = (GetWindowWidth() - navW) / 2;
			int yStart = 0;

			switch (themeToolbar.position)
			{
			case NAVIGATOR_CENTER: yStart = (GetWindowHeight() - navH) / 2; break;
			case NAVIGATOR_RIGHT:  yStart = GetWindowHeight() - navH;        break;
			default:               yStart = 0;                               break;
			}

			for (auto* nav : visibleElements)
			{
				nav->SetPosition(xStart, yStart);
				yStart += themeToolbar.GetMargeY() + nav->GetHeight();
			}
		}
		else
		{
			int xStart = 0;
			int yStart = (GetWindowHeight() - navH) / 2;

			switch (themeToolbar.position)
			{
			case NAVIGATOR_CENTER: xStart = (GetWindowWidth() - navW) / 2; break;
			case NAVIGATOR_RIGHT:  xStart = GetWindowWidth() - navW;        break;
			default:               xStart = 0;                              break;
			}

			for (auto* nav : visibleElements)
			{
				nav->SetPosition(xStart, yStart);
				xStart += themeToolbar.GetMargeX() + nav->GetWidth();
			}
		}
	}
}


/*
void CToolbarWindow::RecalcLayout()
{
	const int navW = GetNavigatorWidth();   // also fills m_cachedNavHeight
	const int navH = GetNavigatorHeight();

	if (isVertical)
	{
		int xStart = (GetWindowWidth() - navW) / 2;
		int yStart = 0;

		switch (themeToolbar.position)
		{
		case NAVIGATOR_CENTER: yStart = (GetWindowHeight() - navH) / 2; break;
		case NAVIGATOR_RIGHT:  yStart = GetWindowHeight() - navH;        break;
		default:               yStart = 0;                               break;
		}

		for (auto& nav : navElement)
		{
			if (!nav->IsVisible()) continue;
			nav->SetPosition(xStart, yStart);
			yStart += themeToolbar.GetMargeY() + nav->GetHeight();
		}
	}
	else
	{
		int xStart = 0;
		int yStart = (GetWindowHeight() - navH) / 2;

		switch (themeToolbar.position)
		{
		case NAVIGATOR_CENTER: xStart = (GetWindowWidth() - navW) / 2; break;
		case NAVIGATOR_RIGHT:  xStart = GetWindowWidth() - navW;        break;
		default:               xStart = 0;                              break;
		}

		for (auto& nav : navElement)
		{
			if (!nav->IsVisible()) continue;
			nav->SetPosition(xStart, yStart);
			xStart += themeToolbar.GetMargeX() + nav->GetWidth();
		}
	}
}
*/

void CToolbarWindow::on_paint(wxPaintEvent& event)
{
	wxSize size = GetClientSize();
	if (size.x <= 0 || size.y <= 0)
		return;

	wxBufferedPaintDC dc(this);

	DrawBackground(&dc);
	RecalcLayout();
	GenerateNavigatorButton(&dc);
}