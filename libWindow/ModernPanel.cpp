
#include <header.h>
#include "ModernPanel.h"

namespace Regards::Window
{
    namespace
    {
        constexpr int PANEL_ID = 1;

        const wxColour PANEL_BACKGROUND(40, 42, 46);
        const wxColour HEADER_BACKGROUND(48, 50, 55);
        const wxColour HEADER_FOREGROUND(230, 232, 235);
        const wxColour ACCENT_COLOUR(80, 160, 240);
        const wxColour HOVER_COLOUR(65, 68, 74);
    }

    // =========================================================
    // CModernPanelHeader
    // =========================================================

    CModernPanelHeader::CModernPanelHeader(
        wxWindow* parent,
        const wxString& title,
        bool vertical,
        bool showCloseButton)
        : wxPanel(parent),
        title_(title),
        vertical_(vertical),
        showCloseButton_(showCloseButton),
        background_(HEADER_BACKGROUND),
        foreground_(HEADER_FOREGROUND),
        accent_(ACCENT_COLOUR),
        hover_(HOVER_COLOUR)
    {
        SetBackgroundStyle(wxBG_STYLE_PAINT);

        SetMinSize(vertical_
            ? wxSize(FromDIP(38), -1)
            : wxSize(-1, FromDIP(36)));

        Bind(wxEVT_PAINT,
            &CModernPanelHeader::OnPaint, this);

        Bind(wxEVT_MOTION,
            &CModernPanelHeader::OnMouseMove, this);

        Bind(wxEVT_LEAVE_WINDOW,
            &CModernPanelHeader::OnMouseLeave, this);

        Bind(wxEVT_LEFT_DOWN,
            &CModernPanelHeader::OnMouseDown, this);
    }

    void CModernPanelHeader::SetTitle(const wxString& title)
    {
        title_ = title;
        Refresh(false);
    }

    void CModernPanelHeader::SetCollapsed(bool collapsed)
    {
        collapsed_ = collapsed;
        Refresh(false);
    }

    wxRect CModernPanelHeader::GetCollapseRect() const
    {
        const int buttonSize = FromDIP(28);
        const int margin = FromDIP(4);

        if (vertical_)
        {
            return wxRect(
                (GetClientSize().x - buttonSize) / 2,
                GetClientSize().y - buttonSize - margin,
                buttonSize,
                buttonSize);
        }

        return wxRect(
            GetClientSize().x - buttonSize - margin,
            (GetClientSize().y - buttonSize) / 2,
            buttonSize,
            buttonSize);
    }

    wxRect CModernPanelHeader::GetCloseRect() const
    {
        if (!showCloseButton_)
            return wxRect();

        const int buttonSize = FromDIP(28);
        const int margin = FromDIP(4);

        if (vertical_)
        {
            return wxRect(
                (GetClientSize().x - buttonSize) / 2,
                margin,
                buttonSize,
                buttonSize);
        }

        return wxRect(
            GetClientSize().x - 2 * buttonSize - margin,
            (GetClientSize().y - buttonSize) / 2,
            buttonSize,
            buttonSize);
    }

    void CModernPanelHeader::OnPaint(wxPaintEvent&)
    {
        wxAutoBufferedPaintDC dc(this);

        const wxSize size = GetClientSize();

        dc.SetBackground(wxBrush(background_));
        dc.Clear();

        dc.SetPen(*wxTRANSPARENT_PEN);

        if (!vertical_)
        {
            wxFont font(
                FromDIP(10),
                wxFONTFAMILY_DEFAULT,
                wxFONTSTYLE_NORMAL,
                wxFONTWEIGHT_MEDIUM);

            dc.SetFont(font);
            dc.SetTextForeground(foreground_);

            dc.DrawText(
                title_,
                FromDIP(12),
                (size.y - dc.GetCharHeight()) / 2);
        }
        else
        {
            wxFont font(
                FromDIP(10),
                wxFONTFAMILY_DEFAULT,
                wxFONTSTYLE_NORMAL,
                wxFONTWEIGHT_MEDIUM);

            dc.SetFont(font);
            dc.SetTextForeground(foreground_);

            if (!collapsed_)
            {
                dc.DrawRotatedText(
                    title_,
                    FromDIP(12),
                    size.y / 2,
                    90);
            }
        }

        // Bouton réduire / développer
        const wxRect collapseRect = GetCollapseRect();

        if (hoverCollapse_)
        {
            dc.SetBrush(wxBrush(hover_));
            dc.DrawRoundedRectangle(
                collapseRect,
                FromDIP(5));
        }

        const int cx =
            collapseRect.x + collapseRect.width / 2;

        const int cy =
            collapseRect.y + collapseRect.height / 2;

        dc.SetPen(wxPen(foreground_, FromDIP(2)));

        if (!vertical_)
        {
            if (collapsed_)
            {
                dc.DrawLine(cx - FromDIP(3), cy - FromDIP(5),
                    cx + FromDIP(3), cy);

                dc.DrawLine(cx + FromDIP(3), cy,
                    cx - FromDIP(3), cy + FromDIP(5));
            }
            else
            {
                dc.DrawLine(cx - FromDIP(5), cy - FromDIP(2),
                    cx, cy + FromDIP(3));

                dc.DrawLine(cx, cy + FromDIP(3),
                    cx + FromDIP(5), cy - FromDIP(2));
            }
        }
        else
        {
            if (collapsed_)
            {
                dc.DrawLine(cx - FromDIP(5), cy - FromDIP(3),
                    cx, cy + FromDIP(3));

                dc.DrawLine(cx, cy + FromDIP(3),
                    cx + FromDIP(5), cy - FromDIP(3));
            }
            else
            {
                dc.DrawLine(cx - FromDIP(5), cy + FromDIP(3),
                    cx, cy - FromDIP(3));

                dc.DrawLine(cx, cy - FromDIP(3),
                    cx + FromDIP(5), cy + FromDIP(3));
            }
        }

        // Bouton fermer
        if (showCloseButton_)
        {
            const wxRect closeRect = GetCloseRect();

            if (hoverClose_)
            {
                dc.SetBrush(wxBrush(wxColour(180, 55, 55)));
                dc.DrawRoundedRectangle(
                    closeRect,
                    FromDIP(5));
            }

            const int x =
                closeRect.x + closeRect.width / 2;

            const int y =
                closeRect.y + closeRect.height / 2;

            dc.SetPen(wxPen(foreground_, FromDIP(2)));

            dc.DrawLine(x - FromDIP(4), y - FromDIP(4),
                x + FromDIP(4), y + FromDIP(4));

            dc.DrawLine(x + FromDIP(4), y - FromDIP(4),
                x - FromDIP(4), y + FromDIP(4));
        }

        dc.SetPen(wxPen(accent_, FromDIP(1)));

        if (vertical_)
            dc.DrawLine(size.x - 1, 0, size.x - 1, size.y);
        else
            dc.DrawLine(0, size.y - 1, size.x, size.y - 1);
    }

    void CModernPanelHeader::OnMouseMove(wxMouseEvent& event)
    {
        const wxPoint pos = event.GetPosition();

        const bool newCollapse =
            GetCollapseRect().Contains(pos);

        const bool newClose =
            showCloseButton_ &&
            GetCloseRect().Contains(pos);

        if (newCollapse != hoverCollapse_ ||
            newClose != hoverClose_)
        {
            hoverCollapse_ = newCollapse;
            hoverClose_ = newClose;
            Refresh(false);
        }

        SetCursor(wxCursor(
            hoverCollapse_ || hoverClose_
            ? wxCURSOR_HAND
            : wxCURSOR_ARROW));

        event.Skip();
    }

    void CModernPanelHeader::OnMouseLeave(wxMouseEvent&)
    {
        hoverCollapse_ = false;
        hoverClose_ = false;

        SetCursor(wxCursor(wxCURSOR_ARROW));
        Refresh(false);
    }

    void CModernPanelHeader::OnMouseDown(wxMouseEvent& event)
    {
        const wxPoint pos = event.GetPosition();

        if (GetCollapseRect().Contains(pos))
        {
            collapsed_ = !collapsed_;
            Refresh(false);

            if (OnCollapse)
                OnCollapse();

            return;
        }

        if (showCloseButton_ &&
            GetCloseRect().Contains(pos))
        {
            if (OnClose)
                OnClose();

            return;
        }
    }

    // =========================================================
    // CModernPanel
    // =========================================================

    CModernPanel::CModernPanel(
        wxWindow* parent,
        const wxString& windowName,
        wxWindowID id,
        const wxString& title,
        bool visible,
        bool vertical,
        bool showCloseButton)
        : CWindowMain(windowName, parent, id),
        isPanelVisible_(visible),
        isVertical_(vertical),
        panelId_(id)
    {
        SetBackgroundColour(PANEL_BACKGROUND);

        header_ = new CModernPanelHeader(
            this,
            title,
            vertical,
            showCloseButton);

        content_ = new wxPanel(this);
        content_->SetBackgroundColour(PANEL_BACKGROUND);

        panelSizer_ = new wxBoxSizer(
            vertical ? wxHORIZONTAL : wxVERTICAL);

        panelSizer_->Add(
            header_,
            0,
            wxEXPAND);

        panelSizer_->Add(
            content_,
            1,
            wxEXPAND);

        SetSizer(panelSizer_);

        header_->OnCollapse = [this]()
            {
                OnHeaderCollapse();
            };

        header_->OnClose = [this]()
            {
                OnHeaderClose();
            };

        Connect(wxID_ANY, wxEVENT_SHOWPANE,
            wxCommandEventHandler(CModernPanel::OnShowPane));

        Connect(wxID_ANY, wxEVENT_CLOSEPANE,
            wxCommandEventHandler(CModernPanel::OnClosePane));

        Connect(wxID_ANY, wxEVENT_REFRESHDATA,
            wxCommandEventHandler(CModernPanel::OnRefreshData));

        UpdatePanelLayout();
    }


    // 2. Dans le fichier d'en-tête (.h) ou en tant que méthodes de votre classe :
    void CModernPanel::OnShowPane(wxCommandEvent& event)
    {
        ShowPanel();
    }

    void CModernPanel::OnClosePane(wxCommandEvent& event)
    {
        HidePanel();
    }

    void CModernPanel::OnRefreshData(wxCommandEvent& event)
    {
        if (mainWindow_)
        {
            wxCommandEvent outboundEvent(wxEVENT_REFRESHDATA);
            outboundEvent.SetExtraLong(1);

            wxQueueEvent(
                mainWindow_->GetEventHandler(),
                outboundEvent.Clone());
        }
    }



    void CModernPanel::SetTitle(const wxString& title)
    {
        if (header_)
            header_->SetTitle(title);
    }

    void CModernPanel::SetWindow(CWindowMain* window)
    {
        if (window == mainWindow_)
            return;

        wxSize oldSize;

        if (mainWindow_)
        {
            oldSize = mainWindow_->GetSize();
            mainWindow_->Reparent(nullptr);
            mainWindow_->Hide();
        }

        mainWindow_ = window;

        if (mainWindow_)
        {
            mainWindow_->Reparent(content_);
            mainWindow_->SetSize(content_->GetClientSize());
            mainWindow_->Show(true);
        }

        if (!content_->GetSizer())
        {
            content_->SetSizer(
                new wxBoxSizer(wxVERTICAL));
        }

        if (mainWindow_)
        {
            content_->GetSizer()->Add(
                mainWindow_,
                1,
                wxEXPAND);
        }

        if (oldSize.IsFullySpecified() && mainWindow_)
            mainWindow_->SetSize(oldSize);

        content_->Layout();
        Resize();
    }

    wxWindow* CModernPanel::GetWindow()
    {
        return isPanelVisible_
            ? static_cast<wxWindow*>(this)
            : static_cast<wxWindow*>(header_);
    }

    wxWindow* CModernPanel::GetPaneWindow()
    {
        return content_;
    }

    CWindowMain* CModernPanel::GetContentWindow() const
    {
        return mainWindow_;
    }

    void CModernPanel::ShowPanel()
    {
        Show(true);
        isPanelVisible_ = true;
        isCollapsed_ = false;

        UpdatePanelLayout();
        Resize();
    }

    void CModernPanel::HidePanel(bool refresh)
    {
        Show(false);

        if (refresh)
            Refresh(false);
    }

    void CModernPanel::SetCollapsed(bool collapsed)
    {
        if (isCollapsed_ == collapsed)
            return;

        isCollapsed_ = collapsed;

        if (header_)
            header_->SetCollapsed(collapsed);

        UpdatePanelLayout();
        Resize();

        if (OnCollapse)
            OnCollapse();
    }

    bool CModernPanel::IsCollapsed() const
    {
        return isCollapsed_;
    }

    bool CModernPanel::IsPanelVisible() const
    {
        return isPanelVisible_;
    }

    void CModernPanel::ClickShowButton(
        const int& id,
        const int& refresh)
    {
        if (id != panelId_ && id != PANEL_ID)
            return;

        ShowPanel();

        wxCommandEvent event(wxEVENT_RESIZE);
        event.SetId(GetId());
        event.SetInt(1);
        event.SetExtraLong(refresh);

        if (GetParent())
            wxQueueEvent(GetParent(), event.Clone());
    }

    void CModernPanel::ClosePane(
        const int& id,
        const int& refresh)
    {
        if (id != panelId_ && id != PANEL_ID)
            return;

        isPanelVisible_ = false;

        Hide();

        wxCommandEvent event(wxEVENT_RESIZE);
        event.SetId(GetId());
        event.SetInt(0);
        event.SetExtraLong(refresh);

        if (GetParent())
            wxQueueEvent(GetParent(), event.Clone());
    }

    void CModernPanel::UpdateScreenRatio()
    {
        if (mainWindow_)
            mainWindow_->UpdateScreenRatio();

        Layout();
        Resize();
    }

    int CModernPanel::GetWidth()
    {
        return GetSize().x;
    }

    int CModernPanel::GetHeight()
    {
        return GetSize().y;
    }

    void CModernPanel::SetPanelBackground(
        const wxColour& colour)
    {
        SetBackgroundColour(colour);
        content_->SetBackgroundColour(colour);

        Refresh(false);
        content_->Refresh(false);
    }

    CModernPanel* CModernPanel::CreatePanel(
        wxWindow* parent,
        const wxString& panelLabel,
        const wxString& windowName,
        const bool& isVisible,
        const int& idPanel,
        const bool& isVertical,
        const bool& refreshButton)
    {
        auto* panel = new CModernPanel(
            parent,
            windowName,
            idPanel,
            panelLabel,
            isVisible,
            isVertical,
            true);

        panel->refreshButton_ = refreshButton;

        return panel;
    }

    void CModernPanel::Resize()
    {
        if (!panelSizer_)
            return;

        Layout();

        if (mainWindow_ && content_->IsShown())
        {
            mainWindow_->SetSize(
                content_->GetClientRect());

            mainWindow_->Resize();
        }

        needToRefresh = true;
    }

    void CModernPanel::UpdatePanelLayout()
    {
        if (!panelSizer_)
            return;

        if (isCollapsed_)
        {
            content_->Hide();

            if (isVertical_)
                header_->SetMinSize(
                    wxSize(FromDIP(38), -1));
            else
                header_->SetMinSize(
                    wxSize(-1, FromDIP(36)));
        }
        else
        {
            content_->Show();

            if (isVertical_)
                header_->SetMinSize(
                    wxSize(FromDIP(38), -1));
            else
                header_->SetMinSize(
                    wxSize(-1, FromDIP(36)));
        }

        panelSizer_->Layout();
        Layout();
    }

    void CModernPanel::OnHeaderCollapse()
    {
        SetCollapsed(!isCollapsed_);
    }

    void CModernPanel::OnHeaderClose()
    {
        ClosePane(panelId_);
        if (OnClose)
            OnClose();
    }
}
