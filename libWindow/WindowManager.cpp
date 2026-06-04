#include "header.h"
#include "WindowManager.h"
#include "PanelWithClickToolbar.h"
#include "MainTheme.h"
#include "WindowToAdd.h"
#include "SeparationBar.h"

#include <algorithm>
#include <cassert>

using namespace Regards::Window;

#define WINDOW_MINSIZE 100

#ifndef WIN32
#define WM_USER 0x4000
#endif

// ---------------------------------------------------------------------------
// Helpers — free functions
// ---------------------------------------------------------------------------

namespace
{
	// Returns the size of a slot's separator, or 0 if it has none.
	int SepSize(const CWindowToAdd* slot) noexcept
	{
		return (slot && slot->separationBar) ? slot->separationBar->size : 0;
	}

	// Returns the effective display width of a slot (content + separator).
	int SlotTotalWidth(const CWindowToAdd* slot) noexcept
	{
		if (!slot || slot->isHide) return 0;
		return slot->rect.width + SepSize(slot);
	}

	// Returns the effective display height of a slot (content + separator).
	int SlotTotalHeight(const CWindowToAdd* slot) noexcept
	{
		if (!slot || slot->isHide) return 0;
		return (slot->fixe ? slot->size : slot->rect.height) + SepSize(slot);
	}

	// Safely shows or hides a wxWindow.
	void SafeShow(wxWindow* wnd, bool show) noexcept
	{
		if (wnd) wnd->Show(show);
	}

	// Pos → compact integer index for LayoutContext::slots[].
	constexpr int PosIndex(Pos p) noexcept { return static_cast<int>(p); }

} // anonymous namespace

// ---------------------------------------------------------------------------
// Construction / destruction
// ---------------------------------------------------------------------------

CWindowManager::CWindowManager(wxWindow* parent, wxWindowID id,
	const CThemeSplitter& theme)
	: CWindowMain("CWindowManager", parent, id)
	, themeSplitter(theme)
{
	separationBarSize = themeSplitter.themeSeparation.size;

	// [IMPORTANT] Bind() replaces Connect() — type-safe, supports lambdas
	//Bind(wxEVENT_REFRESHDATA, &CWindowManager::OnRefreshData, this);
	//Bind(wxEVENT_RESIZE, &CWindowManager::OnResize, this);
	Connect(wxEVENT_REFRESHDATA, wxCommandEventHandler(CWindowManager::OnRefreshData));
	Connect(wxEVENT_RESIZE, wxCommandEventHandler(CWindowManager::OnResize));

}

CWindowManager::~CWindowManager()
{
	// unique_ptr elements are destroyed automatically by the vector destructor.
	// CSeparationBar children (wxWindow) are destroyed by the wxWidgets window
	// tree (parent = this). CSeparationBarToAdd wrapper structs are destroyed
	// as part of their owning CWindowToAdd.
}

// ---------------------------------------------------------------------------
// Slot management
// ---------------------------------------------------------------------------

CWindowToAdd* CWindowManager::FindSlot(Pos position) const
{
	for (const auto& slot : listWindow)
		if (slot && slot->position == position)
			return slot.get();
	return nullptr;
}

void CWindowManager::RegisterSlot(std::unique_ptr<CWindowToAdd> slot,
	Pos position, bool fixe, int size,
	wxRect rect, int id, bool isPanel, bool isTop)
{
	assert(slot != nullptr);

	slot->position = position;
	slot->size = size;
	slot->isTop = isTop;
	slot->fixe = fixe;
	slot->rect = rect;
	slot->id = id;
	slot->isPanel = isPanel;
	slot->isHide = false;

	if (!fixe && position != Pos::wxCENTRAL)
	{
		auto* sep = new CSeparationBarToAdd();
		sep->separationBarId = static_cast<int>(listWindow.size()) + WM_USER + 1200;
		sep->separationBar = new CSeparationBar(
			this, this, sep->separationBarId, themeSplitter.themeSeparation);
		sep->size = themeSplitter.themeSeparation.size;

		const bool isVerticalSplit = (position == Pos::wxLEFT ||
			position == Pos::wxRIGHT);
		sep->separationBar->SetHorizontal(!isVerticalSplit);
		sep->isHorizontal = !isVerticalSplit;

		slot->separationBar = sep;
	}
	else
	{
		slot->separationBar = nullptr;
	}

	listWindow.push_back(std::move(slot));
}

// Public AddWindow overloads — guard nullptr, then delegate to RegisterSlot
void CWindowManager::AddWindow(CWindowMain* window, Pos position, bool fixe,
	int size, wxRect rect, int id, bool isPanel,
	bool isTop)
{
	if (!window) return;  // [CRITIQUE] guard before allocation
	auto slot = std::make_unique<CWindowToAdd>();
	window->Reparent(this);
	slot->SetWindow(window, isPanel);
	RegisterSlot(std::move(slot), position, fixe, size, rect, id, isPanel, isTop);
}

void CWindowManager::AddWindow(CWindowOpenGLMain* window, Pos position,
	bool fixe, int size, wxRect rect, int id,
	bool isPanel, bool isTop)
{
	if (!window) return;  // [CRITIQUE] guard before allocation
	auto slot = std::make_unique<CWindowToAdd>();
	window->Reparent(this);
	slot->SetWindow(window, isPanel);
	RegisterSlot(std::move(slot), position, fixe, size, rect, id, isPanel, isTop);
}

CPanelWithClickToolbar* CWindowManager::AddPanel(CWindowMain* window, Pos pos,
	bool fixe, int size,
	wxRect rect,
	const wxString& panelLabel,
	const wxString& windowName,
	bool isVisible, int idPanel,
	bool refreshButton, bool isTop)
{
	const bool isVertical = (pos == Pos::wxLEFT || pos == Pos::wxRIGHT);
	auto* panel = CPanelWithClickToolbar::CreatePanel(
		this, panelLabel, windowName, isVisible, idPanel, isVertical, refreshButton);
	panel->SetWindow(window);
	AddWindow(panel, pos, fixe, size, rect, idPanel, true, isTop);
	return panel;
}

void CWindowManager::ChangeWindow(CWindowMain* window, Pos position, bool isPanel)
{
	CWindowToAdd* slot = FindSlot(position);
	if (slot)
	{
		window->Reparent(this);
		slot->SetWindow(window, isPanel);
	}
}

// ---------------------------------------------------------------------------
// Event handlers
// ---------------------------------------------------------------------------

void CWindowManager::OnRefreshData(wxCommandEvent& event)
{
	const int id = event.GetId();
	for (const auto& slot : listWindow)
	{
		if (!slot || slot->id != id) continue;
		wxWindow* wnd = slot->GetWindow();
		if (wnd)
		{
			wxCommandEvent evt(wxEVENT_REFRESHDATA);
			evt.SetExtraLong(1);
			wnd->GetEventHandler()->AddPendingEvent(evt);
		}
	}
}

void CWindowManager::OnResize(wxCommandEvent& event)
{
	const int id = event.GetId();
	const bool show = (event.GetInt() == 1);

	for (const auto& slot : listWindow)
	{
		if (!slot || slot->id != id) continue;

		auto safeShowSep = [&](bool visible)
			{
				if (slot->separationBar && slot->separationBar->separationBar)
					slot->separationBar->separationBar->Show(visible);
			};

		if (show)
		{
			safeShowSep(true);
			slot->rect = slot->rect_old;
			slot->fixe = slot->fixe_old;
			slot->size = slot->size_old;
			PropagateResize(slot->diffWidth, slot->diffHeight, slot->position);
		}
		else
		{
			slot->fixe_old = slot->fixe;
			slot->fixe = true;
			slot->diffWidth = 0;
			slot->diffHeight = 0;
			slot->size_old = slot->size;

			safeShowSep(false);

			slot->rect_old = slot->rect;

			if (slot->fixe)
			{
				const bool isHSplit = (slot->position == Pos::wxLEFT ||
					slot->position == Pos::wxRIGHT);
				slot->size = isHSplit
					? slot->GetMasterWindowPt()->GetWidth()
					: slot->GetMasterWindowPt()->GetHeight();
			}
		}

		Init();
		break;
	}

	if (show) Resize();
}

// ---------------------------------------------------------------------------
// Layout engine — ComputeLayout() + ApplyLayout()
// ---------------------------------------------------------------------------
//
// Strategy: one pure-computation pass (ComputeLayout) that derives all slot
// geometries from current state, followed by one mutation pass (ApplyLayout)
// that calls SetSize(). No cross-slot side-effects inside computation.
//
// This replaces Init_top/bottom/left/right/Central() + SetWindowXxxSize().

CWindowManager::LayoutContext CWindowManager::ComputeLayout() const
{
	LayoutContext ctx;
	ctx.totalWidth = GetSize().x;
	ctx.totalHeight = GetSize().y;

	CWindowToAdd* left = FindSlot(Pos::wxLEFT);
	CWindowToAdd* right = FindSlot(Pos::wxRIGHT);
	CWindowToAdd* top = FindSlot(Pos::wxTOP);
	CWindowToAdd* bottom = FindSlot(Pos::wxBOTTOM);

	// --- isTop-priority lateral slots first (they span full height) ----------
	if (left && left->isTop)
		ctx.slots[PosIndex(Pos::wxLEFT)] =
		ComputeHorizontalSlot(left, ctx.totalWidth, { 0, ctx.totalHeight }, false);
	if (right && right->isTop)
		ctx.slots[PosIndex(Pos::wxRIGHT)] =
		ComputeHorizontalSlot(right, ctx.totalWidth, { 0, ctx.totalHeight }, true);

	// --- top / bottom (their width may be trimmed by isTop lateral slots) ----
	ctx.slots[PosIndex(Pos::wxTOP)] = ComputeEdgeSlot(top, ctx.totalWidth, ctx.totalHeight, false);
	ctx.slots[PosIndex(Pos::wxBOTTOM)] = ComputeEdgeSlot(bottom, ctx.totalWidth, ctx.totalHeight, true);

	// --- non-isTop lateral slots (their height is trimmed by top/bottom) -----
	const VBand band = ComputeVerticalBand(ctx);
	if (left && !left->isTop)
		ctx.slots[PosIndex(Pos::wxLEFT)] =
		ComputeHorizontalSlot(left, ctx.totalWidth, band, false);
	if (right && !right->isTop)
		ctx.slots[PosIndex(Pos::wxRIGHT)] =
		ComputeHorizontalSlot(right, ctx.totalWidth, band, true);

	// --- central slot fills whatever remains ---------------------------------
	ctx.slots[PosIndex(Pos::wxCENTRAL)] = ComputeCentralSlot(ctx);

	return ctx;
}

CWindowManager::VBand CWindowManager::ComputeVerticalBand(const LayoutContext& ctx) const
{
	const CWindowToAdd* top = FindSlot(Pos::wxTOP);
	const CWindowToAdd* bottom = FindSlot(Pos::wxBOTTOM);

	int y = 0;
	int height = ctx.totalHeight;

	auto consumeTop = [&](const CWindowToAdd* slot)
		{
			if (!slot || slot->isHide) return;
			const int h = slot->fixe ? slot->size : slot->rect.height;
			const int s = SepSize(slot);
			y += h + s;
			height -= h + s;
		};
	auto consumeBottom = [&](const CWindowToAdd* slot)
		{
			if (!slot || slot->isHide) return;
			const int h = slot->fixe ? slot->size : slot->rect.height;
			const int s = SepSize(slot);
			height -= h + s;
		};

	consumeTop(top);
	consumeBottom(bottom);
	return { y, height };
}

CWindowManager::SlotGeometry CWindowManager::ComputeHorizontalSlot(
	CWindowToAdd* slot, int totalWidth, const VBand& band, bool isRight) const
{
	if (!slot) return {};

	SlotGeometry g;
	const int sepSz = SepSize(slot);

	if (slot->fixe)
	{
		// Fixed: rect is fully recomputed each call
		g.window.y = band.y;
		g.window.height = band.height;
		g.window.width = slot->size;
		g.window.x = isRight ? (totalWidth - slot->size) : 0;
	}
	else
	{
		// Flexible: preserve existing rect dimensions, update position/height
		if (slot->rect.width == 0 && slot->rect.height == 0)
		{
			// First-time initialisation — default to 25 %
			slot->rect.width = totalWidth / 4;
			slot->rect.height = band.height;
			slot->rect.x = isRight ? (totalWidth - slot->rect.width) : 0;
			slot->rect.y = band.y;
		}
		else
		{
			slot->rect.y = band.y;
			slot->rect.height = band.height;
		}
		g.window = slot->rect;
	}

	// Separator geometry (always to the inner edge of the slot)
	if (sepSz > 0)
	{
		g.separator.y = g.window.y;
		g.separator.height = g.window.height;
		g.separator.width = sepSz;
		g.separator.x = isRight
			? (g.window.x - sepSz)
			: (g.window.x + g.window.width);
		g.separatorPos = g.separator.x + sepSz / 2;

		// Persist back so drag-delta reads stay valid
		if (slot->separationBar)
		{
			slot->separationBar->rect = g.separator;
			slot->separationBar->posBar = g.separatorPos;
		}
	}
	slot->rect = g.window;
	return g;
}

CWindowManager::SlotGeometry CWindowManager::ComputeEdgeSlot(
	CWindowToAdd* slot, int totalWidth, int totalHeight, bool isBottom) const
{
	if (!slot) return {};

	// Account for isTop lateral slots trimming the edge slot width
	const CWindowToAdd* left = FindSlot(Pos::wxLEFT);
	const CWindowToAdd* right = FindSlot(Pos::wxRIGHT);

	int x = 0;
	int width = totalWidth;

	auto trimLeft = [&](const CWindowToAdd* s)
		{
			if (!s || !s->isTop || s->isHide) return;
			const int w = (s->fixe ? s->size : s->rect.width) + SepSize(s);
			x += w;
			width -= w;
		};
	auto trimRight = [&](const CWindowToAdd* s)
		{
			if (!s || !s->isTop || s->isHide) return;
			width -= (s->fixe ? s->size : s->rect.width) + SepSize(s);
		};

	trimLeft(left);
	trimRight(right);

	SlotGeometry g;
	const int sepSz = SepSize(slot);
	int defaultH = slot->fixe ? slot->size : (totalHeight / 4);

	if (isBottom)
	{
		g.window.x = x;
		g.window.width = width;
		g.window.height = defaultH;
		g.window.y = totalHeight - defaultH;

		if (sepSz > 0 && !slot->fixe)
		{
			if (slot->rect.width == 0 && slot->rect.height == 0)
			{
				// First-time init
				slot->rect = g.window;
				slot->rect.y += sepSz;
				slot->rect.height -= sepSz;
				g.separator = { x, slot->rect.y - sepSz, width, sepSz };
				g.separatorPos = g.separator.y + sepSz / 2;
			}
			else
			{
				slot->rect.x = x;
				slot->rect.width = width;
				g.window = slot->rect;
			}
		}
		else
		{
			slot->rect = g.window;
		}
	}
	else // top
	{
		g.window.x = x;
		g.window.y = 0;
		g.window.width = width;
		g.window.height = defaultH;

		if (sepSz > 0 && !slot->fixe)
		{
			if (slot->rect.width == 0 && slot->rect.height == 0)
			{
				slot->rect = g.window;
				slot->rect.height -= sepSz;
				g.separator = { x, slot->rect.height, width, sepSz };
				g.separatorPos = g.separator.y + sepSz / 2;
			}
			else
			{
				slot->rect.x = x;
				slot->rect.width = width;
				g.window = slot->rect;
			}
		}
		else
		{
			slot->rect = g.window;
		}
	}

	if (slot->separationBar && sepSz > 0)
	{
		slot->separationBar->rect = g.separator;
		slot->separationBar->posBar = g.separatorPos;
	}
	return g;
}

CWindowManager::SlotGeometry CWindowManager::ComputeCentralSlot(
	const LayoutContext& ctx) const
{
	const CWindowToAdd* left = FindSlot(Pos::wxLEFT);
	const CWindowToAdd* right = FindSlot(Pos::wxRIGHT);
	const CWindowToAdd* top = FindSlot(Pos::wxTOP);
	const CWindowToAdd* bottom = FindSlot(Pos::wxBOTTOM);

	int x = 0, y = 0;
	int width = ctx.totalWidth;
	int height = ctx.totalHeight;

	auto consumeEdge = [&](const CWindowToAdd* s, bool isTop)
		{
			if (!s || s->isHide) return;
			const int h = (s->fixe ? s->size : s->rect.height) + SepSize(s);
			if (isTop) { y += h; height -= h; }
			else         height -= h;
		};
	consumeEdge(top, true);
	consumeEdge(bottom, false);

	auto consumeLeft = [&](const CWindowToAdd* s)
		{
			if (!s || s->isHide) return;
			const int w = s->rect.width + (!s->fixe ? SepSize(s) : 0);
			x += w;
			width -= w;
		};
	auto consumeRight = [&](const CWindowToAdd* s)
		{
			if (!s || s->isHide) return;
			const int w = s->rect.width + (!s->fixe ? SepSize(s) : 0);
			width -= w;
			// Reposition right slot x so it starts just after central
			if (CWindowToAdd* rw = FindSlot(Pos::wxRIGHT))
				rw->rect.x = x + width;
		};
	consumeLeft(left);
	consumeRight(right);

	CWindowToAdd* central = FindSlot(Pos::wxCENTRAL);
	if (central)
		central->rect = { x, y, width, height };

	SlotGeometry g;
	g.window = { x, y, width, height };
	return g;
}

void CWindowManager::ApplyLayout(const LayoutContext& ctx)
{
	const wxRect empty;
	for (const auto& slot : listWindow)
	{
		if (!slot) continue;

		wxWindow* wnd = slot->GetWindow();
		if (wnd)
			wnd->SetSize(wnd->IsShown() ? slot->rect : empty);

		if (slot->separationBar && slot->separationBar->separationBar)
		{
			auto* sep = slot->separationBar->separationBar;
			sep->SetSize(sep->IsShown() ? slot->separationBar->rect : empty);
		}
	}
}

// ---------------------------------------------------------------------------
// Init / Resize / ResetPosition
// ---------------------------------------------------------------------------

void CWindowManager::Init()
{
	// Recompute all slot geometries in one pass, then apply.
	const LayoutContext ctx = ComputeLayout();
	(void)ctx; // geometry is written back into slot->rect inside ComputeLayout
	// ApplyLayout() reads slot->rect, no ctx fields needed here
}

void CWindowManager::Resize()
{
	const int width = GetSize().GetX();
	const int height = GetSize().GetY();
	if (width <= 0 || height <= 0) return;

	const int diffWidth = width - oldWidth;
	const int diffHeight = height - oldHeight;

	if (!init)
	{
		init = true;
		Init();
	}
	else
	{
		// Propagate the window size delta to each slot
		for (Pos p : {Pos::wxCENTRAL, Pos::wxLEFT, Pos::wxRIGHT,
			Pos::wxTOP, Pos::wxBOTTOM})
			PropagateResize(diffWidth, diffHeight, p);

		for (const auto& slot : listWindow)
		{
			if (slot)
			{
				slot->diffHeight += diffHeight;
				slot->diffWidth += diffWidth;
			}
		}
	}

	ApplyLayout(ComputeLayout());

	oldWidth = width;
	oldHeight = height;
}

void CWindowManager::ResetPosition()
{
	// [CRITIQUE] All FindSlot() results are guarded before use
	for (Pos p : {Pos::wxRIGHT, Pos::wxTOP, Pos::wxBOTTOM,
		Pos::wxCENTRAL, Pos::wxLEFT})
	{
		if (CWindowToAdd* slot = FindSlot(p))
			slot->rect = {};
	}
	init = false;
}

// ---------------------------------------------------------------------------
// Drag delta propagation  — replaces MoveTop/Bottom/Left/Right
// ---------------------------------------------------------------------------

void CWindowManager::ApplyDragDelta(Pos draggedSide, int delta)
{
	CWindowToAdd* top = FindSlot(Pos::wxTOP);
	CWindowToAdd* bottom = FindSlot(Pos::wxBOTTOM);
	CWindowToAdd* left = FindSlot(Pos::wxLEFT);
	CWindowToAdd* right = FindSlot(Pos::wxRIGHT);
	CWindowToAdd* central = FindSlot(Pos::wxCENTRAL);

	auto moveSepY = [](CWindowToAdd* s, int d)
		{
			if (s && s->separationBar) s->separationBar->rect.y -= d;
		};
	auto moveSepX = [](CWindowToAdd* s, int d)
		{
			if (s && s->separationBar) s->separationBar->rect.x += d;
		};
	auto stretchSepH = [](CWindowToAdd* s, int d)
		{
			if (s && s->separationBar) s->separationBar->rect.height += d;
		};
	auto stretchSepW = [](CWindowToAdd* s, int d)
		{
			if (s && s->separationBar) s->separationBar->rect.width += d;
		};

	switch (draggedSide)
	{
	case Pos::wxTOP:
		if (!top || !central) return;
		central->rect.y -= delta;
		central->rect.height += delta;
		top->rect.height -= delta;
		moveSepY(top, delta);
		if (left) { left->rect.y -= delta; left->rect.height += delta; moveSepY(left, delta); stretchSepH(left, delta); }
		if (right) { right->rect.y -= delta; right->rect.height += delta; moveSepY(right, delta); stretchSepH(right, delta); }
		break;

	case Pos::wxBOTTOM:
		if (!bottom || !central) return;
		central->rect.height -= delta;
		bottom->rect.height += delta;
		bottom->rect.y -= delta;
		moveSepY(bottom, delta);
		if (left) { left->rect.height -= delta; stretchSepH(left, -delta); }
		if (right) { right->rect.height -= delta; stretchSepH(right, -delta); }
		break;

	case Pos::wxRIGHT:
		if (!right || !central) return;
		central->rect.width += delta;
		right->rect.width -= delta;
		right->rect.x += delta;
		moveSepX(right, delta);
		if (right->isTop)
		{
			if (bottom) { bottom->rect.width += delta; stretchSepW(bottom, delta); }
			if (top) { top->rect.width += delta; stretchSepW(top, delta); }
		}
		break;

	case Pos::wxLEFT:
		if (!left || !central) return;
		central->rect.x += delta;
		central->rect.width -= delta;
		left->rect.width += delta;
		moveSepX(left, delta);
		if (left->isTop)
		{
			if (bottom) { bottom->rect.x += delta; bottom->rect.width -= delta; moveSepX(bottom, delta); stretchSepW(bottom, delta); }
			if (top) { top->rect.width -= delta; top->rect.x += delta; moveSepX(top, delta); stretchSepW(top, delta); }
		}
		break;

	default: break;
	}
}

// ---------------------------------------------------------------------------
// Resize propagation — replaces AddDifference()
// ---------------------------------------------------------------------------

void CWindowManager::PropagateResize(int diffWidth, int diffHeight, Pos position)
{
	if (diffWidth == 0 && diffHeight == 0) return;

	CWindowToAdd* slot = FindSlot(position);
	if (!slot) return;

	auto adjustSep = [&](int dw, int dh, int dx = 0, int dy = 0)
		{
			if (slot->separationBar && slot->separationBar->separationBar)
			{
				slot->separationBar->rect.width += dw;
				slot->separationBar->rect.height += dh;
				slot->separationBar->rect.x += dx;
				slot->separationBar->rect.y += dy;
				slot->separationBar->posBar += (dx + dy); // whichever axis matters
			}
		};

	switch (position)
	{
	case Pos::wxCENTRAL:
		slot->rect.width += diffWidth;
		slot->rect.height += diffHeight;
		break;

	case Pos::wxRIGHT:
		slot->rect.x += diffWidth;
		slot->rect.height += diffHeight;
		adjustSep(0, diffHeight, diffWidth, 0);
		if (slot->separationBar) slot->separationBar->posBar += diffWidth;
		break;

	case Pos::wxLEFT:
		slot->rect.height += diffHeight;
		adjustSep(0, diffHeight);
		break;

	case Pos::wxTOP:
		slot->rect.width += diffWidth;
		adjustSep(diffWidth, 0);
		break;

	case Pos::wxBOTTOM:
		slot->rect.width += diffWidth;
		slot->rect.y += diffHeight;
		adjustSep(diffWidth, 0, 0, diffHeight);
		if (slot->separationBar) slot->separationBar->posBar += diffHeight;
		break;

	default: break;
	}
}

// ---------------------------------------------------------------------------
// SetNewPosition — drag-bar interaction
// ---------------------------------------------------------------------------

void CWindowManager::SetNewPosition(CSeparationBar* separationBar)
{
	const wxPoint mouse = ScreenToClient(wxGetMousePosition());
	const int     totW = GetSize().GetX();
	const int     totH = GetSize().GetY();
	CWindowToAdd* central = FindSlot(Pos::wxCENTRAL);

	for (const auto& slot : listWindow)
	{
		if (!slot || !slot->separationBar) continue;
		if (slot->separationBar->separationBarId != separationBar->GetId()) continue;

		const int oldPos = slot->separationBar->posBar;
		int delta = 0;

		if (slot->separationBar->isHorizontal)
		{
			int py = std::clamp(mouse.y, WINDOW_MINSIZE, totH - WINDOW_MINSIZE);

			if (slot->position == Pos::wxTOP && central)
				py = std::min(py, central->rect.y + central->rect.height - WINDOW_MINSIZE);
			if (slot->position == Pos::wxBOTTOM && central)
				py = std::max(py, central->rect.y + WINDOW_MINSIZE);

			slot->separationBar->posBar = py;
			delta = oldPos - py;
		}
		else
		{
			int px = std::clamp(mouse.x, WINDOW_MINSIZE, totW - WINDOW_MINSIZE);

			if (slot->position == Pos::wxLEFT && central)
				px = std::min(px, central->rect.x + central->rect.width - WINDOW_MINSIZE);
			if (slot->position == Pos::wxRIGHT && central)
				px = std::max(px, central->rect.x + WINDOW_MINSIZE);

			slot->separationBar->posBar = px;
			delta = px - oldPos;
		}

		if (moving)
			ApplyDragDelta(slot->position, delta);

		if (fastRender && moving)
		{
			if (slot->separationBar->isHorizontal)
				DrawSeparationBar(slot->separationBar->rect.x, mouse.y,
					slot->separationBar->rect.width,
					themeSplitter.themeFast.size, true);
			else
				DrawSeparationBar(mouse.x, slot->separationBar->rect.y,
					themeSplitter.themeFast.size,
					slot->separationBar->rect.height, false);
		}
		break;
	}

	Resize();
}

// ---------------------------------------------------------------------------
// Show / Hide
// ---------------------------------------------------------------------------

void CWindowManager::HideWindow(Pos position, bool refresh)
{
	CWindowToAdd* slot = FindSlot(position);
	if (!slot || slot->isHide) return;

	wxWindow* wnd = slot->GetWindow();
	if (!wnd) return;

	slot->isHide = true;
	wnd->Show(false);

	if (slot->separationBar && slot->separationBar->separationBar)
		slot->separationBar->separationBar->Show(false);

	if (slot->isPanel)
		SafeShow(slot->GetPanel(), false);

	if (refresh) { Init(); Resize(); }
}

void CWindowManager::ShowWindow(Pos position, bool refresh)
{
	CWindowToAdd* slot = FindSlot(position);
	if (!slot || !slot->isHide) return;

	slot->isHide = false;

	// [CRITIQUE] guard GetWindow() before dereferencing
	if (wxWindow* wnd = slot->GetWindow())
		wnd->Show(true);

	if (slot->separationBar && slot->separationBar->separationBar && !slot->fixe)
		slot->separationBar->separationBar->Show(true);

	if (slot->isPanel)
		SafeShow(slot->GetPanel(), true);

	if (refresh) { Init(); Resize(); }
}

void CWindowManager::HidePaneWindow(Pos position, int refresh)
{
	CWindowToAdd* slot = FindSlot(position);
	if (!slot || !slot->isPanel) return;
	if (CPanelWithClickToolbar* panel = slot->GetPanel())
		if (panel->IsPanelVisible())
			panel->ClosePane(PANE_WITHCLICKTOOLBAR, refresh);
}

void CWindowManager::ShowPaneWindow(Pos position, int refresh)
{
	CWindowToAdd* slot = FindSlot(position);
	if (!slot || !slot->isPanel) return;
	if (CPanelWithClickToolbar* panel = slot->GetPanel())
		if (!panel->IsPanelVisible())
			panel->ClickShowButton(PANE_WITHCLICKTOOLBAR, refresh);
}

// ---------------------------------------------------------------------------
// Accessors
// ---------------------------------------------------------------------------

bool CWindowManager::IsWindowVisible(Pos position) const
{
	const CWindowToAdd* slot = FindSlot(position);
	return slot && !slot->isHide;
}

wxRect CWindowManager::GetWindowSize(Pos position) const
{
	const CWindowToAdd* slot = FindSlot(position);
	return slot ? slot->rect : wxRect{};
}

int CWindowManager::GetPaneState(Pos position) const
{
	CWindowToAdd* slot = FindSlot(position);
	if (!slot || !slot->isPanel) return 0;
	CPanelWithClickToolbar* panel = slot->GetPanel();
	return (panel && panel->IsPanelVisible()) ? 1 : 0;
}

bool CWindowManager::GetSeparationVisibility() const
{
	return showSeparationBar;
}

void CWindowManager::SetWindowSize(Pos position, bool fixe, int size)
{
	CWindowToAdd* slot = FindSlot(position);
	if (slot) { slot->fixe = fixe; slot->size = size; }
}

// ---------------------------------------------------------------------------
// Separation bar visibility
// ---------------------------------------------------------------------------

void CWindowManager::SetSeparationBarVisible(bool visible)
{
	for (const auto& slot : listWindow)
	{
		// [IMPORTANT] guard slot before testing separationBar
		if (slot && slot->separationBar && slot->separationBar->separationBar)
			slot->separationBar->separationBar->Show(visible);
	}
	showSeparationBar = visible;
}

// ---------------------------------------------------------------------------
// Rendering helpers
// ---------------------------------------------------------------------------

void CWindowManager::GenerateRenderBitmap()
{
	renderBitmap = wxBitmap(GetWindowWidth(), GetWindowHeight());
	wxMemoryDC dc(renderBitmap);

	for (const auto& slot : listWindow)
	{
		if (!slot) continue;

		if (wxWindow* wnd = slot->GetWindow(); wnd && wnd->IsShown())
		{
			wxWindowDC wdc(wnd);
			dc.Blit(slot->rect.x, slot->rect.y,
				slot->rect.width, slot->rect.height, &wdc, 0, 0);
		}

		if (showSeparationBar && slot->separationBar &&
			slot->separationBar->separationBar)
		{
			wxWindowDC sdc(slot->separationBar->separationBar);
			const wxRect& sr = slot->separationBar->rect;
			dc.Blit(sr.x, sr.y, sr.width, sr.height, &sdc, 0, 0);
		}
	}
	dc.SelectObject(wxNullBitmap);
}

void CWindowManager::DrawSeparationBar(int x, int y, int width, int height,
	bool horizontal)
{
	wxWindowDC dc(this);
	dc.DrawBitmap(renderBitmap, 0, 0);

	const wxRect rc{ x, y, width, height };
	dc.GradientFillLinear(rc,
		themeSplitter.themeSeparation.secondColor,
		themeSplitter.themeSeparation.firstColor,
		horizontal ? wxSOUTH : wxEAST);
}

// ---------------------------------------------------------------------------
// Mouse interaction
// ---------------------------------------------------------------------------

bool CWindowManager::OnLButtonDown()
{
	SetFocus();
	moving = true;
	if (fastRender) GenerateRenderBitmap();
	return true;
}

void CWindowManager::OnLButtonUp()
{
	moving = false;
	if (fastRender) Resize();
}

// ---------------------------------------------------------------------------
// Screen ratio / misc
// ---------------------------------------------------------------------------

void CWindowManager::UpdateScreenRatio()
{
	for (const auto& slot : listWindow)
	{
		if (!slot) continue;
		if (CWindowMain* master = (CWindowMain*)slot->GetMasterWindowPt())
			master->UpdateScreenRatio();
		if (slot->separationBar && slot->separationBar->separationBar)
			slot->separationBar->separationBar->UpdateScreenRatio();
	}
}

void CWindowManager::UnInit() {}