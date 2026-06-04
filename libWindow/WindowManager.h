#pragma once
#include <memory>
#include <vector>

#include "SeparationBar.h"
#include "WindowMain.h"
#include "WindowOpenGLMain.h"
#include "WindowToAdd.h"

namespace Regards::Window
{
	class CPanelWithClickToolbar;
	class CWindowToAdd;

	// ---------------------------------------------------------------------------
	// CWindowManager
	//
	// Owns its CWindowToAdd slots (via unique_ptr). CSeparationBar children are
	// owned by the wxWidgets window tree (parent = this); the CSeparationBarToAdd
	// wrapper structs inside each slot are owned by the slot itself.
	// ---------------------------------------------------------------------------
	class CWindowManager : public CWindowMain, public IMoveWindow
	{
	public:
		CWindowManager(wxWindow* parent, wxWindowID id, const CThemeSplitter& theme);
		~CWindowManager() override;

		// --- Public API (signatures preserved, scalars passed by value) ---------

		CPanelWithClickToolbar* AddPanel(CWindowMain* window, Pos pos, bool fixe,
			int size, wxRect rect,
			const wxString& panelLabel,
			const wxString& windowName,
			bool isVisible, int idPanel,
			bool refreshButton, bool isTop = false);

		void AddWindow(CWindowMain* window, Pos position, bool fixe, int size,
			wxRect rect, int id, bool isPanel, bool isTop = false);
		void AddWindow(CWindowOpenGLMain* window, Pos position, bool fixe, int size,
			wxRect rect, int id, bool isPanel, bool isTop = false);

		void SetSeparationBarVisible(bool visible);
		bool GetSeparationVisibility() const;

		void ChangeWindow(CWindowMain* window, Pos position, bool isPanel);
		void GenerateRenderBitmap();

		void HideWindow(Pos position, bool refresh = true);
		void ShowWindow(Pos position, bool refresh = true);
		void HidePaneWindow(Pos position, int refresh = 1);
		void ShowPaneWindow(Pos position, int refresh = 1);
		int  GetPaneState(Pos position) const;

		bool OnLButtonDown() override;
		void OnLButtonUp()   override;
		void UpdateScreenRatio() override;

		// IsWindowVisible and GetWindowIsShow were identical — unified here
		bool IsWindowVisible(Pos position) const;
		bool GetWindowIsShow(Pos position) const { return IsWindowVisible(position); }

		void SetNewPosition(CSeparationBar* separationBar) override;
		void Resize() override;

		void UnInit();
		void Init();
		void ResetPosition();

		wxRect GetWindowSize(Pos position) const;
		void   SetWindowSize(Pos position, bool fixe, int size);

	protected:
		// Slots are owned by this vector; raw pointers obtained via FindSlot()
		// are non-owning observers valid only while the vector is not modified.
		std::vector<std::unique_ptr<CWindowToAdd>> listWindow;

	private:
		// --- Slot management ---------------------------------------------------

		// Registers a fully-configured slot. Takes ownership.
		void RegisterSlot(std::unique_ptr<CWindowToAdd> slot,
			Pos position, bool fixe, int size,
			wxRect rect, int id, bool isPanel, bool isTop);

		// Returns a raw observer pointer; nullptr if position not found.
		CWindowToAdd* FindSlot(Pos position) const;

		// --- Event handlers ----------------------------------------------------
		void OnRefreshData(wxCommandEvent& event);
		void OnResize(wxCommandEvent& event) override;

		// --- Layout engine -----------------------------------------------------
		//
		// The layout is computed in a single pass via ComputeLayout(), which fills
		// a LayoutContext with all slot geometries before any SetSize() call.
		// This replaces the six Init_*() methods and eliminates order-dependent
		// side-effects between them.

		struct SlotGeometry
		{
			wxRect window;         // content rect
			wxRect separator;      // separator bar rect (may be empty)
			int    separatorPos{}; // posBar midpoint
		};

		struct LayoutContext
		{
			int totalWidth{};
			int totalHeight{};

			// Per-slot geometry, keyed by Pos cast to int for fast access
			SlotGeometry slots[6]; // indexed by static_cast<int>(Pos)
		};

		LayoutContext ComputeLayout() const;
		void          ApplyLayout(const LayoutContext& ctx);

		// Helpers used by ComputeLayout() ----------------------------------------

		// Returns the effective y-offset and height of the vertical band
		// after accounting for top/bottom slots.
		struct VBand { int y; int height; };
		VBand ComputeVerticalBand(const LayoutContext& ctx) const;

		// Computes the geometry for a left or right slot given the current band.
		SlotGeometry ComputeHorizontalSlot(CWindowToAdd* slot,
			int totalWidth, const VBand& band,
			bool isRight) const;

		// Computes the geometry for a top or bottom slot.
		SlotGeometry ComputeEdgeSlot(CWindowToAdd* slot,
			int totalWidth, int totalHeight,
			bool isBottom) const;

		// Computes the central slot from surrounding already-computed geometries.
		SlotGeometry ComputeCentralSlot(const LayoutContext& ctx) const;

		// --- Drag delta propagation --------------------------------------------
		//
		// Replaces MoveTop / MoveBottom / MoveLeft / MoveRight with a single
		// method dispatched on position.
		void ApplyDragDelta(Pos draggedSide, int delta);

		// --- Resize propagation ------------------------------------------------
		void PropagateResize(int diffWidth, int diffHeight, Pos position);

		// --- Rendering ---------------------------------------------------------
		void DrawSeparationBar(int x, int y, int width, int height, bool horizontal);

		// --- Clamp helpers for SetNewPosition() --------------------------------
		static int ClampBarPositionH(int y, int totalHeight, int centralY,
			Pos side);
		static int ClampBarPositionV(int x, int totalWidth, int centralX,
			Pos side);

		// --- Members -----------------------------------------------------------
		bool           fastRender{ false };
		bool           init{ false };
		int            oldWidth{ 0 };
		int            oldHeight{ 0 };
		bool           moving{ false };
		wxBitmap       renderBitmap;
		bool           showSeparationBar{ true };
		CThemeSplitter themeSplitter;
		int            separationBarSize{ 0 };
	};

} // namespace Regards::Window