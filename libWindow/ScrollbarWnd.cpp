#include "header.h"
#include "ScrollbarWnd.h"
#include "ScrollbarHorizontalWnd.h"
#include "ScrollbarVerticalWnd.h"
#include "MainThemeInit.h"
#include "MainTheme.h"
using namespace Regards::Window;

CScrollbarWnd::CScrollbarWnd(wxWindow* parent, CWindowMain* centralWindow, wxWindowID id, const wxString& windowName)
	: CWindowMain(windowName, parent, id)
{
	DefaultConstructor();
	AttachCentralWindow(centralWindow);
}


CScrollbarWnd::CScrollbarWnd(wxWindow* parent, CWindowOpenGLMain* centralWindow, wxWindowID id,
	const wxString& windowName)
	: CWindowMain(windowName, parent, id)
{
	DefaultConstructor();
	AttachCentralWindow(centralWindow);
}


void CScrollbarWnd::DefaultConstructor()
{
	CMainTheme* viewerTheme = CMainThemeInit::getInstance();

	scrollHorizontal = nullptr;
	scrollVertical = nullptr;
	showV = true;
	showH = true;
	controlHeight = 0;
	controlWidth = 0;

	// [QUALITE] loadingTimer supprimé — était alloué mais jamais démarré ni utilisé

	if (viewerTheme != nullptr)
	{
		CThemeScrollBar theme;
		viewerTheme->GetScrollTheme(&theme);
		// Enfants directs de CScrollbarWnd : leur position/taille est gérée par Resize().
		scrollHorizontal = new CScrollbarHorizontalWnd("ScrollbarHorizontal", this, wxID_ANY, theme);
		scrollVertical = new CScrollbarVerticalWnd("ScrollbarVertical", this, wxID_ANY, theme);
	}

	// [IMPORTANT] Migration Connect() → Bind() : type-safe, supporte les lambdas, idiomatique wx3

	Connect(wxEVENT_REFRESHDATA, wxCommandEventHandler(CScrollbarWnd::RefreshData));
	Connect(wxEVENT_LEFTPOSITION, wxCommandEventHandler(CScrollbarWnd::OnLeftPosition));
	Connect(wxEVENT_TOPPOSITION, wxCommandEventHandler(CScrollbarWnd::OnTopPosition));
	Connect(wxEVENT_SETCONTROLSIZE, wxCommandEventHandler(CScrollbarWnd::OnControlSize));
	Connect(wxEVENT_SETPOSITION, wxCommandEventHandler(CScrollbarWnd::OnSetPosition));
	Connect(wxEVENT_MOVELEFT, wxCommandEventHandler(CScrollbarWnd::OnMoveLeft));
	Connect(wxEVENT_MOVERIGHT, wxCommandEventHandler(CScrollbarWnd::OnMoveRight));
	Connect(wxEVENT_MOVETOP, wxCommandEventHandler(CScrollbarWnd::OnMoveTop));
	Connect(wxEVENT_MOVEBOTTOM, wxCommandEventHandler(CScrollbarWnd::OnMoveBottom));
	Connect(wxEVENT_SCROLLMOVE, wxCommandEventHandler(CScrollbarWnd::OnScrollMove));

	defaultPageSize = 50;
	defaultLineSize = 5;
}

void CScrollbarWnd::AttachCentralWindow(wxWindow* window)
{
	centralWindow = window;
	if (centralWindow != nullptr && centralWindow->GetParent() != this)
		centralWindow->Reparent(this);
}

// [IMPORTANT] Helper centralisant le forwarding d'événements vers la fenêtre centrale.
// Supprime la duplication présente dans OnMoveLeft/Right/Top/Bottom et OnLeftPosition/TopPosition.
void CScrollbarWnd::ForwardScrollEvent(wxEventType type, int pos)
{
	if (centralWindow != nullptr)
	{
		wxCommandEvent evt(type);
		evt.SetInt(pos);
		centralWindow->GetEventHandler()->AddPendingEvent(evt);
	}
}

void CScrollbarWnd::OnScrollMove(wxCommandEvent& event)
{
	ForwardScrollEvent(wxEVENT_SCROLLMOVE, event.GetInt());
}

void CScrollbarWnd::OnMoveLeft(wxCommandEvent& event)
{
	scrollHorizontal->ClickLeftPage();
	ForwardScrollEvent(wxEVENT_LEFTPOSITION, scrollHorizontal->GetPosition());
}

void CScrollbarWnd::OnMoveRight(wxCommandEvent& event)
{
	scrollHorizontal->ClickRightPage();
	ForwardScrollEvent(wxEVENT_LEFTPOSITION, scrollHorizontal->GetPosition());
}

void CScrollbarWnd::OnMoveTop(wxCommandEvent& event)
{
	scrollVertical->ClickTopPage();
	ForwardScrollEvent(wxEVENT_TOPPOSITION, scrollVertical->GetPosition());
}

void CScrollbarWnd::OnMoveBottom(wxCommandEvent& event)
{
	scrollVertical->ClickBottomPage();
	ForwardScrollEvent(wxEVENT_TOPPOSITION, scrollVertical->GetPosition());
}

void CScrollbarWnd::OnLeftPosition(wxCommandEvent& event)
{
	ForwardScrollEvent(wxEVENT_LEFTPOSITION, event.GetInt());
}

void CScrollbarWnd::OnTopPosition(wxCommandEvent& event)
{
	ForwardScrollEvent(wxEVENT_TOPPOSITION, event.GetInt());
}

void CScrollbarWnd::RefreshData(wxCommandEvent& event)
{
	if (centralWindow != nullptr)
	{
		wxCommandEvent evt(wxEVENT_REFRESHDATA);
		evt.SetExtraLong(1);
		centralWindow->GetEventHandler()->AddPendingEvent(evt);
	}
}

int CScrollbarWnd::GetHeight()
{
	return GetWindowHeight();
}

int CScrollbarWnd::GetWidth()
{
	return GetWindowWidth();
}

// [QUALITE] const ajouté sur SetPageSize/SetLineSize — paramètres scalaires passés par valeur
void CScrollbarWnd::SetPageSize(int pageSize)
{
	scrollHorizontal->SetPageSize(pageSize);
	scrollVertical->SetPageSize(pageSize);
}

void CScrollbarWnd::SetLineSize(int lineSize)
{
	scrollHorizontal->SetLineSize(lineSize);
	scrollVertical->SetLineSize(lineSize);
}

// [CRITIQUE] Garde nullptr : scrollHorizontal/scrollVertical sont initialisés conditionnellement
int CScrollbarWnd::GetBarHeight() const
{
	return scrollHorizontal ? scrollHorizontal->GetHeightSize() : 0;
}

int CScrollbarWnd::GetBarWidth() const
{
	return scrollVertical ? scrollVertical->GetWidthSize() : 0;
}


void CScrollbarWnd::UpdateScreenRatio()
{
	scrollVertical->UpdateScreenRatio();
	scrollHorizontal->UpdateScreenRatio();
	this->Resize();
}

void CScrollbarWnd::OnSetPosition(wxCommandEvent& event)
{
	auto size = static_cast<wxSize*>(event.GetClientData());
	if (size != nullptr)
	{
		int posX = size->x;
		int posY = size->y;
		SetPosition(posX, posY);
		delete size;
	}
}

void CScrollbarWnd::OnControlSize(wxCommandEvent& event)
{
	auto controlSize = static_cast<CControlSize*>(event.GetClientData());
	if (controlSize != nullptr)
	{
		if (controlSize->controlWidth != controlWidth || controlSize->controlHeight != controlHeight)
		{
			controlWidth = controlSize->controlWidth;
			controlHeight = controlSize->controlHeight;
			_useScaleFactor = controlSize->useScaleFactor;
			this->Resize();
		}
		delete controlSize;
	}
}

int CScrollbarWnd::GetPosLargeur() const
{
	return scrollHorizontal->GetPosition();
}

int CScrollbarWnd::GetPosHauteur() const
{
	return scrollVertical->GetPosition();
}

// [QUALITE] bool au lieu de int pour les getters de visibilité
bool CScrollbarWnd::GetShowingScrollV() const { return showV; }
bool CScrollbarWnd::GetShowingScrollH() const { return showH; }

void CScrollbarWnd::ShowVerticalScroll() { showV = true; }
void CScrollbarWnd::ShowHorizontalScroll() { showH = true; }
void CScrollbarWnd::HideVerticalScroll() { showV = false; }
void CScrollbarWnd::HideHorizontalScroll() { showH = false; }

// [QUALITE] Paramètres int passés par valeur (pas de const& sur les scalaires)
void CScrollbarWnd::SetPosition(int posX, int posY)
{
	int pictureWidth = GetWindowWidth();
	int pictureHeight = GetWindowHeight();

	if (scrollHorizontal != nullptr && scrollVertical != nullptr)
	{
		if (scrollVertical->IsShown())
			pictureWidth -= scrollVertical->GetWidthSize();

		if (scrollHorizontal->IsShown())
			pictureHeight -= scrollHorizontal->GetHeightSize();
	}

	int maxPosHeight = max(controlHeight - pictureHeight, 0);
	int maxPosWidth = max(controlWidth - pictureWidth, 0);

	int posHauteur = std::min(posY, maxPosHeight);
	int posLargeur = std::min(posX, maxPosWidth);

	if (scrollVertical != nullptr && showV) scrollVertical->SetPosition(posHauteur);
	if (scrollHorizontal != nullptr && showH) scrollHorizontal->SetPosition(posLargeur);
}

void CScrollbarWnd::Resize()
{
	const int pictureWidth = GetWindowWidth();
	const int pictureHeight = GetWindowHeight();

	if (!(pictureWidth > 0 && pictureHeight > 0))
		return;

	if (scrollHorizontal == nullptr || scrollVertical == nullptr)
	{
		if (centralWindow != nullptr)
			centralWindow->SetSize(0, 0, pictureWidth, pictureHeight);
		return;
	}

	const double scale_factor = _useScaleFactor ? 1.0 : 1.0; // placeholder si DPI variable

	// [CRITIQUE] Calcul des visibilités AVANT toute modification des dimensions,
	// pour éviter que showScrollV influe sur le test de showScrollH.
	const bool showScrollV = showV && (controlHeight > pictureHeight * scale_factor);
	const bool showScrollH = showH && (controlWidth > pictureWidth * scale_factor);

	const int barWidth = scrollVertical->GetWidthSize();
	const int barHeight = scrollHorizontal->GetHeightSize();

	// Dimensions effectives disponibles pour la fenêtre centrale et chaque barre
	const int effectiveW = pictureWidth - (showScrollV ? barWidth : 0);
	const int effectiveH = pictureHeight - (showScrollH ? barHeight : 0);

	const int posLargeur = scrollHorizontal->GetPosition();
	const int posHauteur = scrollVertical->GetPosition();

	// Mise en page (remplace CWindowManager) :
	//  - fenêtre centrale en haut à gauche ;
	//  - barre horizontale en bas, sur toute la largeur ;
	//  - barre verticale à droite, qui s'arrête à la barre horizontale.
	// Le coin (barWidth x barHeight) appartient à la barre horizontale, qui le laisse vide
	// (ShowEmptyRectangle) lorsque les deux barres sont affichées.
	if (centralWindow != nullptr)
		centralWindow->SetSize(0, 0, effectiveW, effectiveH);

	scrollVertical->Show(showScrollV);
	scrollHorizontal->Show(showScrollH);

	if (showScrollV)
		scrollVertical->SetSize(effectiveW, 0, barWidth, effectiveH);

	if (showScrollH)
		scrollHorizontal->SetSize(0, effectiveH, pictureWidth, barHeight);

	scrollHorizontal->ShowEmptyRectangle(showScrollV && showScrollH, barWidth);

	if (showScrollV)
		scrollVertical->UpdateScrollBar(posHauteur, effectiveH * scale_factor, controlHeight);

	if (showScrollH)
		scrollHorizontal->UpdateScrollBar(posLargeur, effectiveW * scale_factor, controlWidth);

	if (oldshowScrollH != showScrollH)
	{
		oldshowScrollH = showScrollH;
		if (centralWindow != nullptr)
		{
			wxCommandEvent evt(wxEVENT_SCROLLBARH);
			evt.SetInt(showScrollH);
			evt.SetExtraLong(barHeight);
			centralWindow->GetEventHandler()->AddPendingEvent(evt);
		}
	}

	scrollVertical->SetShowWindow(showScrollV);
	scrollHorizontal->SetShowWindow(showScrollH);
}
