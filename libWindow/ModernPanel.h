#pragma once

#include <WindowMain.h>
#include <wx/panel.h>
#include <wx/dcbuffer.h>
#include <wx/sizer.h>
#include <functional>

#define PANE_WITHCLICKTOOLBAR 1

namespace Regards::Window
{
    class CModernPanelHeader : public wxPanel
    {
    public:
        CModernPanelHeader(wxWindow* parent,
            const CThemeToolbar& themeClickInfosToolbar,
            const wxString& title,
            bool vertical = false);

        void SetTitle(const wxString& title);
        void SetCollapsed(bool collapsed);

        std::function<void()> OnCollapse;

    private:
        void OnPaint(wxPaintEvent& event);
        void OnMouseMove(wxMouseEvent& event);
        void OnMouseLeave(wxMouseEvent& event);
        void OnMouseDown(wxMouseEvent& event);


        wxRect GetCollapseRect() const;

        wxString title_;

        bool vertical_ = false;
        bool collapsed_ = false;

        bool hoverCollapse_ = false;

        wxColour background_;
        wxColour foreground_;
        wxColour accent_;
        wxColour hover_;
    };

    class CModernPanel : public CWindowMain
    {
    public:
        CModernPanel(wxWindow* parent,
            const CThemePane& theme,
            const CThemeToolbar& themeClickInfosToolbar,
            const wxString& windowName,
            wxWindowID id,
            const wxString& title,
            bool visible = true,
            bool vertical = false);

        ~CModernPanel() override = default;

        void SetTitle(const wxString& title);

        // Le contenu reste non propriétaire.
        void SetWindow(CWindowMain* window);

        wxWindow* GetWindow();
        wxWindow* GetPaneWindow();

        CWindowMain* GetContentWindow() const;

        void ShowPanel();
        void HidePanel(bool refresh = true);

        void SetCollapsed(bool collapsed);
        bool IsCollapsed() const;

        bool IsPanelVisible() const;

        void ClickShowButton(
            const int& id,
            const int& refresh = 1);

        void ClosePane(
            const int& id,
            const int& refresh = 1);

        void UpdateScreenRatio() override;

        int GetWidth() override;
        int GetHeight() override;

        void SetPanelBackground(const wxColour& colour);

        static CModernPanel* CreatePanel(
            wxWindow* parent,
            const wxString& panelLabel,
            const wxString& windowName,
            const bool& isVisible,
            const int& idPanel,
            const bool& isVertical,
            const bool& refreshButton);

        std::function<void()> OnCollapse;

    protected:
        void Resize() override;

    private:
        void UpdatePanelLayout();
        // Etat ouvert/ferme : le panneau reste visible (barre d'en-tete seule quand il est ferme)
        void SetOpenState(bool open, int refresh);
        void NotifyParent(bool open, int refresh);
        void OnHeaderCollapse();

        void OnShowPane(wxCommandEvent& event);
        void OnClosePane(wxCommandEvent& event);
        void OnRefreshData(wxCommandEvent& event);

        CModernPanelHeader* header_ = nullptr;
        wxPanel* content_ = nullptr;
        CWindowMain* mainWindow_ = nullptr;

        wxBoxSizer* panelSizer_ = nullptr;

        bool isPanelVisible_ = true;
        bool isCollapsed_ = false;
        bool isVertical_ = false;
        bool refreshButton_ = false;

        int panelId_ = 0;
    };
}